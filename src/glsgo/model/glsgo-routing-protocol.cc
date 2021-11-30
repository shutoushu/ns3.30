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
 * 
 * 
 * 
 * 
 * チェック項目　ノード数　シミュレーション時間　ブロードキャスト開始時間　ファイル読み取りのファイル名
 *
 */
#define NS_LOG_APPEND_CONTEXT                                                \
  if (m_ipv4)                                                                \
    {                                                                        \
      std::clog << "[node " << m_ipv4->GetObject<Node> ()->GetId () << "] "; \
    }

#include "glsgo-routing-protocol.h"
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
#include "../../shutoushu/model/shutoushu-routing-protocol.h"
#include <algorithm>
#include <limits>
#include <math.h>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <random>
#include <numeric>
#include <iterator>
#include <sys/stat.h> // stat

#include "ns3/mobility-module.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("GlsgoRoutingProtocol");

namespace glsgo {
NS_OBJECT_ENSURE_REGISTERED (RoutingProtocol);

/// UDP Port for GLSGO control traffic
const uint32_t RoutingProtocol::GLSGO_PORT = 654;
int numVehicle = 0; //グローバル変数
int packetCount = 0; //パケット出力数のカウント
int sendpacketCount = 0; //sendpacket 出力数のカウント

RoutingProtocol::RoutingProtocol ()
{
}

TypeId
RoutingProtocol::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("ns3::glsgo::RoutingProtocol")
          .SetParent<Ipv4RoutingProtocol> ()
          .SetGroupName ("Glsgo")
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
                        << ", GLSGO Routing table" << std::endl;

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
  socket->SetRecvCallback (MakeCallback (&RoutingProtocol::RecvGlsgo, this));
  socket->BindToNetDevice (l3->GetNetDevice (i));
  socket->Bind (InetSocketAddress (iface.GetLocal (), GLSGO_PORT));
  socket->SetAllowBroadcast (true);
  socket->SetIpRecvTtl (true);
  m_socketAddresses.insert (std::make_pair (socket, iface));

  // create also a subnet broadcast socket
  socket = Socket::CreateSocket (GetObject<Node> (), UdpSocketFactory::GetTypeId ());
  NS_ASSERT (socket != 0);
  socket->SetRecvCallback (MakeCallback (&RoutingProtocol::RecvGlsgo, this));
  socket->BindToNetDevice (l3->GetNetDevice (i));
  socket->Bind (InetSocketAddress (iface.GetBroadcast (), GLSGO_PORT));
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
              InetSocketAddress inetAddr (m_ipv4->GetAddress (i, 0).GetLocal (), GLSGO_PORT);
              if (socket->Bind (inetAddr))
                {
                  NS_FATAL_ERROR ("Failed to bind() ZEAL socket");
                }
              socket->BindToNetDevice (m_ipv4->GetNetDevice (i));
              socket->SetAllowBroadcast (true);
              socket->SetRecvCallback (MakeCallback (&RoutingProtocol::RecvGlsgo, this));
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
  numVehicle++;


  for (int i = 1; i < SimTime; i++)
    {
      if (id != 0)
        Simulator::Schedule (Seconds (i), &RoutingProtocol::SendHelloPacket, this);
      Simulator::Schedule (Seconds (i), &RoutingProtocol::SetMyPos, this);
    }

  if (id == 0)
    {
      Simulator::Schedule (Seconds (Grobal_StartTime - 2), &RoutingProtocol::SourceAndDestination,
                           this);
    }

  //**結果出力******************************************//
  for (int i = 1; i < SimTime; i++)
    {
      if (id == 0)
        Simulator::Schedule (Seconds (i), &RoutingProtocol::SimulationResult,
                             this); //結果出力関数
    }

  for (int i = 0; i < Grobal_SourceNodeNum; i++)
    {
      Simulator::Schedule (Seconds (Grobal_StartTime + i * 1), &RoutingProtocol::Send, this);
    }
  ///////////////////////////////////////////////////////////////////////////////////////////
}

void
RoutingProtocol::SourceAndDestination ()
{
  std::cout << "source and                        destination function\n";
  for (int i = 0; i < numVehicle; i++) ///node数　設定する
    {
      if (m_my_posx[i] >= SourceLowX && m_my_posx[i] <= SourceHighX && m_my_posy[i] >= SourceLowY &&
          m_my_posy[i] <= SourceHighY)
        {
          source_list.push_back (i);
          // std::cout<<"source list id" << i << "position x"<<m_my_posx[i]<<"y"<<m_my_posy[i]<<"\n";
        }
      
      //geocast の場合 destination  node は指定しない
      if (m_my_posx[i] >= DesLowX && m_my_posx[i] <= DesHighX && m_my_posy[i] >= DesLowY &&
          m_my_posy[i] <= DesHighY)
        {
          des_list.push_back (i);
          // std::cout<<"destination list id" << i << "position x"<<m_my_posx[i]<<"y"<<m_my_posy[i]<<"\n";
        }
    }

  std::mt19937 get_rand_mt (Grobal_Seed);

  std::shuffle (source_list.begin (), source_list.end (), get_rand_mt);
  std::shuffle (des_list.begin (), des_list.end (), get_rand_mt);

  for (int i = 0; i < Grobal_SourceNodeNum; i++)
    {
      std::cout << "shuffle source id" << source_list[i] << "\n";
      // std::cout << "shuffle destination id" << des_list[i] << "\n";
    }
}

