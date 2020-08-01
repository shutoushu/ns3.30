/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 IITP RAS
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#define NS_LOG_APPEND_CONTEXT                                                \
  if (m_ipv4)                                                                \
    {                                                                        \
      std::clog << "[node " << m_ipv4->GetObject<Node> ()->GetId () << "] "; \
    }

#include "shutoushu-routing-protocol.h"
#include "ns3/log.h"
#include "ns3/boolean.h"
#include "ns3/random-variable-stream.h"
#include "ns3/inet-socket-address.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/udp-l4-protocol.h"
#include "ns3/udp-header.h"
#include "ns3/wifi-net-device.h"
#include "ns3/adhoc-wifi-mac.h"
#include "ns3/string.h"
#include "ns3/pointer.h"
#include <algorithm>
#include <limits>
#include <math.h>

#include "ns3/mobility-module.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ShutoushuRoutingProtocol");

namespace shutoushu {
NS_OBJECT_ENSURE_REGISTERED (RoutingProtocol);

/// UDP Port for SHUTOUSHU control traffic
const uint32_t RoutingProtocol::SHUTOUSHU_PORT = 654;

RoutingProtocol::RoutingProtocol ()
{
}

TypeId
RoutingProtocol::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("ns3::shutoushu::RoutingProtocol")
          .SetParent<Ipv4RoutingProtocol> ()
          .SetGroupName ("Shutoushu")
          .AddConstructor<RoutingProtocol> ()
          .AddAttribute ("UniformRv", "Access to the underlying UniformRandomVariable",
                         StringValue ("ns3::UniformRandomVariable"),
                         MakePointerAccessor (&RoutingProtocol::m_uniformRandomVariable),
                         MakePointerChecker<UniformRandomVariable> ());
  return tid;
}

RoutingProtocol::~RoutingProtocol ()
{
}

void
RoutingProtocol::DoDispose ()
{
}

void
RoutingProtocol::PrintRoutingTable (Ptr<OutputStreamWrapper> stream, Time::Unit unit) const
{
  *stream->GetStream () << "Node: " << m_ipv4->GetObject<Node> ()->GetId ()
                        << "; Time: " << Now ().As (unit)
                        << ", Local time: " << GetObject<Node> ()->GetLocalTime ().As (unit)
                        << ", SHUTOUSHU Routing table" << std::endl;

  //Print routing table here.
  *stream->GetStream () << std::endl;
}

int64_t
RoutingProtocol::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  m_uniformRandomVariable->SetStream (stream);
  return 1;
}

Ptr<Ipv4Route>
RoutingProtocol::RouteOutput (Ptr<Packet> p, const Ipv4Header &header, Ptr<NetDevice> oif,
                              Socket::SocketErrno &sockerr)
{

  //std::cout<<"Route Ouput Node: "<<m_ipv4->GetObject<Node> ()->GetId ()<<"\n";
  Ptr<Ipv4Route> route;

  if (!p)
    {
      std::cout << "loopback occured! in routeoutput";
      return route; // LoopbackRoute (header,oif);
    }

  if (m_socketAddresses.empty ())
    {
      sockerr = Socket::ERROR_NOROUTETOHOST;
      NS_LOG_LOGIC ("No zeal interfaces");
      std::cout << "RouteOutput No zeal interfaces!!, packet drop\n";

      Ptr<Ipv4Route> route;
      return route;
    }

  return route;
}

bool
RoutingProtocol::RouteInput (Ptr<const Packet> p, const Ipv4Header &header,
                             Ptr<const NetDevice> idev, UnicastForwardCallback ucb,
                             MulticastForwardCallback mcb, LocalDeliverCallback lcb,
                             ErrorCallback ecb)
{

  std::cout << "Route Input Node: " << m_ipv4->GetObject<Node> ()->GetId () << "\n";
  return true;
}

void
RoutingProtocol::SetIpv4 (Ptr<Ipv4> ipv4)
{
  NS_ASSERT (ipv4 != 0);
  NS_ASSERT (m_ipv4 == 0);
  m_ipv4 = ipv4;
}

