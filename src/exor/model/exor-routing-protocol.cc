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

#include "exor-routing-protocol.h"
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

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ExorRoutingProtocol");

namespace exor {
NS_OBJECT_ENSURE_REGISTERED (RoutingProtocol);

/// UDP Port for EXOR control traffic
const uint32_t RoutingProtocol::EXOR_PORT = 654;


RoutingProtocol::RoutingProtocol ()
{
  
}

TypeId
RoutingProtocol::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::exor::RoutingProtocol")
    .SetParent<Ipv4RoutingProtocol> ()
    .SetGroupName ("Exor")
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
                        << ", EXOR Routing table" << std::endl;

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

  std::cout<<"Route Ouput Node: "<<m_ipv4->GetObject<Node> ()->GetId ()<<"\n";
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
  socket->SetRecvCallback (MakeCallback (&RoutingProtocol::RecvExor,this));
  socket->BindToNetDevice (l3->GetNetDevice (i));
  socket->Bind (InetSocketAddress (iface.GetLocal (), EXOR_PORT));
  socket->SetAllowBroadcast (true);
  socket->SetIpRecvTtl (true);
  m_socketAddresses.insert (std::make_pair (socket,iface));


    // create also a subnet broadcast socket
  socket = Socket::CreateSocket (GetObject<Node> (),
                                 UdpSocketFactory::GetTypeId ());
  NS_ASSERT (socket != 0);
  socket->SetRecvCallback (MakeCallback (&RoutingProtocol::RecvExor, this));
  socket->BindToNetDevice (l3->GetNetDevice (i));
  socket->Bind (InetSocketAddress (iface.GetBroadcast (), EXOR_PORT));
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
              InetSocketAddress inetAddr (m_ipv4->GetAddress (i, 0).GetLocal (), EXOR_PORT);
              if (socket->Bind (inetAddr))
                {
                  NS_FATAL_ERROR ("Failed to bind() ZEAL socket");
                }
              socket->BindToNetDevice (m_ipv4->GetNetDevice (i));
              socket->SetAllowBroadcast (true);
              socket->SetRecvCallback (MakeCallback (&RoutingProtocol::RecvExor, this));
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

  int8_t id =m_ipv4->GetObject<Node> ()->GetId ();
  if(id == 0)   //if Node ID = 0
    {
      std::cout<<"broadcast will be send\n";
      SendXBroadcast();
      //Simulator::Schedule(Seconds(2), &RoutingProtocol::SendXBroadcast, this);
    } 

}

void
RoutingProtocol::RecvExor (Ptr<Socket> socket)
{
  std::cout<<"In recv Exor(Node "<< m_ipv4->GetObject<Node> ()->GetId ()<<")\n";
}


void
RoutingProtocol::SendXBroadcast (void)
{
 for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j = m_socketAddresses.begin (); j
       != m_socketAddresses.end (); ++j)
    {

      Ptr<Socket> socket = j->first;
      Ipv4InterfaceAddress iface = j->second;
      Ptr<Packet> packet = Create<Packet> ();

      RrepHeader rrepHeader(0,3,Ipv4Address("10.1.1.15"),5,Ipv4Address("10.1.1.13"),Seconds(3));
      packet->AddHeader (rrepHeader);
      
      TypeHeader tHeader (EXORTYPE_RREP);
      packet->AddHeader (tHeader);
       
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
      socket->SendTo (packet, 0, InetSocketAddress (destination, EXOR_PORT));
      std::cout<<"broadcast sent\n"; 
        
     }
}




} //namespace exor
} //namespace ns3
