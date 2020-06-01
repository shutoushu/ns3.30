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
#define NS_LOG_APPEND_CONTEXT                                   \
  if (m_ipv4) { std::clog << "[node " << m_ipv4->GetObject<Node> ()->GetId () << "] "; }

#include "sample-routing-protocol.h"
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

#include "ns3/mobility-module.h"




namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SampleRoutingProtocol");

namespace sample {
NS_OBJECT_ENSURE_REGISTERED (RoutingProtocol);

/// UDP Port for SAMPLE control traffic
const uint32_t RoutingProtocol::SAMPLE_PORT = 654;

//グローバル変数の宣言
std::vector<int> RoutingProtocol::recv_num;





RoutingProtocol::RoutingProtocol ()
{
  
}

TypeId
RoutingProtocol::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::sample::RoutingProtocol")
    .SetParent<Ipv4RoutingProtocol> ()
    .SetGroupName ("Sample")
    .AddConstructor<RoutingProtocol> ()
    .AddAttribute ("UniformRv",
                   "Access to the underlying UniformRandomVariable",
                   StringValue ("ns3::UniformRandomVariable"),
                   MakePointerAccessor (&RoutingProtocol::m_uniformRandomVariable),
                   MakePointerChecker<UniformRandomVariable> ())
  ;
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
                        << ", SAMPLE Routing table" << std::endl;

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
RoutingProtocol::RouteOutput (Ptr<Packet> p, const Ipv4Header &header,
                              Ptr<NetDevice> oif, Socket::SocketErrno &sockerr)
{

  //std::cout<<"Route Ouput Node: "<<m_ipv4->GetObject<Node> ()->GetId ()<<"\n";
  Ptr<Ipv4Route> route;

  if (!p)
    {
	  std::cout << "loopback occured! in routeoutput";
	  return route;// LoopbackRoute (header,oif);

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
                             MulticastForwardCallback mcb, LocalDeliverCallback lcb, ErrorCallback ecb)
{
 
  std::cout<<"Route Input Node: "<<m_ipv4->GetObject<Node> ()->GetId ()<<"\n";
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
  NS_LOG_FUNCTION (this << m_ipv4->GetAddress (i, 0).GetLocal ()
                        << " interface is up");
  Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
  Ipv4InterfaceAddress iface = l3->GetAddress (i,0);
  if (iface.GetLocal () == Ipv4Address ("127.0.0.1"))
    {
      return;
    }
  // Create a socket to listen only on this interface
  Ptr<Socket> socket;

  socket = Socket::CreateSocket (GetObject<Node> (),UdpSocketFactory::GetTypeId ());
  NS_ASSERT (socket != 0);
  socket->SetRecvCallback (MakeCallback (&RoutingProtocol::RecvSample,this));
  socket->BindToNetDevice (l3->GetNetDevice (i));
  socket->Bind (InetSocketAddress (iface.GetLocal (), SAMPLE_PORT));
  socket->SetAllowBroadcast (true);
  socket->SetIpRecvTtl (true);
  m_socketAddresses.insert (std::make_pair (socket,iface));


    // create also a subnet broadcast socket
  socket = Socket::CreateSocket (GetObject<Node> (),
                                 UdpSocketFactory::GetTypeId ());
  NS_ASSERT (socket != 0);
  socket->SetRecvCallback (MakeCallback (&RoutingProtocol::RecvSample, this));
  socket->BindToNetDevice (l3->GetNetDevice (i));
  socket->Bind (InetSocketAddress (iface.GetBroadcast (), SAMPLE_PORT));
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
              InetSocketAddress inetAddr (m_ipv4->GetAddress (i, 0).GetLocal (), SAMPLE_PORT);
              if (socket->Bind (inetAddr))
                {
                  NS_FATAL_ERROR ("Failed to bind() ZEAL socket");
                }
              socket->BindToNetDevice (m_ipv4->GetNetDevice (i));
              socket->SetAllowBroadcast (true);
              socket->SetRecvCallback (MakeCallback (&RoutingProtocol::RecvSample, this));
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
  int32_t id =m_ipv4->GetObject<Node> ()->GetId ();
   Ptr<MobilityModel> mobility = m_ipv4->GetObject<Node> () ->GetObject<MobilityModel>();
       Vector mypos = mobility->GetPosition ();
  if(id == 0 ){
       std::cout<<"M0de Position----------------------------------------------------------"<<"\n";
  }

   std::cout<<"|node id|"<<id<<"|xposition|"<<mypos.x<<"|yposition"<<mypos.y<<"\n";
   if(id == NumNodes - 1){
    std::cout<<"-----------------------------------------------------------------------"<<"\n";
  }
  if(id == 1)   //if Node ID = 0
    {
      for(int i=0; i<NumNodes; i++)
     {
      recv_num.push_back(0);
     }
      for(int i=0; i<SimTime; i++){//シミュレーション時間分ループを回す　単位 second
        //std::cout<<"nodeid="<<id<<"  broadcast will be send\n";
        //SendXBroadcast();
        Simulator::Schedule(Seconds(i), &RoutingProtocol::SendXBroadcast, this);
      }
    }
    
    if(id == 2 || id == 5 || id == 8 || id == 10 || id == 15 || id == 20 || id == 25 || id == 30)
    {
      for(int i=0; i<SimTime; i++){//シミュレーション時間分ループを回す　単位 second
        Simulator::Schedule(Seconds(i), &RoutingProtocol::SendXBroadcast, this);
      }
    }

    Simulator::Schedule(Seconds(SimTime-1), &RoutingProtocol::SimulationResult, this);
}

void
RoutingProtocol::RecvSample (Ptr<Socket> socket)
{
  int32_t id =m_ipv4->GetObject<Node> ()->GetId ();
  recv_num[id]++;
  //
  
  std::cout<<"receive id"<<id<<"\n";
  //std::cout<<"In recv Sample(Node "<< m_ipv4->GetObject<Node> ()->GetId ()<<")\n";
  //int8_t id =m_ipv4->GetObject<Node> ()->GetId ();
    //if(id == 3 || id == 4 id ==5 )
    //SendXBroadcast();
}


void
RoutingProtocol::SendXBroadcast (void)
{
  //int32_t id =m_ipv4->GetObject<Node> ()->GetId ();
 for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j = m_socketAddresses.begin (); j
       != m_socketAddresses.end (); ++j)
    {

      Ptr<Socket> socket = j->first;
      Ipv4InterfaceAddress iface = j->second;
      Ptr<Packet> packet = Create<Packet> ();

      RrepHeader rrepHeader(0,3,Ipv4Address("10.1.1.15"),5,Ipv4Address("10.1.1.13"),Seconds(3));
      packet->AddHeader (rrepHeader);
      
      TypeHeader tHeader (SAMPLETYPE_RREP);
      packet->AddHeader (tHeader);

      //std::cout<<"time"<<Simulator::Now().GetSeconds()<<"\n";
       
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
      socket->SendTo (packet, 0, InetSocketAddress (destination, SAMPLE_PORT));
      //std::cout<<"broadcast sent\n"; 
        
     }
}
// シミュレーション結果の出力関数
void
RoutingProtocol::SimulationResult(void)//
{
  //int recv_num[NumNodes];//パケットを受け取った回数
  int32_t id =m_ipv4->GetObject<Node> ()->GetId ();
  //for(int i=0; i<NumNodes; i++)
  //{
    std::cout<<"node "<<id<< "  パケット受信数 "<<recv_num[id]<<"\n";
  //}

}




} //namespace sample
} //namespace ns3