void
RoutingProtocol::NotifyInterfaceUp (uint32_t i)
{
  NS_LOG_FUNCTION (this << m_ipv4->GetAddress (i, 0).GetLocal () << " interface is up");
  Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
  Ipv4InterfaceAddress iface = l3->GetAddress (i, 0);
  if (iface.GetLocal () == Ipv4Address ("127.0.0.1"))
    {
      return;
    }
  // Create a socket to listen only on this interface
  Ptr<Socket> socket;

  socket = Socket::CreateSocket (GetObject<Node> (), UdpSocketFactory::GetTypeId ());
  NS_ASSERT (socket != 0);
  socket->SetRecvCallback (MakeCallback (&RoutingProtocol::RecvShutoushu, this));
  socket->BindToNetDevice (l3->GetNetDevice (i));
  socket->Bind (InetSocketAddress (iface.GetLocal (), SHUTOUSHU_PORT));
  socket->SetAllowBroadcast (true);
  socket->SetIpRecvTtl (true);
  m_socketAddresses.insert (std::make_pair (socket, iface));

  // create also a subnet broadcast socket
  socket = Socket::CreateSocket (GetObject<Node> (), UdpSocketFactory::GetTypeId ());
  NS_ASSERT (socket != 0);
  socket->SetRecvCallback (MakeCallback (&RoutingProtocol::RecvShutoushu, this));
  socket->BindToNetDevice (l3->GetNetDevice (i));
  socket->Bind (InetSocketAddress (iface.GetBroadcast (), SHUTOUSHU_PORT));
  socket->SetAllowBroadcast (true);
  socket->SetIpRecvTtl (true);
  m_socketSubnetBroadcastAddresses.insert (std::make_pair (socket, iface));

  if (m_mainAddress == Ipv4Address ())
    {
      m_mainAddress = iface.GetLocal ();
    }

  NS_ASSERT (m_mainAddress != Ipv4Address ());

  /*  for (uint32_t i = 0; i < m_ipv4->GetNInterfaces (); i++)
        {

          // Use primary address, if multiple
          Ipv4Address addr = m_ipv4->GetAddress (i, 0).GetLocal ();
        //  std::cout<<"############### "<<addr<<" |ninterface "<<m_ipv4->GetNInterfaces ()<<"\n";
       

              TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
              Ptr<Node> theNode = GetObject<Node> ();
              Ptr<Socket> socket = Socket::CreateSocket (theNode,tid);
              InetSocketAddress inetAddr (m_ipv4->GetAddress (i, 0).GetLocal (), SHUTOUSHU_PORT);
              if (socket->Bind (inetAddr))
                {
                  NS_FATAL_ERROR ("Failed to bind() ZEAL socket");
                }
              socket->BindToNetDevice (m_ipv4->GetNetDevice (i));
              socket->SetAllowBroadcast (true);
              socket->SetRecvCallback (MakeCallback (&RoutingProtocol::RecvShutoushu, this));
              //socket->SetAttribute ("IpTtl",UintegerValue (1));
              socket->SetRecvPktInfo (true);

              m_socketAddresses[socket] = m_ipv4->GetAddress (i, 0);

              //  NS_LOG_DEBUG ("Socket Binding on ip " << m_mainAddress << " interface " << i);

              break;
           
        }
*/
}

void
RoutingProtocol::NotifyInterfaceDown (uint32_t i)
{
}

void
RoutingProtocol::NotifyAddAddress (uint32_t i, Ipv4InterfaceAddress address)
{
}

void
RoutingProtocol::NotifyRemoveAddress (uint32_t i, Ipv4InterfaceAddress address)
{
}

void
RoutingProtocol::DoInitialize (void)
{
  int32_t id = m_ipv4->GetObject<Node> ()->GetId ();
  //int32_t time = Simulator::Now ().GetMicroSeconds ();
  //m_trans[id] = 1;

  if (id == 0)
    {
      SetPushBack ();
      SetInterSection ();
    }

  for (int i = 1; i < SimTime; i++)
    {
      Simulator::Schedule (Seconds (i), &RoutingProtocol::SendHelloPacket, this);
      Simulator::Schedule (Seconds (i), &RoutingProtocol::SetMyPos, this);
    }
  // for (int i = 1; i < SimTime; i++)
  //   {
  //     if (id == 3)
  //       Simulator::Schedule (Seconds (i * 4), &RoutingProtocol::SendHelloPacket, this);
  //   }

  //**結果出力******************************************//
  for (int i = 1; i < SimTime; i++)
    {
      if (id == 0)
        Simulator::Schedule (Seconds (i), &RoutingProtocol::SimulationResult,
                             this); //結果出力関数
    }

  //sourse node**********************source node は優先度0 hopcount = 1*************************
  // m_start_time[9] = 24000000; //15second
  // if (id == 1)
  //   Simulator::Schedule (Seconds (24), &RoutingProtocol::SendShutoushuBroadcast, this, 0, 9, 100,
  //                        830, 1);

  // m_start_time[8] = 26000000; //15second
  // if (id == 1)
  //   Simulator::Schedule (Seconds (26), &RoutingProtocol::SendShutoushuBroadcast, this, 0, 8, 130,
  //                        730, 1);

  // m_start_time[7] = 28000000; //15second
  // if (id == 1)
  //   Simulator::Schedule (Seconds (28), &RoutingProtocol::SendShutoushuBroadcast, this, 0, 7, 100,
  //                        680, 1);

  //--------------------------SHUTOUSHU Grid　用---------------------------------------//
  m_start_time[246] = 140000000; //140second key = destination
  if (id == 222)
    Simulator::Schedule (Seconds (140), &RoutingProtocol::SendShutoushuBroadcast, this, 0, 246,
                         1156, 301, 1);

  m_start_time[312] = 145000000; //50second
  if (id == 360)
    Simulator::Schedule (Seconds (145), &RoutingProtocol::SendShutoushuBroadcast, this, 0, 312,
                         2073, 901, 1);

  m_start_time[377] = 150000000; //50second
  if (id == 402)
    Simulator::Schedule (Seconds (150), &RoutingProtocol::SendShutoushuBroadcast, this, 0, 377, 422,
                         1498, 1);

  m_start_time[473] = 155000000; //50second
  if (id == 484)
    Simulator::Schedule (Seconds (155), &RoutingProtocol::SendShutoushuBroadcast, this, 0, 473, 601,
                         2009, 1);

  m_start_time[488] = 16000000; //50second
  if (id == 471)
    Simulator::Schedule (Seconds (160), &RoutingProtocol::SendShutoushuBroadcast, this, 0, 488,
                         2101, 576, 1);

  m_start_time[139] = 165000000; //50second
  if (id == 172)
    Simulator::Schedule (Seconds (165), &RoutingProtocol::SendShutoushuBroadcast, this, 0, 139,
                         1208, 2101, 1);

  m_start_time[395] = 170000000; //50second
  if (id == 40)
    Simulator::Schedule (Seconds (170), &RoutingProtocol::SendShutoushuBroadcast, this, 0, 395, 898,
                         1589, 1);

  m_start_time[558] = 175000000; //50second
  if (id == 58)
    Simulator::Schedule (Seconds (175), &RoutingProtocol::SendShutoushuBroadcast, this, 0, 558,
                         1371, 1201, 1);

  m_start_time[633] = 180000000; //50second
  if (id == 64)
    Simulator::Schedule (Seconds (180), &RoutingProtocol::SendShutoushuBroadcast, this, 0, 633, 601,
                         293, 1);

  m_start_time[223] = 185000000; //50second
  if (id == 407)
    Simulator::Schedule (Seconds (180), &RoutingProtocol::SendShutoushuBroadcast, this, 0, 223,
                         3147, 1832, 1);

  m_start_time[846] = 185000000; //50second
  if (id == 840)
    Simulator::Schedule (Seconds (185), &RoutingProtocol::SendShutoushuBroadcast, this, 0, 846, 601,
                         759, 1);

  m_start_time[640] = 190000000; //50second
  if (id == 644)
    Simulator::Schedule (Seconds (190), &RoutingProtocol::SendShutoushuBroadcast, this, 0, 640,
                         1501, 338, 1);

  m_start_time[268] = 195000000; //50second
  if (id == 243)
    Simulator::Schedule (Seconds (195), &RoutingProtocol::SendShutoushuBroadcast, this, 0, 268,
                         1501, 926, 1);
}

