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
#define Dmin 200  //速度判定に至る距離の差の最小値
#define Vmin 100 //違反車両とみなす速度のしきい値
#define NS_LOG_APPEND_CONTEXT                                   \
  if (m_ipv4) { std::clog << "[node " << m_ipv4->GetObject<Node> ()->GetId () << "] "; }

#define ReceiveLog 1 //1 log 0 lognothing
#define ImageRange 40 //画像処理の距離
#define NodeNum 16 //ノード数
#define NotransTime 5//NotransTime 止まっていたら通信をやめさせる

#include "shuto-routing-protocol.h"
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
#include <stdio.h>
#include <map>
#include <cmath>

#include "ns3/mobility-module.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ShutoRoutingProtocol");

namespace shuto {
NS_OBJECT_ENSURE_REGISTERED (RoutingProtocol);

/// UDP Port for SHUTO control traffic
const uint32_t RoutingProtocol::SHUTO_PORT = 654;




RoutingProtocol::RoutingProtocol ()
{
 //printf("map\n");
 //printf(maprecvid);
 //printf(maprecvhelloid);
 //SaveXMap(1,500);
 //SaveXMap(2,300);
 //SaveXMap(3,400);
 //int i=GetXMap(1);
 //std::cout<<"getxmap"<<i<<"\n";
 


}

TypeId
RoutingProtocol::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::shuto::RoutingProtocol")
    .SetParent<Ipv4RoutingProtocol> ()
    .SetGroupName ("Shuto")
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
                        << ", SHUTO Routing table" << std::endl;

  //Print routing table here.
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
  socket->SetRecvCallback (MakeCallback (&RoutingProtocol::RecvShuto,this));
  socket->BindToNetDevice (l3->GetNetDevice (i));
  socket->Bind (InetSocketAddress (iface.GetLocal (), SHUTO_PORT));
  socket->SetAllowBroadcast (true);
  socket->SetIpRecvTtl (true);
  m_socketAddresses.insert (std::make_pair (socket,iface));