void
RoutingProtocol::Send ()
{
  int32_t id = m_ipv4->GetObject<Node> ()->GetId ();
  int time = Simulator::Now ().GetSeconds ();

  if (time >= Grobal_StartTime)
    {
      if (id == source_list[time - Grobal_StartTime])
        {
          int index_time =
              time -
              Grobal_StartTime; //example time16 simstarttime15のときm_source_id = 1 すなわち２つめのsourceid

          Ptr<MobilityModel> mobility = m_ipv4->GetObject<Node> ()->GetObject<MobilityModel> ();
          Vector mypos = mobility->GetPosition ();
          int MicroSeconds = Simulator::Now ().GetMicroSeconds ();
          m_start_time[des_list[index_time]] = MicroSeconds + 300000; //秒数をずらし多分足す
          std::cout << "m_start_time" << m_start_time[des_list[index_time]] << "\n";
          double shift_time = 0.3; //送信時間を0.3秒ずらす
          Simulator::Schedule (Seconds (shift_time), &RoutingProtocol::MulticastFirstSend, this);
          std::cout << "\n\n\n\n\ngeocasting source node point x=" << mypos.x << "y=" << mypos.y << std::endl;
          MulticastRegionRegister (id); 
          //source nodeのidを引数として渡す source node idに紐づくmulticast region node idを登録
        }
    }
}

//送信タイミングにmulticast regionにいるノードIDをmultimapに保存
void
RoutingProtocol::MulticastRegionRegister (int32_t source_id)
{
  for (auto itr = m_my_posx.begin (); itr != m_my_posx.end (); itr++)
  {
    if (m_my_posx[itr->first] >= DesLowX && m_my_posx[itr->first] <= DesHighX && 
    m_my_posy[itr->first] >= DesLowY && m_my_posy[itr->first] <= DesHighY)
        {
          m_multicast_region_id.insert (std::make_pair (source_id, itr->first));
        }
  }
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
  for (auto itr = m_first_recv_time.begin (); itr != m_first_recv_time.end (); itr++)
    {
      double diftime; //論文のt-t0と同意
      double first_recv = (double) itr->second;
      diftime = (current_time - first_recv) / 1000000;
      double rt = (double) m_recvcount[itr->first] / diftime; //論文のrtの計算
      if (rt > 1)
        rt = 1.0;
      m_rt[itr->first] = rt;
      double etx = 1.000000 / (rt * rt);
      if (etx < 1)
        etx = 1;
      m_etx[itr->first] = etx;
    }
}

void
RoutingProtocol::SetPriValueMap (int32_t des_x, int32_t des_y)
{
  double Dsd; //ソースノードと目的地までの距離(sendGLSGObroadcastするノード)
  double Did; //候補ノードと目的地までの距離
  int Distination_x = des_x;
  int Distination_y = des_y;
  Ptr<MobilityModel> mobility = m_ipv4->GetObject<Node> ()->GetObject<MobilityModel> ();
  Vector mypos = mobility->GetPosition ();

  for (auto itr = m_etx.begin (); itr != m_etx.end (); itr++)
    { ////next                  目的地までの距離とETX値から優先度を示す値をマップに保存する
      Dsd = getDistance (mypos.x, mypos.y, Distination_x, Distination_y);
      Did = getDistance (m_xpoint[itr->first], m_ypoint[itr->first], Distination_x, Distination_y);
      double dif = Dsd - Did;
      if (dif < 0)
        {
          continue;
        }
      m_pri_value[itr->first] = (Dsd - Did) / (m_etx[itr->first] * m_etx[itr->first]);
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

      TypeHeader tHeader (GLSGOTYPE_HELLO);
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
      //socket->SendTo (packet, 0, InetSocketAddress (destination, GLSGO_PORT));
      Simulator::Schedule (Jitter, &RoutingProtocol::SendToHello, this, socket, packet,
                           destination);
    }
}

void
RoutingProtocol::SendToHello (Ptr<Socket> socket, Ptr<Packet> packet, Ipv4Address destination)
{
  socket->SendTo (packet, 0, InetSocketAddress (destination, GLSGO_PORT));
}
void
RoutingProtocol::SendToFlooding (Ptr<Socket> socket, Ptr<Packet> packet, Ipv4Address destination)
{
  socket->SendTo (packet, 0, InetSocketAddress (destination, GLSGO_PORT));
}

