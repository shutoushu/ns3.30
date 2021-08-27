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

#include "jbr-routing-protocol.h"
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
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <random>

#include "ns3/mobility-module.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("JbrRoutingProtocol");

namespace jbr {
NS_OBJECT_ENSURE_REGISTERED (RoutingProtocol);

/// UDP Port for JBR control traffic
const uint32_t RoutingProtocol::JBR_PORT = 654;
int posx = 0; //送信者の位置情報
int posy = 0;
int maxLenge = 0; // 受信者との距離マックス

RoutingProtocol::RoutingProtocol ()
{
}

TypeId
RoutingProtocol::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("ns3::jbr::RoutingProtocol")
          .SetParent<Ipv4RoutingProtocol> ()
          .SetGroupName ("Jbr")
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
                        << ", JBR Routing table" << std::endl;

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

  //std::cout << "Route Ouput Node: " << m_ipv4->GetObject<Node> ()->GetId () << "\n";
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
  socket->SetRecvCallback (MakeCallback (&RoutingProtocol::RecvJbr, this));
  socket->BindToNetDevice (l3->GetNetDevice (i));
  socket->Bind (InetSocketAddress (iface.GetLocal (), JBR_PORT));
  socket->SetAllowBroadcast (true);
  socket->SetIpRecvTtl (true);
  m_socketAddresses.insert (std::make_pair (socket, iface));

  // create also a subnet broadcast socket
  socket = Socket::CreateSocket (GetObject<Node> (), UdpSocketFactory::GetTypeId ());
  NS_ASSERT (socket != 0);
  socket->SetRecvCallback (MakeCallback (&RoutingProtocol::RecvJbr, this));
  socket->BindToNetDevice (l3->GetNetDevice (i));
  socket->Bind (InetSocketAddress (iface.GetBroadcast (), JBR_PORT));
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
              InetSocketAddress inetAddr (m_ipv4->GetAddress (i, 0).GetLocal (), JBR_PORT);
              if (socket->Bind (inetAddr))
                {
                  NS_FATAL_ERROR ("Failed to bind() ZEAL socket");
                }
              socket->BindToNetDevice (m_ipv4->GetNetDevice (i));
              socket->SetAllowBroadcast (true);
              socket->SetRecvCallback (MakeCallback (&RoutingProtocol::RecvJbr, this));
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
  Ptr<MobilityModel> mobility = m_ipv4->GetObject<Node> ()->GetObject<MobilityModel> ();
  Vector pos = mobility->GetPosition ();


  int32_t id = m_ipv4->GetObject<Node> ()->GetId ();
  std::cout<<"--topology--  id " << id << "x_pos " << pos.x << "y_pos" << pos.y << "\n";

  for (int i = 1; i < 100; i++)
    {
      Simulator::Schedule (Seconds (i), &RoutingProtocol::SendHelloPacket, this);
      if (id == 74)
        {
          // Simulator::Schedule (Seconds (i), &RoutingProtocol::SendHelloPacket, this);
          Simulator::Schedule (Seconds (i), &RoutingProtocol::SimulationResult,
                               this); //結果出力関数
        }
    }
  if (id == 0)
        {
          ReadSumoFile ();
        }
  if (id == 75)
      {
        Simulator::Schedule (Seconds (3.4), &RoutingProtocol::CallSendXUnicast, this);
      }
}