    // create also a subnet broadcast socket
  socket = Socket::CreateSocket (GetObject<Node> (),
                                 UdpSocketFactory::GetTypeId ());
  NS_ASSERT (socket != 0);
  socket->SetRecvCallback (MakeCallback (&RoutingProtocol::RecvShuto, this));
  socket->BindToNetDevice (l3->GetNetDevice (i));
  socket->Bind (InetSocketAddress (iface.GetBroadcast (), SHUTO_PORT));
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
              InetSocketAddress inetAddr (m_ipv4->GetAddress (i, 0).GetLocal (), SHUTO_PORT);
              if (socket->Bind (inetAddr))
                {
                  NS_FATAL_ERROR ("Failed to bind() ZEAL socket");
                }
              socket->BindToNetDevice (m_ipv4->GetNetDevice (i));
              socket->SetAllowBroadcast (true);
              socket->SetRecvCallback (MakeCallback (&RoutingProtocol::RecvShuto, this));
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
RoutingProtocol::DoInitialize (void)     //ノードの数だけnode0から呼び出される関数 
{
  uint32_t id = m_ipv4->GetObject<Node> ()->GetId ();
  //std::cout<< "doinitialize method id   " <<id<<"\n\n";
  //mtran[id]=0;//はじめ通信不許可にセットする　全ノード
  //myposcount[id]=0;
  if(id == 0)
  {
    SetPushBack();

  }

      for(int i=1; i<30; i++){
          Simulator::Schedule(Seconds(i), &RoutingProtocol::HelloNodeId, this);
          Simulator::Schedule(Seconds(i), &RoutingProtocol::SetMyPos, this);
      }

      




}

void
RoutingProtocol::RecvShuto (Ptr<Socket> socket)
{
  //std::cout<<"recv\n";
  Address sourceAddress;
  Ptr<Packet> packet = socket->RecvFrom (sourceAddress);
  uint32_t id = m_ipv4->GetObject<Node> ()->GetId ();
  //std::cout<<"packet size"<<packet->GetSize()<<"\n";
  

  TypeHeader tHeader (SHUTOTYPE_HELLOID);//default
  packet->RemoveHeader (tHeader);
  
  if (!tHeader.IsValid ())
    {
      NS_LOG_DEBUG ("Shuto protocol message " << packet->GetUid () << " with unknown type received: " << tHeader.Get () << ". Drop");
      return; // drop
    }
  switch (tHeader.Get ())
    {
    case SHUTOTYPE_HELLOID:
    {
      if(trans[id] == 1){//通信可能ノードのみ

      std::cout<<"In recv hellloid (Node "<< id<<")------------------------------------------------------\n";
      IdHeader Idheader;
      packet->RemoveHeader(Idheader);
      uint32_t helloid = Idheader.GetHelloId ();
      //std::cout<<"reveive  id value"<< helloid<<"\n\n";
      uint32_t helloxpoint = Idheader.GetHelloXpoint ();
      //std::cout<<"reveive  xpoint value"<< helloxpoint<<"\n\n";
      uint32_t helloypoint = Idheader.GetHelloYpoint ();
      //std::cout<<"reveive  ypoint value"<< helloypoint<<"\n\n";

      uint32_t recvidtime = Simulator::Now().GetMicroSeconds();
      //uint32_t recvidtime = Simulator::Now().GetSeconds();
      std::cout<<"reveive  time"<< recvidtime<<"\n\n";
      
      int intrecvidtime = recvidtime;  //８ビットをintに変換
      MobilityHelper mobility;
      //int posx,posy;
      //Ptr<ConstantPositionMobilityModel> mobility = m_ipv4->GetObject<ConstantPositionMobilityModel>();
      //posx = mobility->GetPosition().x;
      //posy = mobility->GetPosition().y;

      //std::cout<<"receice node xpostion "<<posx<<"receive node  ypositoin "<<posy<<"\n";
      
      if(mxpoint.count(helloid) == 0){//受け取ったIDを持っているかどうか

        std::cout<<"mxpoint doesn't have"<< helloid<<"."<<"\n";
        SendXBroadcast(helloid,helloxpoint,helloypoint,0,recvidtime,0);

      }else{//持っている場合

        std::cout<<"m has"<<helloid<<"."<<"\n";
        std::cout<<"スピード違反の疑いがある id="<<helloid<<"\n";
        std::cout<<" その車両のX座標は"<<mxpoint[helloid]<<"\n";
        std::cout<<" その車両のY座標は"<<mypoint[helloid]<<"\n";
        std::cout<<" その車両時間は"<<mtime[helloid]<<"\n";
        std::cout<<"その車両の警戒値は"<<mdanger[helloid]<<"\n";
        //---------------------------------------------------------------------------------speed measument
        double dist = std::sqrt((mxpoint[helloid] - helloxpoint) * (mxpoint[helloid] - helloxpoint) 
        + (mypoint[helloid] - helloypoint) * (mypoint[helloid] - helloypoint));
        std::cout<<" dist="<<dist<<"\n";
        if(dist>Dmin) //Dminより位置の差が大きい場合
        {
        std::cout<<" 速度を測定します"<<"\n";
        int timelag = intrecvidtime - mtime[helloid];
        double cotimelag = timelag/100000;
        std::cout<<" cotimelag="<<cotimelag<<"\n";
        double speed = dist/cotimelag*10;
        std::cout<<"その車両の速度は"<<speed<<"m/s"<<"\n";
        double cospeed = speed*3.6;
        std::cout<<"その車両の速度は"<<cospeed<<"km/h"<<"\n";
        if(cospeed>Vmin){//速度違反なら
          std::cout<<"速度違反者がいました　そのIDは"<<helloid<<"\n";
          mdanger[helloid]++; //警戒値の更新
          mxpoint[helloid]=helloxpoint; //速度違反に伴い
          mypoint[helloid]=helloypoint; //判定した位置情報と時間を
          mtime[helloid]=recvidtime; //更新する
          SendXBroadcast(helloid,helloxpoint,helloypoint,0,recvidtime,mdanger[helloid]);
          std::cout<<"mdanger[helloid]"<<mdanger[helloid]<<"\n";
        }else{//速度違反じゃないなら
          
        }

        }else{//Dminより位置の差が小さい場合
          std::cout<<"Dminより小さかったです"<<"\n";
          //SendXBroadcast(helloid,helloxpoint,helloypoint,0,recvidtime,0);
        }
        //---------------------------------------------------------------------------------speed measument
      }//持っている場合
      
      break;
      }
    }//SHUTOTYPE_HELLOID
    case SHUTOTYPE_DANGER:
      {
        if(trans[id] == 1){//通信可能ノードのみ

        DangerHeader dangerheader;
        packet->RemoveHeader(dangerheader);
        
        uint32_t recvid = dangerheader.GetNodeId();
        uint32_t posx = dangerheader.GetPosX();
        uint32_t posy = dangerheader.GetPosY();
        uint8_t hopcount = dangerheader.GetHopCount();
        uint32_t recvtime = dangerheader.GetRecvTime();
        uint8_t danger = dangerheader.GetDanger();
         std::cout<<"In recv broadcast (Node "<< id<<")------------------------------------------------------\n";
        if(mxpoint.count(recvid) == 0){//受け取ったIDを持っているかどうか
          std::cout<<"そのIDのマップを持っていないのでセットします\n";
          ////////////////////////////////////////////////////////////////////パケットMidをマップに保存
          SaveXMap(recvid,posx);
          SaveYMap(recvid,posy);
          SaveTMap(recvid,recvtime);
          SaveDMap(recvid, danger);
          if(hopcount ==  0)
        {
        
        std::cout<<"GetBroadcast value-------------------------------------------------\n";
      
        std::cout<<"broadcast reveive hellonode id"<< recvid<<"\n\n";
        std::cout<<"broadcast reveive point"<< posx<<"\n\n";
        std::cout<<"broadcast reveive point"<< posy<<"\n\n";
        std::cout<<"broadcast reveive hopcount"<< (unsigned)hopcount<<"\n\n";
        std::cout<<"broadcast reveive recvtime"<< recvtime<<"\n\n";
        std::cout<<"broadcast reveive danger"<< (unsigned)danger<<"\n\n";
        
        std::cout<<"----------------------------------------------------------------\n\n";
        //hopcount = 2;
        hopcount = 1;
        ReSendXBroadcast(recvid, posx, posy, hopcount, recvtime, danger );
        }else if(hopcount <=4)
        {
           std::cout<<"In recv rebroadcast (Node "<< id<<")------------------------------------------------------\n";
          std::cout<<"GetReBroadcast value-------------------------------------------------\n";

          std::cout<<"broadcast reveive hellonode id"<< recvid<<"\n\n";
          std::cout<<"broadcast reveive point"<< posx<<"\n\n";
          std::cout<<"broadcast reveive point"<< posy<<"\n\n";
          std::cout<<"broadcast reveive hopcount"<< (unsigned)hopcount<<"\n\n";
          std::cout<<"broadcast reveive recvtime"<< recvtime<<"\n\n";
          std::cout<<"broadcast reveive danger"<< (unsigned)danger<<"\n\n";
          hopcount = hopcount+1;
          std::cout<<"----------------------------------------------------------------\n\n";
        }////受け取ったIDを持っているかどうかのif

          ////////////////////////////////////////////////////////////////////////
        }else{//持っている場合

          if(mdanger[recvid]<danger){//保持している警戒値と受け取ったWidの警戒値が異なっていたら
            mdanger[recvid]=danger;
            mxpoint[recvid]=posx;
            mypoint[recvid]=posy;
            mtime[recvid]=recvtime;
          }else{
            std::cout<<"すでにそのIDをマップを保持しているのでセットしません\n";
          }
           if(hopcount ==  0)
        {
        
        std::cout<<"GetBroadcast value-------------------------------------------------\n";
      
        std::cout<<"broadcast reveive hellonode id"<< recvid<<"\n\n";
        std::cout<<"broadcast reveive point"<< posx<<"\n\n";
        std::cout<<"broadcast reveive point"<< posy<<"\n\n";
        std::cout<<"broadcast reveive hopcount"<< (unsigned)hopcount<<"\n\n";
        std::cout<<"broadcast reveive recvtime"<< recvtime<<"\n\n";
        std::cout<<"broadcast reveive danger"<< (unsigned)danger<<"\n\n";
        
        std::cout<<"----------------------------------------------------------------\n\n";
        //hopcount = 2;
        hopcount = 1;
        ReSendXBroadcast(recvid, posx, posy, hopcount, recvtime, danger );
        }else if(hopcount <=4)
        {
           std::cout<<"In recv rebroadcast (Node "<< id<<")------------------------------------------------------\n";
          std::cout<<"GetReBroadcast value-------------------------------------------------\n";

          std::cout<<"broadcast reveive hellonode id"<< recvid<<"\n\n";
          std::cout<<"broadcast reveive point"<< posx<<"\n\n";
          std::cout<<"broadcast reveive point"<< posy<<"\n\n";
          std::cout<<"broadcast reveive hopcount"<< (unsigned)hopcount<<"\n\n";
          std::cout<<"broadcast reveive recvtime"<< recvtime<<"\n\n";
          std::cout<<"broadcast reveive danger"<< (unsigned)danger<<"\n\n";
          hopcount = hopcount+1;
          std::cout<<"----------------------------------------------------------------\n\n";
        }








        }//持っている場合
        
        break;


      }//case SHUTO_DANGER
    
      }//通信可能ノードのみ
    }
 
 


  
}

void
RoutingProtocol::SendXBroadcast (uint32_t recvid, uint32_t posx ,uint32_t posy, uint8_t hopcount, uint32_t time,uint8_t danger )
{
  //std::cout<<"sendxbroadcast\n\n";
  for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j = m_socketAddresses.begin (); j
       != m_socketAddresses.end (); ++j)
    {
      uint32_t id = m_ipv4->GetObject<Node> ()->GetId ();//doinitialize  のIDと同じID

      Ptr<Socket> socket = j->first;
      Ipv4InterfaceAddress iface = j->second;
      Ptr<Packet> packet = Create<Packet> ();
      /////////////////////////////////////////////////////////////////////////////
     // dangerheader sendcodes
      
      //DangerHeader dangerHeader(id,11,11,11,11);
      DangerHeader dangerHeader(recvid,posx,posy,hopcount,time,danger);
      packet->AddHeader (dangerHeader);
      TypeHeader tHeader (SHUTOTYPE_DANGER);
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
      std::cout<<Simulator::Now().GetMicroSeconds()<<"\n";  
      socket->SendTo (packet, 0, InetSocketAddress (destination, SHUTO_PORT));
      
      std::cout<<"broadcast sent id " << id << "sendid"<< recvid <<"\n";
        
     }
  
}