//**window size 以下のhello message の取得回数と　初めて取得した時間を保存する関数**//
void
RoutingProtocol::SetCountTimeMap (void)
{
  int32_t current_time = Simulator::Now ().GetMicroSeconds ();
  int count = 0; //取得回数のカウント変数 この関数が呼び出されるたびに０に初期化
  int check_id = -1; //MAPのIDが変化するタイミングを調べる変数
  for (auto itr = m_recvtime.begin (); itr != m_recvtime.end (); itr++)
    {
      if (check_id == -1) //一番最初にこのループに入ったときの処理
        {
          int32_t dif_time = 0;
          dif_time = current_time - itr->second; //現在の時刻とmapの取得時刻を比較
          if (dif_time < WindowSize)
            {
              check_id = itr->first; //keyを代入
              m_first_recv_time[itr->first] = itr->second; //ループの最初　＝　最初のhello取得時間
              count = 1;
            }
        }
      else
        {
          if (check_id != itr->first) //ループ中のkeyが変わった時
            {
              int32_t dif_time = 0;
              dif_time = current_time - itr->second; //現在の時刻とmapの取得時刻を比較
              //std::cout << "current time" << current_time << "\n";
              //std::cout << "dif_time" << dif_time << "\n";
              if (dif_time < WindowSize)
                {
                  m_first_recv_time[itr->first] = itr->second;
                }
              count = 1; //カウント変数の初期化
              check_id = itr->first; //新しいkeyを代入
              m_recvcount[check_id] = count; //カウント変数を代入
            }
          else
            {
              int32_t dif_time = 0;
              dif_time = current_time - itr->second; //現在の時刻とmapの取得時刻を比較
              //std::cout << "current time" << current_time << "\n";
              //std::cout << "dif_time" << dif_time << "\n";
              if (dif_time < WindowSize) //時間の差がウィンドウサイズ以内だったら
                {
                  auto itr2 = m_first_recv_time.find (itr->first); // キーitr->firstを検索
                  if (itr2 == m_first_recv_time.end ()) //使ったら削除する
                    { // 見つからなかった場合
                      m_first_recv_time[itr->first] = itr->second;
                    }
                  count++; //keyが変わらずループが回っている時カウント変数を加算していく
                  m_recvcount[check_id] = count; //カウント変数を代入
                }
            }
        }

      // int32_t id = m_ipv4->GetObject<Node> ()->GetId ();
      // std::cout << "id" << id << "    current time" << current_time;
      // std::cout << " send node has recvmap  = " << itr->first // キーを表示
      //           << " time  = " << itr->second << "\n"; // 値を表示
    }
}