void
RoutingProtocol::RecvJbr (Ptr<Socket> socket)
{
  int32_t id = m_ipv4->GetObject<Node> ()->GetId ();
  Ptr<MobilityModel> mobility = m_ipv4->GetObject<Node> ()->GetObject<MobilityModel> ();
  Vector mypos = mobility->GetPosition ();

  Address sourceAddress;
  Ptr<Packet> packet = socket->RecvFrom (sourceAddress);
  TypeHeader tHeader (JBRTYPE_HELLO);
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
      case JBRTYPE_HELLO: { //hello message を受け取った場合
        // std::cout << "hello packet receive\n";
        HelloHeader helloheader;
        packet->RemoveHeader (helloheader); //近隣ノードからのhello packet
        int32_t recv_hello_id = helloheader.GetNodeId (); //NOde ID
        int32_t recv_hello_posx = helloheader.GetPosX (); //Node xposition
        int32_t recv_hello_posy = helloheader.GetPosY (); //Node yposition
        int32_t recv_hello_time = Simulator::Now ().GetMicroSeconds ();
        // std::cout << "id " << recv_hello_id << " x " << recv_hello_posx << " y "
        //  << recv_hello_posy << "\n";
         SaveXpoint (recv_hello_id, recv_hello_posx);
         SaveYpoint (recv_hello_id, recv_hello_posy);
         SaveRecvTime (recv_hello_id, recv_hello_time);
        break; //breakがないとエラー起きる
      }
      case JBRTYPE_RECOVER: {
        JbrHeader jbrheader;
        packet->RemoveHeader (jbrheader);
        int32_t send_x = jbrheader.GetSendX ();
        int32_t send_y = jbrheader.GetSendY ();
        int32_t local_source_x = jbrheader.GetLocalSourceX ();
        int32_t local_source_y = jbrheader.GetLocalSourceY ();
        int32_t previous_x = jbrheader.GetPreviousX (); 
        int32_t previous_y = jbrheader.GetPreviousY (); 
        int32_t next_id = jbrheader.GetNextId ();
        int32_t des_id = jbrheader.GetDesId (); 
        int32_t des_x = jbrheader.GetDesX ();
        int32_t des_y = jbrheader.GetDesY (); 
        int32_t hop = jbrheader.GetHop ();

        if (id == next_id) //receive nodeがnext hop nodeとして指定されていたら　
        {
          int32_t local_source_distance = getDistance (local_source_x, local_source_y, des_x, des_y);
          int32_t current_distance = getDistance (mypos.x, mypos.y, des_x, des_y);

          if (local_source_distance > current_distance) //local optimum revovery
          {
            std::cout << "\nlocal optimum's recovery successful!  node id " << id << "\n";
            std::cout << "local source node x y " << local_source_x << " , "<< local_source_y << 
            "current distance to destination" << current_distance << "\n";
          }
          else { //no recovery
            if (maxHop < hop)
            {
              std::cout << "\n*************reach max hop************* \n";
              break;
            }
            SendXUnicast(send_x, send_y, local_source_x, local_source_y, previous_x, previous_y,
            des_id, des_x, des_y, hop);
            std::cout << "local optimum's recovery continu\n";
          }
        }
        break;
      }
    default:
      std::cout << "unknown_type\n";
    }
}

void
RoutingProtocol::CallSendXUnicast (void)
{
  int32_t one_before_x = 550;
  int32_t one_before_y = 200; 
  int32_t local_source_x = 500;
  int32_t local_source_y = 200;
  int32_t previous_x = 550; 
  int32_t previous_y = 200; 
  int32_t des_id = 175; 
  int32_t des_x = 500;
  int32_t des_y = 600; 
  int32_t hop = 0;
  SendXUnicast(one_before_x, one_before_y, local_source_x, local_source_y, 
  previous_x, previous_y, des_id, des_x, des_y, hop);
}