void
RoutingProtocol::SendToGlsgo (Ptr<Socket> socket, Ptr<Packet> packet, Ipv4Address destination,
                             int32_t hopcount, int32_t source_id)
{
  int32_t id = m_ipv4->GetObject<Node> ()->GetId ();
  Ptr<MobilityModel> mobility = m_ipv4->GetObject<Node> ()->GetObject<MobilityModel> ();
  Vector mypos = mobility->GetPosition ();

  if (m_wait[source_id] == hopcount) //送信するhopcount がまだ待機中だったら
    {
      std::cout << "id " << id << " geocast----------------------------------------------------"
                << "time" << Simulator::Now ().GetMicroSeconds () << "m_wait" << m_wait[source_id]
                << "position(" << mypos.x << "," << mypos.y << ")"
                << "\n";
      socket->SendTo (packet, 0, InetSocketAddress (destination, GLSGO_PORT));
      m_wait.erase (source_id);
      if (m_finish_time[source_id] == 0) // まだ受信車両が受信してなかったら
        {
          s_send_log[m_send_check[source_id]] = 1;
          broadcount[source_id] = broadcount[source_id] + 1;
        }
    }
  else
    {
      //rebroadcast cancel
      std::cout << "geocast rebroadcast cancel \n";
      m_send_check.clear (); //broadcast canselされたのでsend checkしたindexをクリア
    }
}

void
RoutingProtocol::MulticastFirstSend (void)
{
  for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j = m_socketAddresses.begin ();
       j != m_socketAddresses.end (); ++j)
    {
      int32_t source_node_id = m_ipv4->GetObject<Node> ()->GetId (); //geocastのsource node id
      Ptr<MobilityModel> mobility = m_ipv4->GetObject<Node> ()->GetObject<MobilityModel> ();
      Vector mypos = mobility->GetPosition (); //source nodeの位置情報
      int candidate_node_id[NumCandidateNodes + 1];
      std::cout << "source node id " << source_node_id << "decision candidate node\n";
      DecisionRelayCandidateNode(candidate_node_id);

      s_source_id.push_back (source_node_id);
      s_source_x.push_back (mypos.x);
      s_source_y.push_back (mypos.y);
      s_time.push_back (Simulator::Now ().GetMicroSeconds ());
      s_hop.push_back (1);
      s_pri_1_id.push_back (candidate_node_id[1]);
      s_pri_2_id.push_back (candidate_node_id[2]);
      s_pri_3_id.push_back (candidate_node_id[3]);
      s_pri_4_id.push_back (candidate_node_id[4]);
      s_pri_5_id.push_back (candidate_node_id[5]);
      s_pri_1_r.push_back (m_rt[candidate_node_id[1]]);
      s_pri_2_r.push_back (m_rt[candidate_node_id[2]]);
      s_pri_3_r.push_back (m_rt[candidate_node_id[3]]);
      s_pri_4_r.push_back (m_rt[candidate_node_id[4]]);
      s_pri_5_r.push_back (m_rt[candidate_node_id[5]]);
      s_des_id.push_back (10000000);
      s_send_log.push_back (source_node_id);
      m_send_check[source_node_id] = sendpacketCount;

      sendpacketCount++;
      m_recvcount.clear ();
      m_first_recv_time.clear ();
      m_etx.clear ();
      m_pri_value.clear ();
      m_rt.clear (); //保持している予想伝送確率をクリア

      Ptr<Socket> socket = j->first;
      Ipv4InterfaceAddress iface = j->second;
      Ptr<Packet> packet = Create<Packet> ();

      SendHeader sendHeader (source_node_id, GeocastCenterX, GeocastCenterY, 
                             source_node_id, mypos.x, mypos.y, 1,
                             candidate_node_id[1], candidate_node_id[2], 
                             candidate_node_id[3], candidate_node_id[4], candidate_node_id[5]);

      packet->AddHeader (sendHeader);

      TypeHeader tHeader (GLSGOTYPE_SEND);
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

      socket->SendTo (packet, 0, InetSocketAddress (destination, GLSGO_PORT));
      std::cout << "id " << source_node_id
                    << " source node geocast----------------------------------------------------"
                    << "time" << Simulator::Now ().GetMicroSeconds () << "\n";
    }
}