void
RoutingProtocol::HelloNodeId(void)
{
 for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j = m_socketAddresses.begin (); j
       != m_socketAddresses.end (); ++j)
    {
      uint32_t id = m_ipv4->GetObject<Node> ()->GetId ();//doinitialize  のIDと同じID
      if(trans[id] == 1  && id == 10){//通信可能ノードのみ
      std::cout<< "HelloNodeId method id   " <<id<<"\n\n";

      //shuto scenario kokodekireru
      int posx,posy;
      /*
      if(id !=8){
        Ptr<ConstantPositionMobilityModel> mobility = m_ipv4->GetObject<ConstantPositionMobilityModel>();
        posx = mobility->GetPosition().x;
        posy = mobility->GetPosition().y;
        std::cout<<"hellosend node xpostion "<<posx<<"hellosend node  ypositoin "<<posy<<"\n";
      }else{
        Ptr<WaypointMobilityModel> mobility = m_ipv4->GetObject<WaypointMobilityModel>();
        posx = mobility->GetPosition().x;
        posy = mobility->GetPosition().y;
        std::cout<<"hellosend node xpostion "<<posx<<"hellosend node  ypositoin "<<posy<<"\n";
      }*/
      //位置情報を取得
      Ptr<MobilityModel> mobility = m_ipv4->GetObject<Node> () ->GetObject<MobilityModel>();
      Vector mypos = mobility->GetPosition ();
      posx = mypos.x;
      posy = mypos.y;

      Ptr<Socket> socket = j->first;
      Ipv4InterfaceAddress iface = j->second;
      Ptr<Packet> packet = Create<Packet> ();
    
      IdHeader idheader(id,posx,posy);
      packet->AddHeader (idheader);
      TypeHeader tHeader (SHUTOTYPE_HELLOID);  
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
      std::cout<<Simulator::Now().GetMicroSeconds()<<"\n";  
      Time Jitter = Time (MicroSeconds (m_uniformRandomVariable->GetInteger (0,510)));
      //socket->SendTo (packet, 0, InetSocketAddress (destination, SHUTO_PORT));
      Simulator::Schedule (Jitter, &RoutingProtocol::SendTo,this, socket, packet,destination);
     }
    }
}//Hellonodeid