void
RoutingProtocol::SendXUnicast (int32_t one_before_x, int32_t one_before_y, int32_t local_source_x, 
int32_t local_source_y, int32_t previous_x, int32_t previous_y, int32_t des_id, 
int32_t des_x, int32_t des_y, int32_t hop)
{
  for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j = m_socketAddresses.begin ();
       j != m_socketAddresses.end (); ++j)
    {
      int32_t id = m_ipv4->GetObject<Node> ()->GetId ();

      Ptr<MobilityModel> mobility = m_ipv4->GetObject<Node> ()->GetObject<MobilityModel> ();
      Vector mypos = mobility->GetPosition ();
      posx = mypos.x;
      posy = mypos.y;
      
      Ptr<Socket> socket = j->first;
      Ipv4InterfaceAddress iface = j->second;
      Ptr<Packet> packet = Create<Packet> ();

      hop++;
      int32_t next_id = DecisionNextId (one_before_x, one_before_y, local_source_x, local_source_y, 
      previous_x, previous_y,des_x, des_y);


      JbrHeader JbrHeader (id, mypos.x, mypos.y, next_id, local_source_x, local_source_y,
      previous_x, previous_y, des_id, des_x, des_y, hop);
      packet->AddHeader (JbrHeader);

      TypeHeader tHeader (JBRTYPE_RECOVER);
      packet->AddHeader (tHeader);

      // Send to all-hosts broadcast if on /32 addr, subnet-directed otherwise
      Ipv4Address destination;
      if (iface.GetMask () == Ipv4Mask::GetOnes ())
        {
          destination = Ipv4Address ("255.255.255.255");
        }
      else  //絶対こっちに入る
        {
          destination = iface.GetBroadcast ();
        }
      if (next_id != 10000)
      {
        std::cout << "\n\nid" << id << "recovery"
                << "x=" << posx << "y=" << posy << "recovery unicast  time" << Simulator::Now ().GetSeconds ()
                << "\n";
        std::cout << "next id " << next_id << "\n";
        socket->SendTo (packet, 0, InetSocketAddress (destination, JBR_PORT));
      }else{
        std::cout << "\n\n\n\n\n\nrecovery strategy no next hop node | current id" << id <<std::endl;
      }
      
    }
}