void
RoutingProtocol::SetEtxMap (void) //////ETXをセットする関数
{
  int32_t current_time = Simulator::Now ().GetMicroSeconds ();
  //current_time = current_time / 1000000; //secondになおす

  for (auto itr = m_first_recv_time.begin (); itr != m_first_recv_time.end (); itr++)
    {
      //double micro_second = 1000000;
      double diftime; //論文のt-t0と同意
      double first_recv = (double) itr->second;
      diftime = (current_time - first_recv) / 1000000;
      //difftime = difftime / 1000000;
      //std::cout << "id" << itr->first << "dif_time" << diftime << "\n";
      double rt = (double) m_recvcount[itr->first] / diftime; //論文のrtの計算
      //std::cout << "id" << itr->first << "のrtは" << rt << "rt*rt" << rt * rt << "\n";
      double etx = 1.000000 / (rt * rt);
      if (etx < 1)
        etx = 1;
      m_etx[itr->first] = etx;

      //std::cout << "id " << itr->first << " m_etx" << m_etx[itr->first] << "\n";
      //std::cout << "\n";
    }
}

void
RoutingProtocol::SetPriValueMap (int32_t des_x, int32_t des_y)
{
  double Dsd; //ソースノードと目的地までの距離(sendSHUTOUSHUbroadcastするノード)
  double Did; //候補ノードと目的地までの距離
  int Distination_x = des_x;
  int Distination_y = des_y;
  Ptr<MobilityModel> mobility = m_ipv4->GetObject<Node> ()->GetObject<MobilityModel> ();
  Vector mypos = mobility->GetPosition ();
  int32_t id = m_ipv4->GetObject<Node> ()->GetId ();
  std::cout << "id" << id << "が持つ\n";

  for (auto itr = m_etx.begin (); itr != m_etx.end (); itr++)
    { ////next                  目的地までの距離とETX値から優先度を示す値をマップに保存する
      Dsd = getDistance (mypos.x, mypos.y, Distination_x, Distination_y);
      Did = getDistance (m_xpoint[itr->first], m_ypoint[itr->first], Distination_x, Distination_y);
      int inter = 0; //初期値0 intersectionにいる場合は1に変わる
      double dif = Dsd - Did;
      if (dif < 0)
        {
          continue;
        }
      //std::cout << "id " << itr->first << " Dsd" << Dsd << " Did" << Did << "\n";

      ///交差点にいるか　いないかの場合分け
      for (int x = 0; x < 2200;)
        {
          for (int y = 0; y < 2200;)
            {
              //std::cout << "x" << x << "y" << y << "\n";
              int DisInter =
                  getDistance (x, y, m_xpoint[itr->first], m_ypoint[itr->first]); //交差点までの距離
              if (DisInter < 8)
                {
                  std::cout << "候補ノードid" << itr->first << "は交差点にいます\n";
                  inter = 1;
                }
              y = y + 300;
            }
          x = x + 300;
        }

      if (inter == 1)
        {
          m_pri_value[itr->first] = (Dsd - Did) / (m_etx[itr->first] * m_etx[itr->first]);
          double angle = getAngle (
              des_x, des_y, mypos.x, mypos.y, m_xpoint[itr->first],
              m_ypoint[itr->first]); //角度Bを求める 中心となる座標b_x b_y = source node  = id = id
          std::cout << "source id" << id << "候補ノードid" << itr->first
                    << "とのdestinationまでの角度は" << angle << "\n";

          m_pri_value[itr->first] = m_pri_value[itr->first] * InterPoint * angle;

          //std::cout << "test angle" << getAngle (1.0, 1.0, 0.0, 0.0, 0.0, 1.0) << "\n";
        }
      else
        {
          m_pri_value[itr->first] = (Dsd - Did) / (m_etx[itr->first] * m_etx[itr->first]);
        }

      std::cout << "id=" << itr->first << "のm_pri_value " << m_pri_value[itr->first] << "\n";
    }
}

void
RoutingProtocol::SendHelloPacket (void)
{
  for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j = m_socketAddresses.begin ();
       j != m_socketAddresses.end (); ++j)
    {
      int32_t id = m_ipv4->GetObject<Node> ()->GetId ();
      Ptr<MobilityModel> mobility = m_ipv4->GetObject<Node> ()->GetObject<MobilityModel> ();
      Vector mypos = mobility->GetPosition ();

      Ptr<Socket> socket = j->first;
      Ipv4InterfaceAddress iface = j->second;
      Ptr<Packet> packet = Create<Packet> ();

      HelloHeader helloHeader (id, mypos.x, mypos.y);
      packet->AddHeader (helloHeader);

      TypeHeader tHeader (SHUTOUSHUTYPE_HELLO);
      packet->AddHeader (tHeader);

      Ipv4Address destination;
      if (iface.GetMask () == Ipv4Mask::GetOnes ())
        {
          destination = Ipv4Address ("255.255.255.255");
        }
      else
        {
          destination = iface.GetBroadcast ();
        }

      Time Jitter = Time (MicroSeconds (m_uniformRandomVariable->GetInteger (0, 50000)));
      //socket->SendTo (packet, 0, InetSocketAddress (destination, SHUTOUSHU_PORT));
      if (m_trans[id] == 1) //通信可能ノードのみ
        {
          Simulator::Schedule (Jitter, &RoutingProtocol::SendToHello, this, socket, packet,
                               destination);
        }
    }
}