void
RoutingProtocol::DecisionRelayCandidateNode (int32_t candidate_node_id[NumCandidateNodes + 1])
{
  for(int i = 1; i < NumCandidateNodes + 1; i++)
  {
    candidate_node_id[i] = 10000000; ///ダミーID で初期化
  }
  SetCountTimeMap (); //window sizeないの最初のhelloを受け取った時間と回数をマップに格納する関数
  SetEtxMap (); //m_rt と　EtX mapをセットする
  SetPriValueMap (GeocastCenterX, GeocastCenterX); //優先度を決める値をセットする関数
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
        candidate_node_id[1] = max_id;
        break;
      case 2:
        candidate_node_id[2] = max_id;
        break;
      case 3:
        candidate_node_id[3] = max_id;
        break;
      case 4:
        candidate_node_id[4] = max_id;
        break;
      case 5:
        candidate_node_id[5] = max_id;
        break;
      }
  }
  // 候補ノード選択アルゴリズム
    int candidataNum = 5; // 候補ノード数　初期値は最大

    for (int n = 1; n < 6; n++) //5回回して、 条件を満たすまでまわる nは候補ノード数
    {
      double infiniteProduct = 1; // 総乗
      for (int p = 1; p <= n; p++) //pは優先度  優先度は1から回す
        {
          double rtMiss = 1 - m_rt[candidate_node_id[p]]; // 伝送に失敗する予想確率
          infiniteProduct = infiniteProduct * rtMiss;
        }
      if (TransProbability <= (1 - infiniteProduct)) //選択アルゴリズムの条件を満たすならば
        {
          candidataNum = n; //候補ノード数を変更
          break;
        }
    }
    switch (candidataNum) //候補ノード数によってダミーノードIDを加える
      {
      case 1:
        candidate_node_id[2] = 10000000;
        candidate_node_id[3] = 10000000;
        candidate_node_id[4] = 10000000;
        candidate_node_id[5] = 10000000;
        break;
      case 2:
        candidate_node_id[3] = 10000000;
        candidate_node_id[4] = 10000000;
        candidate_node_id[5] = 10000000;
        break;
      case 3:
        candidate_node_id[4] = 10000000;
        candidate_node_id[5] = 10000000;
        break;
      case 4:
        candidate_node_id[5] = 10000000;
        break;
      }

    for (int i = 1; i < 6; i++)
      {
        if (candidate_node_id[i] != 10000000)
          {
            std::cout << "優先度" << i << "の node id = " << candidate_node_id[i] << "予想伝送確率"
                      << m_rt[candidate_node_id[i]] << "hello受信回数 " << m_recvcount[candidate_node_id[i]] << "\n";
          }
      }
}

void
RoutingProtocol::SendGeocast(int32_t pri_value, int32_t source_id, int32_t hopcount)
{
  for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j = m_socketAddresses.begin ();
      j != m_socketAddresses.end (); ++j)
  {
    if (hopcount > maxHop) // hop数が最大値を超えたらブレイク
      break;
    
    int32_t send_node_id = m_ipv4->GetObject<Node> ()->GetId (); //broadcastするノードID
    Ptr<MobilityModel> mobility = m_ipv4->GetObject<Node> ()->GetObject<MobilityModel> ();
    Vector mypos = mobility->GetPosition (); //broadcastするノードの位置情報

    int candidate_node_id[NumCandidateNodes + 1];
    std::cout << "send node id" << send_node_id << "decision relya candidate nodes\n";
    DecisionRelayCandidateNode(candidate_node_id);

    hopcount++;

    s_source_id.push_back (source_id); //source node の id
    s_source_x.push_back (mypos.x);    //send nodeのx座標
    s_source_y.push_back (mypos.y);    //send nodeのy座標
    s_time.push_back (Simulator::Now ().GetMicroSeconds ());
    s_hop.push_back (hopcount);
    s_pri_1_id.push_back (candidate_node_id[1]);
    s_pri_2_id.push_back (candidate_node_id[2]);
    s_pri_3_id.push_back (candidate_node_id[3]);
    s_pri_4_id.push_back (candidate_node_id[4]);
    s_pri_5_id.push_back (candidate_node_id[5]);
    s_pri_1_r.push_back (m_rt[candidate_node_id[1]]);
    s_pri_2_r.push_back (m_rt[candidate_node_id[2]]);
    s_pri_3_r.push_back (m_rt[candidate_node_id[3]]);
    s_pri_4_r.push_back (m_rt[candidate_node_id[4]]);
    s_pri_5_r.push_back (m_rt[candidate_node_id[5]]);
    s_des_id.push_back (10000000);
    s_send_log.push_back (0);
    m_send_check[source_id] = sendpacketCount;

    sendpacketCount++;
    m_recvcount.clear ();
    m_first_recv_time.clear ();
    m_etx.clear ();
    m_pri_value.clear ();
    m_rt.clear (); //保持している予想伝送確率をクリア

    Ptr<Socket> socket = j->first;
    Ipv4InterfaceAddress iface = j->second;
    Ptr<Packet> packet = Create<Packet> ();

    SendHeader sendHeader (source_id, GeocastCenterX, GeocastCenterY, 
                            send_node_id, mypos.x, mypos.y, 1,
                            candidate_node_id[1], candidate_node_id[2], 
                            candidate_node_id[3], candidate_node_id[4], candidate_node_id[5]);

    packet->AddHeader (sendHeader);

    TypeHeader tHeader (GLSGOTYPE_SEND);
    packet->AddHeader (tHeader);

    int32_t wait_time = (pri_value * WaitT) - WaitT; //待ち時間
    std::mt19937 rand_src (Grobal_Seed); //シード値
    std::uniform_int_distribution<int> wait_Jitter (100, 500);

    wait_time = wait_time + wait_Jitter(rand_src);
    m_wait[source_id] = hopcount; //今から待機するホップカウント

    Ipv4Address destination;
    if (iface.GetMask () == Ipv4Mask::GetOnes ())
    {
      destination = Ipv4Address ("255.255.255.255");
    }
    else
    {
      destination = iface.GetBroadcast ();
    }
    if (candidate_node_id[1] != 10000000) //中継候補ノードをⅠ個も持っていないノードはbrさせない
    {
      Simulator::Schedule (MicroSeconds (wait_time), &RoutingProtocol::SendToGlsgo, this,
                                socket, packet, destination, hopcount, source_id);
    }
  }
}