int 
RoutingProtocol::DecisionNextId (int32_t one_before_x, int32_t one_before_y, int32_t local_source_x, 
  int32_t local_source_y, int32_t previous_x, int32_t previous_y, int32_t des_x, int32_t des_y)
{
  int existence_intersection = 1; //0 = intersection nodeあり 1 = なし　
  std::cout << "**********id "<<m_ipv4->GetObject<Node> ()->GetId () <<"の近隣テーブル**********\n";
  Ptr<MobilityModel> mobility = m_ipv4->GetObject<Node> ()->GetObject<MobilityModel> ();
  Vector current_pos = mobility->GetPosition ();
  int closest_inter_id = 10000; //junctino node の中で最も宛先に近いノード
  int farthest_simple_id = 10000;  // simple nodeの中で最もcurrent nodeから離れたノード
  int minangle_id = 10000;
  double closest_inter_to_des = 10000; // 最小のintersection nodeとdestinatino nodeの距離を格納
  double farthest_current_des = 0; // current node から最も離れているsimple nodeの距離を格納
  double minimum_angle = 360; //minimum angle method 最小の角度を格納
  std::string current_road_id = distinctionRoad (current_pos.x, current_pos.y);

  for (auto itr = m_xpoint.begin (); itr != m_xpoint.end (); itr++) 
  {
    if (judgeIntersection(itr->second, m_ypoint[itr->first]) == 0) //近隣ノードがintersection node
    {
      int dif_last_recv = Simulator::Now ().GetMicroSeconds () - m_last_recv_time[itr->first];
      if (dif_last_recv < 2200000) //直近２秒でhello packetを受信していたら
      {
        std::cout << "junction neighbor id " << itr->first <<  "\n";
        //jbr 条件 1
        double mndis = getDistance (current_pos.x, current_pos.y, 
        m_xpoint[itr->first], m_ypoint[itr->first]); //current to potential 
        double cldis = getDistance (one_before_x, one_before_y, 
        current_pos.x, current_pos.y); // current to one_before
        double nldis = getDistance (m_xpoint[itr->first], m_ypoint[itr->first], 
        one_before_x, one_before_y); // onebefore to potential
        std::cout << "\n nldis " << nldis << "cldis" << cldis << "mndis" << mndis << "\n";
        if(nldis > cldis && nldis > mndis) //jbr  potential nodeの条件
        {
          if(judgeIntersection(current_pos.x, current_pos.y) == 0) //current nodeがcoordinator
          {
            if(distinctionRoad(current_pos.x, current_pos.y) != 
            distinctionRoad(m_xpoint[itr->first], m_ypoint[itr->first])) //同じintersection　じゃなかったら
            {
              if (getDistance (m_xpoint[itr->first], m_ypoint[itr->first], des_x, des_y) < closest_inter_to_des)
              {
                //intersection の中で宛先に近いものを更新していく
                closest_inter_to_des = getDistance (m_xpoint[itr->first], m_ypoint[itr->first], des_x, des_y);
                closest_inter_id = itr->first;
              }
              existence_intersection = 0;
            }else{
              std::cout << "neighbor id" << itr->first << "は 同じ intersection にいる\n"; 
            }
          }else{ //current node no coordinator
            std::cout << " current node is no coordinator \n";
            if (getDistance (m_xpoint[itr->first], m_ypoint[itr->first], des_x, des_y) < closest_inter_to_des)
            {
              //intersection の中で宛先に近いものを更新していく
              closest_inter_to_des = getDistance (m_xpoint[itr->first], m_ypoint[itr->first], des_x, des_y);
              closest_inter_id = itr->first;
              std::cout << "今の所 id " << itr->first << "が一番近い\n";
            }
            existence_intersection = 0;
          }
        }
      }
    }else{ //近隣ノードがsimple nodeのみ
      std::cout << "simple neighbor id " << itr->first <<  "\n";
      if(judgeIntersection(current_pos.x, current_pos.y) == 0) //current nodeがcoordinator
      {
        //minimum angle method
        double sdangle = getAngle (des_x, des_y, local_source_x, local_source_y,
        previous_x, previous_y);
        double snangle = getAngle (m_xpoint[itr->first], m_ypoint[itr->first], local_source_x, 
        local_source_y, previous_x, previous_y);
        int sdangle_direction = RotationDirection (des_x, des_y, local_source_x, 
        local_source_y, previous_x, previous_y);
        int snangle_direction = RotationDirection (m_xpoint[itr->first], m_ypoint[itr->first], 
        local_source_x, local_source_y, previous_x, previous_y);

        std::cout << "snangle direction" << snangle_direction << "sdangle direction " << sdangle_direction << "\n";

        //符号判定 同じ符号でなければ　角度の回転方向が違えば
        if (signbit(sdangle_direction) != signbit(snangle_direction))
        {
          std::cout << "\n\n id" << itr->first << "角度修正 snagle before" << snangle;
          snangle = 360 - snangle;
          std::cout << "after" << snangle << std::endl;
        }

        double minangle = sdangle - snangle;

        //minangle は絶対値の差
        if (minangle < 0)
        {
          minangle = - minangle;
        }

        std::cout << "neighbor id " << itr->first << "のminangle" << minangle << "\n";
        std::cout << "snangle" << snangle << "sdangle" << sdangle << "\n";
        if(minangle < minimum_angle)
        {
          minimum_angle = minangle;
          minangle_id = itr->first;
        }
      }else{
        //current nodeから最も離れたノード
        std::string neighbor_road_id = distinctionRoad (m_xpoint[itr->first], m_ypoint[itr->first]);
        int link_judge = LinkRoad (current_road_id ,neighbor_road_id); //linkもってたら1
        if (link_judge == 1) // neighbor が linkを持つことのができるroadだったら
        {
          //jbr 条件 1
          double mndis = getDistance (current_pos.x, current_pos.y, 
          m_xpoint[itr->first], m_ypoint[itr->first]); //current to potential 
          double cldis = getDistance (one_before_x, one_before_y, 
          current_pos.x, current_pos.y); // current to one_before
          double nldis = getDistance (m_xpoint[itr->first], m_ypoint[itr->first], 
          one_before_x, one_before_y); // onebefore to potential
          if(nldis > cldis && nldis > mndis) //jbr  potential nodeの条件
          {
            if(farthest_current_des < mndis)
            {
              farthest_current_des = mndis;
              farthest_simple_id = itr->first;
            }
          }
        }
      }
    }
  }
  if(existence_intersection == 0)
  {
    std::cout << "交差点ノードが存在\n";
    std::cout << "closest intersection node id is " << closest_inter_id << "\n";
    return closest_inter_id;
  }else {
    std::cout << "交差点ノードが存在しない\n";
    if(judgeIntersection(current_pos.x, current_pos.y) == 0) //current node = coordinator
    {
      std::cout << "minimum angle  node id is " << minangle_id << "\n";
      return minangle_id;
    }else{
      std::cout << "farthest simple node id is " << farthest_simple_id << "\n";
      return farthest_simple_id;
    }
  }
  return 0;
}