void
RoutingProtocol::SendToHello (Ptr<Socket> socket, Ptr<Packet> packet, Ipv4Address destination)
{
  // int32_t id = m_ipv4->GetObject<Node> ()->GetId ();
  // std::cout << " send id " << id << "  time  " << Simulator::Now ().GetMicroSeconds () << "\n";
  socket->SendTo (packet, 0, InetSocketAddress (destination, SHUTOUSHU_PORT));
}

void
RoutingProtocol::SendToShutoushu (Ptr<Socket> socket, Ptr<Packet> packet, Ipv4Address destination,
                                  int32_t hopcount, int32_t des_id)
{
  int32_t id = m_ipv4->GetObject<Node> ()->GetId ();
  int32_t current_time = Simulator::Now ().GetMicroSeconds ();
  std::cout << "send shutoushu m_wait" << m_wait[des_id] << "\n";
  std::cout << "send Shutoushu hopcount" << hopcount << "\n";

  if (m_wait[des_id] == hopcount) //送信するhopcount がまだ待機中だったら
    {
      std::cout << "id " << id << " broadcast----------------------------------------------------"
                << "time" << Simulator::Now ().GetMicroSeconds () << "m_wait" << m_wait[des_id]
                << "\n";
      socket->SendTo (packet, 0, InetSocketAddress (destination, SHUTOUSHU_PORT));
      m_wait.erase (des_id);
      broadcount[des_id] = broadcount[des_id] + 1;
    }
  else
    {
      std::cout << "id " << id
                << " ブロードキャストキャンセル----------------------------------------------------"
                << "m_wait" << m_wait[id] << "time" << current_time << "\n";
    }
}

void
RoutingProtocol::SendShutoushuBroadcast (int32_t pri_value, int32_t des_id, int32_t des_x,
                                         int32_t des_y, int32_t hopcount)
{

  for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j = m_socketAddresses.begin ();
       j != m_socketAddresses.end (); ++j)
    {
      int32_t send_node_id = m_ipv4->GetObject<Node> ()->GetId (); //broadcastするノードID

      if (m_trans[send_node_id] == 0) //通信許可がないノードならbreakする
        {
          std::cout << "通信許可が得られていないノードが　sendshutoushu broadcast id"
                    << send_node_id << "time" << Simulator::Now ().GetMicroSeconds () << "\n";

          std::cout << "m_trans = " << m_trans[send_node_id] << "\n";
          break;
        }

      SetCountTimeMap (); //window sizeないの最初のhelloを受け取った時間と回数をマップに格納する関数
      for (auto itr = m_first_recv_time.begin (); itr != m_first_recv_time.end (); itr++)
        {
          //int32_t id = m_ipv4->GetObject<Node> ()->GetId ();
          // std::cout << "id" << id;
          // std::cout << "が持つ近隣ノードID  = " << itr->first // キーを表示
          //           << "からの最初のHellomessage取得時間は  = " << itr->second << "\n"; // 値を表示
        }
      for (auto itr = m_recvcount.begin (); itr != m_recvcount.end (); itr++)
        {
          //int32_t id = m_ipv4->GetObject<Node> ()->GetId ();
          // std::cout << "id" << id;
          // std::cout << "が持つ近隣ノードID  = " << itr->first // キーを表示
          //           << "からの最初のHellomessageの取得回数は  = " << itr->second
          //           << "\n"; // 値を表示
        }

      SetEtxMap (); //EtX mapをセットする
      SetPriValueMap (des_x, des_y); //優先度を決める値をセットする関数

      // int32_t pri1_node_id = 10000000; ///ダミーID
      // int32_t pri2_node_id = 10000000;
      // int32_t pri3_node_id = 10000000;
      // int32_t pri4_node_id = 10000000;
      // int32_t pri5_node_id = 10000000;

      int32_t pri_id[6];
      pri_id[1] = 10000000; ///ダミーID
      pri_id[2] = 10000000;
      pri_id[3] = 10000000;
      pri_id[4] = 10000000;
      pri_id[5] = 10000000;

      for (int i = 1; i < 6; i++) //5回回して、大きいものから取り出して削除する
        {
          int count = 1;
          int max_id = 10000000;
          double max_value = 0;
          for (auto itr = m_pri_value.begin (); itr != m_pri_value.end (); itr++)
            {

              if (count == 1)
                {
                  max_value = m_pri_value[itr->first];
                }
              else
                {
                  if (max_value < m_pri_value[itr->first])
                    {
                      max_value = m_pri_value[itr->first];
                    }
                }
              count++;
            }

          for (auto itr = m_pri_value.begin (); itr != m_pri_value.end (); itr++)
            {

              if (m_pri_value[itr->first] == max_value)
                {
                  max_id = itr->first;
                }
            }

          m_pri_value.erase (max_id);

          switch (i)
            {
            case 1:
              pri_id[1] = max_id;
              break;
            case 2:
              pri_id[2] = max_id;
              break;
            case 3:
              pri_id[3] = max_id;
              break;
            case 4:
              pri_id[4] = max_id;
              break;
            case 5:
              pri_id[5] = max_id;
              break;
            }
        }
      m_recvcount.clear ();
      m_first_recv_time.clear ();
      m_etx.clear ();
      m_pri_value.clear ();

      for (int i = 1; i < 6; i++)
        {
          if (pri_id[i] != 10000000)
            std::cout << "優先度" << i << "の node id = " << pri_id[i] << "\n";
        }

      // std::cout << "優先度１のIdは " << pri_id[1] << "優先度2のIdは " << pri_id[2]
      //           << "優先度3のIdは " << pri_id[3] << "優先度4のIdは " << pri_id[4]
      //           << "優先度5のIdは " << pri_id[5] << "\n";

      Ptr<Socket> socket = j->first;
      Ipv4InterfaceAddress iface = j->second;
      Ptr<Packet> packet = Create<Packet> ();

      // SendHeader sendHeader (des_id, des_x, des_y, pri1_node_id, pri2_node_id, pri3_node_id,
      //                        pri4_node_id, pri5_node_id);
      if (pri_value != 0) //source node じゃなかったら
        {
          hopcount++;
        }
      SendHeader sendHeader (des_id, des_x, des_y, hopcount, pri_id[1], pri_id[2], pri_id[3],
                             pri_id[4], pri_id[5]);

      packet->AddHeader (sendHeader);

      TypeHeader tHeader (SHUTOUSHUTYPE_SEND);
      packet->AddHeader (tHeader);

      //std::cout << "packet size" << packet->GetSize () << "\n";
      //int32_t current_time = Simulator::Now ().GetMicroSeconds ();

      int32_t wait_time = (pri_value * WaitT) - WaitT; //待ち時間
      m_wait[des_id] = hopcount; //今から待機するホップカウント

      std::cout << " id " << send_node_id << "の待ち時間は  " << wait_time << "\n";
      std::cout << "自身の優先度pri value" << pri_value << "\n";
      std::cout << "m_wait(hopcount)" << m_wait[des_id] << "\n";
      std::cout << "-------------------------------------------\n";

      // Send to all-hosts broadcast if on /32 addr, subnet-directed otherwise
      Ipv4Address destination;
      if (iface.GetMask () == Ipv4Mask::GetOnes ())
        {
          destination = Ipv4Address ("255.255.255.255");
        }
      else
        {
          destination = iface.GetBroadcast ();
        }

      if (pri_value == 0) //初期のソースノードなら無条件にbroadcast
        {
          socket->SendTo (packet, 0, InetSocketAddress (destination, SHUTOUSHU_PORT));
          std::cout << "id " << send_node_id
                    << " broadcast----------------------------------------------------"
                    << "time" << Simulator::Now ().GetMicroSeconds () << "\n";
        }
      else
        {
          Simulator::Schedule (MicroSeconds (wait_time), &RoutingProtocol::SendToShutoushu, this,
                               socket, packet, destination, hopcount, des_id);
        }
    }
} // namespace shutoushu