void
RoutingProtocol::SendTo(Ptr<Socket> socket, Ptr<Packet> packet, Ipv4Address destination){
  socket->SendTo (packet, 0, InetSocketAddress (destination, SHUTO_PORT));
}


void
RoutingProtocol::ReSendXBroadcast (uint32_t id, uint32_t posx ,
          uint32_t posy, uint8_t hopcount, uint32_t time,uint8_t danger )
{
 for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j = m_socketAddresses.begin (); j
       != m_socketAddresses.end (); ++j)
    {
      //uint32_t id = m_ipv4->GetObject<Node> ()->GetId ();//doinitialize  のIDと同じID
      //std::cout<< "ReSendXBroadCast method id   " <<id<<"\n\n";


      Ptr<Socket> socket = j->first;
      Ipv4InterfaceAddress iface = j->second;
      Ptr<Packet> packet = Create<Packet> ();
      /////////////////////////////////////////////////////////////////////////////
      //dangerheader sendcode
      std::cout<<"rebroadcast send"<<(unsigned)hopcount<<"\n";
      DangerHeader dangerHeader(id, posx, posy, hopcount, time, danger);
      packet->AddHeader (dangerHeader);  
      TypeHeader tHeader (SHUTOTYPE_DANGER);  
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
      //std::cout<<Simulator::Now().GetMicroSeconds()<<"\n";  
      socket->SendTo (packet, 0, InetSocketAddress (destination, SHUTO_PORT));
      
      //std::cout<<"broadcast sent\n"; 
        
     }
}//resendxbroadcast