void
RoutingProtocol::Flooding(int32_t source_id, int32_t hopcount)
{
  for(std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j = m_socketAddresses.begin ();
    j != m_socketAddresses.end (); ++j)
  {
    if (hopcount > maxHop) // hop数が最大値を超えたらブレイク
      break;

    int32_t send_node_id = m_ipv4->GetObject<Node> ()->GetId (); //broadcastするノードID
    Ptr<MobilityModel> mobility = m_ipv4->GetObject<Node> ()->GetObject<MobilityModel> ();
    Vector mypos = mobility->GetPosition (); //broadcastするノードの位置情報

    hopcount++;
    
    Ptr<Socket> socket = j->first;
    Ipv4InterfaceAddress iface = j->second;
    Ptr<Packet> packet = Create<Packet> ();

    SendHeader sendHeader (source_id, GeocastCenterX, GeocastCenterY, 
                            send_node_id, mypos.x, mypos.y, hopcount,
                            10000000, 10000000, 10000000, 10000000, 10000000);

    packet->AddHeader (sendHeader);

    TypeHeader tHeader (GLSGOTYPE_SEND);
    packet->AddHeader (tHeader);

    s_source_id.push_back (send_node_id);
    s_source_x.push_back (mypos.x);
    s_source_y.push_back (mypos.y);
    s_time.push_back (Simulator::Now ().GetMicroSeconds ());
    s_hop.push_back (hopcount);
    s_pri_1_id.push_back (10000000);
    s_pri_2_id.push_back (10000000);
    s_pri_3_id.push_back (10000000);
    s_pri_4_id.push_back (10000000);
    s_pri_5_id.push_back (10000000);
    s_pri_1_r.push_back (10000000);
    s_pri_2_r.push_back (10000000);
    s_pri_3_r.push_back (10000000);
    s_pri_4_r.push_back (10000000);
    s_pri_5_r.push_back (10000000);
    s_des_id.push_back (10000000);
    s_send_log.push_back (0);
    m_send_check[source_id] = sendpacketCount;

    sendpacketCount++;

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
    Simulator::Schedule (Jitter, &RoutingProtocol::SendToFlooding, this, socket, packet,
                           destination);
  }
}