int
RoutingProtocol::LinkRoad (std::string current_road_id, std::string neighbor_road_id)
{
  //current road agnle
  std::string current_from_to = m_road_from_to[current_road_id];
  replace(current_from_to.begin(), current_from_to.end(), '_', ' ');
  std::istringstream iss(current_from_to);
  std::string current_from, current_to;
  iss >> current_from >> current_to ;  //road = junction from  〜 junction to
  double current_angle = getTwoPointAngle(m_junction_x[current_from], m_junction_y[current_from],
  m_junction_x[current_to], m_junction_y[current_to]);

  //neighbor road angle
  std::string neighbor_from_to = m_road_from_to[neighbor_road_id];
  replace(neighbor_from_to.begin(), neighbor_from_to.end(), '_', ' ');
  std::istringstream iss2(neighbor_from_to);
  std::string neighbor_from, neighbor_to;
  iss2 >> neighbor_from >> neighbor_to ;  //road = junction from  〜 junction to
  double neighbor_angle = getTwoPointAngle(m_junction_x[neighbor_from], m_junction_y[neighbor_from],
  m_junction_x[neighbor_to], m_junction_y[neighbor_to]);

  double dif_angle = current_angle - neighbor_angle;
  if (dif_angle < 0)
  {
    dif_angle = -dif_angle;
  }

  if (dif_angle < 10) //road同士のangle の差がほぼなければリンクを持つことができる
  {
    return 1;
  }else{
    return 0;
  }
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

double 
RoutingProtocol::getTwoPointAngle (double x, double y, double x2, double y2)
{
  double radian = atan2(y2 - y,x2 - x);
  double angle = radian * 180 / 3.14159265; //ラジアンから角度に変換
  return angle;
}

int
RoutingProtocol::RotationDirection (double a_x, double a_y, double b_x, double b_y, double c_x,
                   double c_y)
{
  // ※ A = potential node or destiantino node   B = local source node C= previous node
  double BA_x = a_x - b_x; //ベクトルAのｘ座標
  double BA_y = a_y - b_y; //ベクトルAのy座標
  double BC_x = c_x - b_x; //ベクトルCのｘ座標
  double BC_y = c_y - b_y; //ベクトルCのy座標

  int direction = BC_x * BA_y - BA_x * BC_y; 
  std::cout << "rotation direction check direction = "  << direction <<std::endl;
  return direction;
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

      TypeHeader tHeader (JBRTYPE_HELLO);
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
      Simulator::Schedule (Jitter, &RoutingProtocol::SendToHello, this, socket, packet,
                           destination);
    }
}

void
RoutingProtocol::SendToHello (Ptr<Socket> socket, Ptr<Packet> packet, Ipv4Address destination)
{
  socket->SendTo (packet, 0, InetSocketAddress (destination, JBR_PORT));
}

std::string
RoutingProtocol::distinctionRoad (int x_point, int y_point)
{
  int interRange = 20; //交差点の半径
  for (auto itr = m_junction_x.begin (); itr != m_junction_x.end (); itr++)
    {
      double distance = getDistance(x_point, y_point, itr->second, m_junction_y[itr->first]);
      if (distance <= interRange) //交差点の内部の座標だったら
      {
        return itr->first; // junction(intersection) id
      }
    }
    std::string separator = "_"; //区切り文字指定
    std::string from_to;
    double min_distance = 100.0; //数値は仮おき　最小距離の道路にノードは存在する
    std::string road_id;

    for (auto itr = m_road_from_to.begin (); itr != m_road_from_to.end (); itr++)
    {
      from_to = itr->second;
      replace(from_to.begin(), from_to.end(), '_', ' ');
      std::istringstream iss(from_to);

      std::string from, to;
      iss >> from >> to ;  //road = junction from  〜 junction to

      double distance = lineDistance(m_junction_x[from], m_junction_y[from], m_junction_x[to], m_junction_y[to], 
            x_point, y_point);//線分と座標の距離
      
      if(min_distance > distance)
      {
        min_distance = distance;
        road_id = itr->first;
      }
    }
  return road_id; //roadのIDを返す
}