void
RoutingProtocol::RecvShutoushu (Ptr<Socket> socket)
{
  int32_t id = m_ipv4->GetObject<Node> ()->GetId ();
  Address sourceAddress;
  Ptr<Packet> packet = socket->RecvFrom (sourceAddress);
  TypeHeader tHeader (SHUTOUSHUTYPE_HELLO);
  packet->RemoveHeader (tHeader);

  if (!tHeader.IsValid ())
    {
      NS_LOG_DEBUG ("Senko protocol message " << packet->GetUid ()
                                              << " with unknown type received: " << tHeader.Get ()
                                              << ". Drop");
      return; // drop
    }
  switch (tHeader.Get ())
    {
      case SHUTOUSHUTYPE_HELLO: { //hello message を受け取った場合

        if (m_trans[id] == 0)
          break;

        HelloHeader helloheader;
        packet->RemoveHeader (helloheader); //近隣ノードからのhello packet
        int32_t recv_hello_id = helloheader.GetNodeId (); //NOde ID
        int32_t recv_hello_posx = helloheader.GetPosX (); //Node xposition
        int32_t recv_hello_posy = helloheader.GetPosY (); //Node yposition
        int32_t recv_hello_time = Simulator::Now ().GetMicroSeconds (); //

        // // ////*********recv hello packet log*****************////////////////
        // std::cout << "Node ID " << id << "が受信したHello packetは"
        //           << "id:" << recv_hello_id << "xposition" << recv_hello_posx << "yposition"
        //           << recv_hello_posy << "\n";
        // // ////*********************************************////////////////
        SaveXpoint (recv_hello_id, recv_hello_posx);
        SaveYpoint (recv_hello_id, recv_hello_posy);
        SaveRecvTime (recv_hello_id, recv_hello_time);

        break; //breakがないとエラー起きる
      }
      case SHUTOUSHUTYPE_SEND: {
        if (m_trans[id] == 0)
          break;

        //std::cout << "\n\n--------------------------------------------------------\n";
        //std::cout << "recv id" << id << "time------------------------------------------"
        // << Simulator::Now ().GetMicroSeconds () << "\n";

        SendHeader sendheader;
        packet->RemoveHeader (sendheader);

        int32_t des_id = sendheader.GetDesId ();
        int32_t des_x = sendheader.GetPosX ();
        int32_t des_y = sendheader.GetPosY ();
        int32_t hopcount = sendheader.GetHopcount ();
        // int32_t pri1_id = sendheader.GetId1 ();
        // int32_t pri2_id = sendheader.GetId2 ();
        // int32_t pri3_id = sendheader.GetId3 ();
        // int32_t pri4_id = sendheader.GetId4 ();
        // int32_t pri5_id = sendheader.GetId5 ();

        if (des_id == id) //宛先が自分だったら
          {
            std::cout << "time" << Simulator::Now ().GetMicroSeconds () << "  id" << id
                      << "受信しましたよ　成功しました-------------\n";
            if (m_finish_time[des_id] == 0)
              m_finish_time[des_id] = Simulator::Now ().GetMicroSeconds ();
            break;
          }

        int32_t pri_id[] = {sendheader.GetId1 (), sendheader.GetId2 (), sendheader.GetId3 (),
                            sendheader.GetId4 (), sendheader.GetId5 ()};

        ////*********recv hello packet log*****************////////////////
        // std::cout << " id " << id << "が受信したshutoushu packetは\n";
        // for (int i = 0; i < 5; i++)
        //   {
        //     if (pri_id[i] != 1000000)
        //       std::cout << "pri_id[" << i << "] =" << pri_id[i] << "すなわち優先度は" << i + 1
        //                 << "\n";
        //   }
        // std::cout << "from id" << hopcount << "\n";
        //           << "id:" << des_id << "xposition" << des_x << "yposition" << des_y
        //           << "priority 1 node id" << pri1_id << "priority 2 node id" << pri2_id
        //           << "priority 3 node id" << pri3_id << "priority 4 node id" << pri4_id
        //           << "priority 5 node id" << pri5_id << "\n";
        ////*********************************************////////////////

        if (m_wait[des_id] != 0) //待ち状態ならば
          {
            if (m_wait[des_id] >= hopcount)
              { //待機中のホップカウントより大きいホップカウントを受け取ったなら
                m_wait.erase (des_id);
              }

            for (int i = 0; i < 5; i++)
              {
                if (id == pri_id[i]) //packetに自分のIDが含まれているか
                  {
                    //std::cout << "\n--------------------------------------------------------\n";
                    std::cout << "関係あるrecv id" << id << "time------------------------------\n"
                              << Simulator::Now ().GetMicroSeconds ();
                    SendShutoushuBroadcast (i + 1, des_id, des_x, des_y, hopcount);
                  }
                else //含まれていないか
                  {
                    //m_wait.erase (des_id); //マップの初期化をして　broadcastを止める
                  }
              }
          }
        else //待ち状態じゃないならば
          {
            for (int i = 0; i < 5; i++)
              {
                if (id == pri_id[i]) //packetに自分のIDが含まれているか
                  {
                    //std::cout << "\n--------------------------------------------------------\n";
                    std::cout << "関係あるrecv id" << id << "time------------------------------"
                              << Simulator::Now ().GetMicroSeconds () << "\n";
                    SendShutoushuBroadcast (i + 1, des_id, des_x, des_y, hopcount);
                  }
                else //含まれていないか
                  {
                  }
              }
          }

        break;
      }
    }
}