//////////////////////////////////////////////////////////////////////////////mapmethod
void
RoutingProtocol::SaveXMap(int mapid, int mapxpoint)
{
 
mxpoint[mapid] = mapxpoint;
if(mxpoint.find(1) == mxpoint.end()){
  std::cout<<"not found"<<std::endl;
}else{
  std::cout<<"found"<<std::endl;
}


}//savemap

int
RoutingProtocol::GetXMap(int getid)
{
  return mxpoint[getid];
}

void
RoutingProtocol::SaveYMap(int mapid, int mapypoint)
{
 
mypoint[mapid] = mapypoint;
if(mypoint.find(1) == mypoint.end()){
  std::cout<<"not found"<<std::endl;
}else{
  std::cout<<"found"<<std::endl;
}


}//savemap

int
RoutingProtocol::GetYMap(int getid)
{
  return mypoint[getid];
}

void
RoutingProtocol::SaveTMap(int mapid, int maptime)
{
 
mtime[mapid] = maptime;
if(mtime.find(1) == mtime.end()){
  std::cout<<"not found"<<std::endl;
}else{
  std::cout<<"found"<<std::endl;
}


}//savemap

int
RoutingProtocol::GetTMap(int getid)
{
  return mtime[getid];
}

void
RoutingProtocol::SaveDMap(int mapid, int mapdanger)
{
 
mdanger[mapid] = mapdanger;
if(mdanger.find(1) == mdanger.end()){
  std::cout<<"not found"<<std::endl;
}else{
  std::cout<<"found"<<std::endl;
}


}//savemap