int
RoutingProtocol::judgeIntersection (int x_point, int y_point)
{
  int interRange = 20; //交差点の半径
  for (auto itr = m_junction_x.begin (); itr != m_junction_x.end (); itr++)
    {
      double distance = getDistance(x_point, y_point, itr->second, m_junction_y[itr->first]);
      if (distance <= interRange) //交差点の内部の座標だったら
      {
        return 0; // on intersection
      }
    }
    return 1; // no intersection node
}

double
RoutingProtocol::getDistance (double x, double y, double x2, double y2)
{
  double distance = std::sqrt ((x2 - x) * (x2 - x) + (y2 - y) * (y2 - y));

  return distance;
}

double
RoutingProtocol::lineDistance(double line_x1, double line_y1, double line_x2, double line_y2, 
  double dot_x, double dot_y)
{
  double a,b,c; // ax + by + c = 0
  double root;
  double distance; //求める距離

  a = line_y1 - line_y2;
  b = line_x2 - line_x1;
  c = (-b * line_y1) + (-a * line_x1);
  root = sqrt(a*a + b*b);

  //std::cout << "a " << a << " b " << b << " c " << c << std::endl;

  if (root == 0.0) {
    std::cout << " root が求められません" << std::endl;
  }

  distance = ((a * dot_x) + (b * dot_y) + c) / root;

  if (distance < 0.0)
  {
    distance  = -distance;
  }

  return distance;
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
  //int32_t id = m_ipv4->GetObject<Node> ()->GetId ();
  m_recvtime.insert (std::make_pair (map_id, map_recvtime));
  m_last_recv_time[map_id] = map_recvtime;
}


//sumoからnet fileを読み込む
int
RoutingProtocol::ReadSumoFile (void)
{
  //std::ifstream ifs("../../../sumo/tools/no_signal/200/original_net.xml");
  std::ifstream ifs("sumo/tools/no_signal/no_signal_netfile");
  std::string str;

  if (ifs.fail()) {
      std::cerr << "Failed to open file." << std::endl;
      return -1;
  }
  while (getline(ifs, str)) { //1行ずつstrに格納
      std::cout<<"---------------------input road segment -----------------------------\n";
      std::cout << "#" << str << std::endl;
      std::istringstream iss(str);
      std::string split;

      std::vector<std::string> v_road;
      while (iss >> split) {
          v_road.push_back(split);
      }
      if(v_road[0] == "junction") //junction
      {
        std::cout<<"junction id" << v_road[1] <<"\n";
        std::cout<<"junction x" << v_road[2] <<"\n";
        std::cout<<"junction y" << v_road[3] <<"\n";
        m_junction_x[v_road[1]] = stod(v_road[2]);
        m_junction_y[v_road[1]] = stod(v_road[3]);
        std::cout<<"junction x test " << m_junction_x[v_road[1]] <<"\n";
      }
      else{ //road segment
        std::cout<<"road id" << v_road[1] <<"\n";
        std::cout<<"road segment from to" << v_road[2] <<"\n";
        m_road_from_to[v_road[1]] = v_road[2];
        std::cout << " road from to test " <<  m_road_from_to[v_road[1]] << std::endl;
      }
  }
  std::cout<<"---------------------input road segment -----------------------------\n\n\n\n\n";
  return 0;
}

void
RoutingProtocol::SimulationResult (void) //
{
  if (Simulator::Now ().GetSeconds () == 16)
    {
      for (auto itr = recvCount.begin (); itr != recvCount.end (); itr++)
        {
          std::cout << "recv hello packet id = " << itr->first // キーを表示
                    << "取得数 " << itr->second << "\n"; // 値を表示
        }
      std::cout << "recv max lenge " << maxLenge << "\n";
      std::cout << "m_beta" << 9.0 << "\n";
      std::cout << "m_beta" << 30.2 << "\n";
    }
}

std::map<int, int> RoutingProtocol::recvCount;
std::map<std::string, double> RoutingProtocol::m_junction_x; // key junction id value xposition
std::map<std::string, double> RoutingProtocol::m_junction_y; // key junction id value yposition
std::map<std::string, std::string> RoutingProtocol::m_road_from_to; // key road id value junction from to

} //namespace jbr
} //namespace ns3