void
RoutingProtocol::SaveXpoint (int32_t map_id, int32_t map_xpoint)
{
  m_xpoint[map_id] = map_xpoint;
}

void
RoutingProtocol::SaveYpoint (int32_t map_id, int32_t map_ypoint)
{
  m_ypoint[map_id] = map_ypoint;
}

void
RoutingProtocol::SaveRecvTime (int32_t map_id, int32_t map_recvtime)
{
  m_recvtime.insert (std::make_pair (map_id, map_recvtime));
}

void
RoutingProtocol::SendXBroadcast (void)
{
}

int
RoutingProtocol::getDistance (double x, double y, double x2, double y2)
{
  double distance = std::sqrt ((x2 - x) * (x2 - x) + (y2 - y) * (y2 - y));

  return (int) distance;
}

double
RoutingProtocol::getAngle (double a_x, double a_y, double b_x, double b_y, double c_x, double c_y)
{
  double BA_x = a_x - b_x; //ベクトルAのｘ座標
  double BA_y = a_y - b_y; //ベクトルAのy座標
  double BC_x = c_x - b_x; //ベクトルCのｘ座標
  double BC_y = c_y - b_y; //ベクトルCのy座標

  double BABC = BA_x * BC_x + BA_y * BC_y;
  double BA_2 = (BA_x * BA_x) + (BA_y * BA_y);
  double BC_2 = (BC_x * BC_x) + (BC_y * BC_y);

  //double radian = acos (cos);
  //double angle = radian * 180 / 3.14159265;

  double radian = acos (BABC / (std::sqrt (BA_2 * BC_2)));
  double angle = radian * 180 / 3.14159265; //ラジアンから角度に変換
  return angle;
}