int
RoutingProtocol::GetDMap(int getid)
{
  return mdanger[getid];
}



////////////////////////////////////////////////////////mapmethod
/*
int GetSpeed(int newx,int newy, int newtime, int x,int y, int time){

  double dist = std::sqrt((newx - x) * (newx - x) + (newy - y) * (newy - y));
  double timelag = newtime -  time;
  double speed = dist/timelag;
  return speed;
}
*/
bool has_key_using_count(std::map<int,int> &mxpoint,int n)
{
  if (mxpoint.count(n) == 0){
        std::cout << "m doesn't have " << n << "." << std::endl;
        return false;
    }
    else{
        std::cout << "m has " << n << "." <<std::endl;
        return true;
    }
}
//// ノードが自分の速度を自分で図る　ｘ秒ごとの自分のポジションを保存

void
RoutingProtocol::SetMyPos()//自分の位置を取得してマップに保存
{
  //std::cout<<"setmypos"<<"\n\n";
  uint32_t id = m_ipv4->GetObject<Node> ()->GetId ();
  int myposx,myposy;
  Ptr<MobilityModel> mobility = m_ipv4->GetObject<Node> () ->GetObject<MobilityModel>();
   Vector mypos = mobility->GetPosition ();
   myposx = mypos.x;
   myposy = mypos.y;
  if(trans[id] != 2){//目的地に到着してなかったら

  if(myxpoint.count(id) == 0){//すでに自分の前の位置がセットされていないとき
   myxpoint[id] = myposx;
   myypoint[id] = myposy;
  }else{//前の座標のデータがあるとき
   double mydist = std::sqrt((myxpoint[id] - myposx) * (myxpoint[id] - myposx) 
        + (myypoint[id] - myposy) * (mypoint[id] - myposy));//ｘ秒前の位置と現在の自分の位置の距離
        ////////////////////////////////////x=1のときmydistは秒速に値する
       // std::cout<<"１秒前からの移動"<<mydist<<"\n";
        double myspeed=mydist*60*60/1000;
        //std::cout<<"自分の時速"<<myspeed<<"\n";
        if(mydist > 0 && trans[id] == 0){//車両が動き出したら　
          trans[id]=1;//通信可能にする
          //std::cout<<"通信可能になりました"<<id<<"\n";
        }

        if (myspeed > Vmin)
        {
          //std::cout<<"違反車両がいました そのID"<<id<<"\n";
        }

        if(mydist == 0 && trans[id] == 1)//通信可能車両が止まっていたら
        {
          myposcount[id]++;
        }
        if(mydist > 0 && myposcount[id] > 0)//何秒か止まっていたのに再び動き出したら
        {
          myposcount[id]=0;//止まっていた時間を初期化する
        }
        //時速に変換
        if(myposcount[id] > NotransTime && trans[id] == 1){//通信可能ノードが一定期間停止
          trans[id]=2;
          std::cout<<"通信不可能になりました"<<"\n";
        }
   myxpoint[id] = myposx;//位置情報をマップに保存
   myypoint[id] = myposy;
  }

  }//目的地に到着してなかったら


}

std::vector<int> RoutingProtocol::trans;

} //namespace shuto
} //namespace ns3