void
RoutingProtocol::RecvGlsgo (Ptr<Socket> socket)
{
  int32_t id = m_ipv4->GetObject<Node> ()->GetId ();
  Ptr<MobilityModel> mobility = m_ipv4->GetObject<Node> ()->GetObject<MobilityModel> ();
  Vector mypos = mobility->GetPosition ();
  Address sourceAddress;
  Ptr<Packet> packet = socket->RecvFrom (sourceAddress);

  TypeHeader tHeader (GLSGOTYPE_HELLO);
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
      case GLSGOTYPE_HELLO: { //hello message を受け取った場合

        HelloHeader helloheader;
        packet->RemoveHeader (helloheader); //近隣ノードからのhello packet
        int32_t recv_hello_id = helloheader.GetNodeId (); //NOde ID
        int32_t recv_hello_posx = helloheader.GetPosX (); //Node xposition
        int32_t recv_hello_posy = helloheader.GetPosY (); //Node yposition
        int32_t recv_hello_time = Simulator::Now ().GetMicroSeconds (); //
        SaveXpoint (recv_hello_id, recv_hello_posx);
        SaveYpoint (recv_hello_id, recv_hello_posy);
        SaveRecvTime (recv_hello_id, recv_hello_time);
        break; //breakがないとエラー起きる
      }
      case GLSGOTYPE_SEND: 
      {
        std::cout << "sendheader packet receive\n";
        SendHeader sendheader;
        packet->RemoveHeader (sendheader);

        int32_t source_id = sendheader.GetDesId ();  //geocast は この部分を source idに変更
        int32_t send_id = sendheader.GetSendId ();
        int32_t send_x = sendheader.GetSendPosX ();
        int32_t send_y = sendheader.GetSendPosY ();
        int32_t hopcount = sendheader.GetHopcount ();
        int32_t pri_id[] = {sendheader.GetId1 (), sendheader.GetId2 (), sendheader.GetId3 (),
                            sendheader.GetId4 (), sendheader.GetId5 ()};
        int32_t judge_multicast_region = 0;  // 1 = multicast regionに位置する
        
        // multicast region に位置するノードかどうかの判別
        for (auto itr = m_multicast_region_id.begin (); itr != m_multicast_region_id.end (); itr++)
        {
          if (itr->first == source_id) // 
          {
            if(itr->second == id) // multicast regionに登録されているIDが自分のIDと一致するか
            {
              judge_multicast_region = 1;
            }
          }
        }

        if(judge_multicast_region == 1) //multicast regionに位置するならならば
        {
          std::cout << "recv id " << id << "はmulticast regionに位置する\n";
          std::cout << "pos " << mypos << std::endl;
          if(m_recv_packet_id[source_id] == 1) //以前に同様のパケットを受信したことがある
          {
            std::cout << "以前に受け取ったことがある　パケット破棄します\n";
            break; //パケットを破棄
          }else{  //初めてパケットを受け取る
            //rebroadcast 単純
            std::cout << "単純Floodingをします\n";
            m_multicast_region_recv_id.insert(std::make_pair(source_id, id));
            Flooding(source_id, hopcount);
            
            p_source_id.push_back (source_id);
            p_send_x.push_back (send_x);
            p_send_y.push_back (send_y);
            p_recv_x.push_back (mypos.x);
            p_recv_y.push_back (mypos.y);
            p_recv_time.push_back (Simulator::Now ().GetMicroSeconds ());
            p_recv_priority.push_back (10000000);
            p_hopcount.push_back (hopcount);
            p_recv_id.push_back (id);
            p_send_id.push_back (send_id);
            p_destination_x.push_back (GeocastCenterX);
            p_destination_y.push_back (GeocastCenterY);
            p_pri_1.push_back (10000000);
            p_pri_2.push_back (10000000);
            p_pri_3.push_back (10000000);
            p_pri_4.push_back (10000000);
            p_pri_5.push_back (10000000);
            m_recv_packet_id[source_id] = 1; //受信したpacketを記録
            packetCount++;
          }
        }else
        { // multicast region外のノード
          if (m_wait[source_id] != 0) //待ち状態ならば
          {
            if (m_wait[source_id] >= hopcount)
            { //待機中のホップカウントより大きいホップカウントを受け取ったなら
              m_wait.erase (source_id);
            }
          }
          for (int i = 0; i < 5; i++) //relay candidate nodeに自分のIDが含まれているかチェック
            {
              if (id == pri_id[i]) //packetに自分のIDが含まれているか
                {
                  p_source_id.push_back (source_id);
                  p_send_x.push_back (send_x);
                  p_send_y.push_back (send_y);
                  p_recv_x.push_back (mypos.x);
                  p_recv_y.push_back (mypos.y);
                  p_recv_time.push_back (Simulator::Now ().GetMicroSeconds ());
                  p_recv_priority.push_back (i + 1);
                  p_hopcount.push_back (hopcount);
                  p_recv_id.push_back (id);
                  p_send_id.push_back (send_id);
                  p_destination_x.push_back (GeocastCenterX);
                  p_destination_y.push_back (GeocastCenterY);
                  p_pri_1.push_back (pri_id[0]);
                  p_pri_2.push_back (pri_id[1]);
                  p_pri_3.push_back (pri_id[2]);
                  p_pri_4.push_back (pri_id[3]);
                  p_pri_5.push_back (pri_id[4]);
                  packetCount++;
                  SendGeocast(i + 1, source_id, hopcount);
                  m_recv_packet_id[source_id] = 1; //受信したpacketを記録
                  break;
                }
            }
            //含まれていない (待状態　かつ　relay candidate nodeではない)
            // //send nodeがmulticast regionに存在するかどうか
            if (send_x >= DesLowX && send_x <= DesHighX && send_y >= DesLowY &&
              send_y <= DesHighY) // send nodeがmulticast regionに存在するならば　
              {
                if(m_recv_packet_id[source_id] == 1) //以前に同様のパケットを受信したことがある
                {
                  break; //パケットを破棄
                }else{
                  //rebroadcast 単純
                  Flooding(source_id, hopcount);
                  p_source_id.push_back (source_id);
                  p_send_x.push_back (send_x);
                  p_send_y.push_back (send_y);
                  p_recv_x.push_back (mypos.x);
                  p_recv_y.push_back (mypos.y);
                  p_recv_time.push_back (Simulator::Now ().GetMicroSeconds ());
                  p_recv_priority.push_back (10000000);
                  p_hopcount.push_back (hopcount);
                  p_recv_id.push_back (id);
                  p_send_id.push_back (send_id);
                  p_destination_x.push_back (GeocastCenterX);
                  p_destination_y.push_back (GeocastCenterY);
                  p_pri_1.push_back (pri_id[0]);
                  p_pri_2.push_back (pri_id[1]);
                  p_pri_3.push_back (pri_id[2]);
                  p_pri_4.push_back (pri_id[3]);
                  p_pri_5.push_back (pri_id[4]);
                  packetCount++;
                  m_recv_packet_id[source_id] = 1; //受信したpacketを記録
                  break;
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
void
RoutingProtocol::SetMyPos (void)
{
  int32_t id = m_ipv4->GetObject<Node> ()->GetId ();
  Ptr<MobilityModel> mobility = m_ipv4->GetObject<Node> ()->GetObject<MobilityModel> ();
  Vector mypos = mobility->GetPosition ();

  m_my_posx[id] = mypos.x;
  m_my_posy[id] = mypos.y;
}

// シミュレーション結果の出力関数
void
RoutingProtocol::SimulationResult (void) //
{
  std::cout << "time" << Simulator::Now ().GetSeconds () << "\n";
  //int32_t id = m_ipv4->GetObject<Node> ()->GetId ();
  
  if (Simulator::Now ().GetSeconds () == Grobal_StartTime + Grobal_SourceNodeNum + 1)
    {
      for(int source_list_index = 0; source_list_index < Grobal_SourceNodeNum; source_list_index ++)
      {
        int count = m_multicast_region_id.count(source_list[source_list_index]);
        int recv_count = m_multicast_region_recv_id.count(source_list[source_list_index]);
        std::cout << "source id " << source_list[source_list_index] << "のMRの個数 " << count << std::endl;
        std::cout << "multicast region 受信数" << recv_count << std::endl;
        double pdr = recv_count/count;
        std::cout << "pdr" << pdr << std::endl;
      }
      // int source_node_id = 0;
      // int region_count = 0;
      // for (auto itr = m_mlticast_region.begin (); itr != m_start_time.end (); itr++)
      // {

      // }

      std::string filename;
      std::string send_filename;

      if(Buildings == 1)
      {
        ////書き出し path
       std::string shadow_dir = "data/get_data/geocast/shadow" + std::to_string(Grobal_m_beta) + "_" + std::to_string (Grobal_m_gamma);
        std::cout<<"shadowing packet csv \n";
        filename = shadow_dir + "/glsgo/glsgo-seed_" + std::to_string (Grobal_Seed) + "nodenum_" +
                             std::to_string (numVehicle) + ".csv";
        send_filename = shadow_dir + "/send_glsgo/glsgo-seed_" + std::to_string (Grobal_Seed) + "nodenum_" +
                                  std::to_string (numVehicle) + ".csv";
        
        const char *dir = shadow_dir.c_str();
        struct stat statBuf;

        if (stat(dir, &statBuf) != 0) //directoryがなかったら
        {
          std::cout<<"ディレクトリが存在しないので作成します\n";
          mkdir(dir, S_IRWXU);
        }
        std::string s_glsgo_dir = shadow_dir + "/glsgo";
        std::string  s_send_glsgo_dir = shadow_dir + "/send_glsgo";
        const char * c_glsgo_dir = s_glsgo_dir.c_str();
        const char * c_send_glsgo_dir = s_send_glsgo_dir.c_str();
        if(stat(c_glsgo_dir, &statBuf) != 0)
        {
          mkdir(c_glsgo_dir, S_IRWXU);
          mkdir(c_send_glsgo_dir, S_IRWXU);
        }
      }
      else{
        std::cout<<"no_shadowing packet csv \n";
        filename = "data/no_buildings/glsgo/glsgo-seed_" + std::to_string (Grobal_Seed) + "nodenum_" +
                             std::to_string (numVehicle) + ".csv";
        send_filename = "data/no_buildings/send_glsgo/glsgo-seed_" + std::to_string (Grobal_Seed) + "nodenum_" +
                                  std::to_string (numVehicle) + ".csv";
      }

      std::ofstream packetTrajectory (filename);
      packetTrajectory << "source_id"
                       << ","
                       << "send_x"
                       << ","
                       << "send_y"
                       << ","
                       << "recv_x"
                       << ","
                       << "recv_y"
                       << ","
                       << "time"
                       << ","
                       << "recv_priority"
                       << ","
                       << "hopcount"
                       << ","
                       << "recv_id"
                       << ","
                       << "send_id"
                       << ","
                       << "destination_x"
                       << ","
                       << "destination_y"
                       << ","
                       << "pri_1"
                       << ","
                       << "pri_2"
                       << ","
                       << "pri_3"
                       << ","
                       << "pri_4"
                       << ","
                       << "pri_5" << std::endl;
      std::ofstream send_packetTrajectory (send_filename);
      send_packetTrajectory << "source_id"
                            << ","
                            << "source_x"
                            << ","
                            << "source_y"
                            << ","
                            << "time"
                            << ","
                            << "hop"
                            << ","
                            << "pri_1_id"
                            << ","
                            << "pri_2_id"
                            << ","
                            << "pri_3_id"
                            << ","
                            << "pri_4_id"
                            << ","
                            << "pri_5_id"
                            << ","
                            << "pri_1_r"
                            << ","
                            << "pri_2_r"
                            << ","
                            << "pri_3_r"
                            << ","
                            << "pri_4_r"
                            << ","
                            << "pri_5_r"
                            << ","
                            << "des_id"
                            << ","
                            << "send_log" << std::endl;
      for (int i = 0; i < packetCount; i++)
        {
          packetTrajectory << p_source_id[i] << ", " << p_send_x[i] << ", " << p_send_y[i] << ", " << p_recv_x[i] << ", "
                           << p_recv_y[i] << ", " << p_recv_time[i] << ", " << p_recv_priority[i]
                           << ", " << p_hopcount[i] << ", " << p_recv_id[i] << ", "
                           << p_send_id[i] << ", " << p_destination_x[i] << ", " << p_destination_y[i] << ", " << p_pri_1[i]
                           << ", " << p_pri_2[i] << ", " << p_pri_3[i] << ", " << p_pri_4[i] << ", "
                           << p_pri_5[i] << std::endl;
        }
      for (int i = 0; i < sendpacketCount; i++)
        {
          send_packetTrajectory << s_source_id[i] << ", " << s_source_x[i] << ", " << s_source_y[i]
                                << ", " << s_time[i] << ", " << s_hop[i] << ", " << s_pri_1_id[i]
                                << ", " << s_pri_2_id[i] << ", " << s_pri_3_id[i] << ", "
                                << s_pri_4_id[i] << ", " << s_pri_5_id[i] << ", " << s_pri_1_r[i]
                                << ", " << s_pri_2_r[i] << ", " << s_pri_3_r[i] << ", "
                                << s_pri_4_r[i] << ", " << s_pri_5_r[i] << ", " << s_des_id[i]
                                << ", " << s_send_log[i] << std::endl;
        }
    }
}

std::map<int, int> RoutingProtocol::broadcount; //key 0 value broudcast数
std::map<int, int> RoutingProtocol::m_start_time; //key destination_id value　送信時間
std::map<int, int> RoutingProtocol::m_finish_time; //key destination_id value 受信時間
std::map<int, double> RoutingProtocol::m_my_posx; // key node id value position x
std::map<int, double> RoutingProtocol::m_my_posy; // key node id value position y
std::vector<int> RoutingProtocol::source_list;
std::vector<int> RoutingProtocol::des_list;
std::multimap<int, int> RoutingProtocol::m_multicast_region_id;
std::multimap<int, int>RoutingProtocol::m_multicast_region_recv_id;

//パケット軌跡出力用の変数
//パケット軌跡出力用の変数
std::vector<int> RoutingProtocol::p_source_id;
std::vector<int> RoutingProtocol::p_send_x;
std::vector<int> RoutingProtocol::p_send_y;
std::vector<int> RoutingProtocol::p_recv_x;
std::vector<int> RoutingProtocol::p_recv_y;
std::vector<int> RoutingProtocol::p_recv_time;
std::vector<int> RoutingProtocol::p_recv_priority;
std::vector<int> RoutingProtocol::p_hopcount;
std::vector<int> RoutingProtocol::p_recv_id;
std::vector<int> RoutingProtocol::p_send_id;
std::vector<int> RoutingProtocol::p_destination_x;
std::vector<int> RoutingProtocol::p_destination_y;
std::vector<int> RoutingProtocol::p_pri_1;
std::vector<int> RoutingProtocol::p_pri_2;
std::vector<int> RoutingProtocol::p_pri_3;
std::vector<int> RoutingProtocol::p_pri_4;
std::vector<int> RoutingProtocol::p_pri_5;
//送信側ログ用変数
std::vector<int> RoutingProtocol::s_source_id;
std::vector<int> RoutingProtocol::s_source_x;
std::vector<int> RoutingProtocol::s_source_y;
std::vector<int> RoutingProtocol::s_time;
std::vector<int> RoutingProtocol::s_hop;
std::vector<int> RoutingProtocol::s_pri_1_id;
std::vector<int> RoutingProtocol::s_pri_2_id;
std::vector<int> RoutingProtocol::s_pri_3_id;
std::vector<int> RoutingProtocol::s_pri_4_id;
std::vector<int> RoutingProtocol::s_pri_5_id;
std::vector<double> RoutingProtocol::s_pri_1_r;
std::vector<double> RoutingProtocol::s_pri_2_r;
std::vector<double> RoutingProtocol::s_pri_3_r;
std::vector<double> RoutingProtocol::s_pri_4_r;
std::vector<double> RoutingProtocol::s_pri_5_r;
std::vector<int> RoutingProtocol::s_des_id;
std::vector<int> RoutingProtocol::s_send_log;

} // namespace glsgo
} // namespace ns3