void
RoutingProtocol::SetMyPos (void)
{
  int32_t id = m_ipv4->GetObject<Node> ()->GetId ();
  Ptr<MobilityModel> mobility = m_ipv4->GetObject<Node> ()->GetObject<MobilityModel> ();
  Vector mypos = mobility->GetPosition ();
  if (m_my_posx[id] == 0 && m_my_posy[id] == 0)
    {
      m_my_posx[id] = mypos.x;
      m_my_posy[id] = mypos.y;
    }
  else
    {
      double distance = 0; //1秒前の自分の位置との距離の差
      distance = getDistance ((double) m_my_posx[id], (double) m_my_posy[id], (double) mypos.x,
                              (double) mypos.y);

      if (m_trans[id] == 1)
        {
          //std::cout << "id " << id << "distance" << distance << "\n";
          // if (m_stop_count[id] > StopTransTime)
          //   {
          //     m_trans[id] = 0; //通信不可能にする
          //     std::cout << "id " << id << "time" << Simulator::Now ().GetMicroSeconds ()
          //               << "は通信不可能になりました\n";
          //   }

          // if (distance == 0)
          //   {
          //     m_stop_count[id]++; //静止してる時ストップタイムを加算
          //     //std::cout << "id " << id << "静止しとる\n";
          //   }

          // if (distance > 0)
          //   m_stop_count[id] = 0; //初期値に戻す
        }
      else //m_transs[id] == 0
        {
          if (distance > 0)
            {
              m_trans[id] = 1; //動き出し通信可能に
              m_stop_count[id] = 0;
              std::cout << "id " << id << "time" << Simulator::Now ().GetMicroSeconds ()
                        << "は通信可能になりました\n";
            }
        }

      m_my_posx[id] = mypos.x;
      m_my_posy[id] = mypos.y;
    }
}

// シミュレーション結果の出力関数
void
RoutingProtocol::SimulationResult (void) //
{
  std::cout << "time" << Simulator::Now ().GetSeconds () << "\n";
  //int32_t id = m_ipv4->GetObject<Node> ()->GetId ();
  if (Simulator::Now ().GetSeconds () == SimTime - 2)
    {
      // //*******************************ノードが持つ座標の確認ログ***************************//
      //std::cout << "id=" << id << "の\n";
      // for (auto itr = m_xpoint.begin (); itr != m_xpoint.end (); itr++)
      //   {
      //     std::cout << "recv hello packet id = " << itr->first // キーを表示
      //               << ", x座標 = " << itr->second << "\n"; // 値を表示
      //   }
      // for (auto itr = m_ypoint.begin (); itr != m_ypoint.end (); itr++)
      //   {
      //     std::cout << "recv hello packet id = " << itr->first // キーを表示
      //               << ", y座標 = " << itr->second << "\n"; // 値を表示
      //   }
      //**************************************************************************************//

      //*******************************ノードが持つ受信時刻ログ***************************//
      //std::cout << "id=" << id << "recv数" << m_recvtime.size () << "\n";

      // auto itr = m_recvtime.find (1);
      // if (itr != m_recvtime.end ())
      //   { // 見つかった場合

      //     for (auto itr = m_recvtime.begin (); itr != m_recvtime.end (); itr++)
      //       {
      //         std::cout << itr->first << " " << itr->second << "\n"; //  キー、値を表示
      //       }
      //   }
      // itr = m_recvtime.find (1);
      // if (itr == m_recvtime.end ())
      //   { // 見つからなかった場合
      //     std::cout << "not found.\n";
      //   }
      //**************************************************************************************//
      std::cout << "\n\n\n結果出力----------------------------------\n\n";
      for (auto itr = broadcount.begin (); itr != broadcount.end (); itr++)
        {
          std::cout << "des id " << itr->first << "shutoushu broadcast数" << broadcount[itr->first]
                    << "\n";
        }
      for (auto itr = m_finish_time.begin (); itr != m_finish_time.end (); itr++)
        {
          int end_to_end_time = m_finish_time[itr->first] - m_start_time[itr->first];
          std::cout << "destination id = " << itr->first
                    << "end to end deley time = " << end_to_end_time << "\n";
        }
      double packet_recv_rate = (double) m_finish_time.size () / (double) m_start_time.size ();
      std::cout << "本シミュレーションのパケット到達率は" << packet_recv_rate << "\n";
      std::cout << "交差点ノードにおける重み付けは" << InterPoint << "\n";
    }
}

std::map<int, int> RoutingProtocol::broadcount; //key 0 value broudcast数
std::map<int, int> RoutingProtocol::m_start_time; //key destination_id value　送信時間
std::map<int, int> RoutingProtocol::m_finish_time; //key destination_id value 受信時間
std::map<int, double> RoutingProtocol::m_my_posx; // key node id value position x
std::map<int, double> RoutingProtocol::m_my_posy; // key node id value position y
std::map<int, int> RoutingProtocol::m_trans; //key node id value　通信可能かどうか1or0
std::map<int, int> RoutingProtocol::m_stop_count; //key node id value 止まっている時間カウント
std::vector<int> RoutingProtocol::intersection_x; //交差点のx座標
std::vector<int> RoutingProtocol::intersection_y; //交差点のy座標

} // namespace shutoushu
} // namespace ns3
