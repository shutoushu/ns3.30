/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2018 Alberto Gallegos
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
 * Author: Alberto Gallegos <ramonet@fc.ritsumei.ac.jp>
 *         Ritsumeikan University, Shiga, Japan
 */
#ifndef SHUTOROUTINGPROTOCOL_H
#define SHUTOROUTINGPROTOCOL_H


#include "shuto-packet.h"
#include "ns3/node.h"
#include "ns3/random-variable-stream.h"
#include "ns3/output-stream-wrapper.h"
#include "ns3/ipv4-routing-protocol.h"
#include "ns3/ipv4-interface.h"
#include "ns3/ipv4-l3-protocol.h"
#include <map>

namespace ns3 {
namespace shuto {
/**
 * \ingroup shuto
 *
 * \brief SHUTO routing protocol
 */

class RoutingProtocol : public Ipv4RoutingProtocol
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  static const uint32_t SHUTO_PORT;

  static std::vector<int> trans; //配列番号がID　値が通信の値　初期値０　動く１　止２

  /// constructor
  RoutingProtocol ();
  virtual ~RoutingProtocol ();
  virtual void DoDispose ();

  // Inherited from Ipv4RoutingProtocol
  Ptr<Ipv4Route> RouteOutput (Ptr<Packet> p, const Ipv4Header &header, Ptr<NetDevice> oif, Socket::SocketErrno &sockerr);
  bool RouteInput (Ptr<const Packet> p, const Ipv4Header &header, Ptr<const NetDevice> idev,
                   UnicastForwardCallback ucb, MulticastForwardCallback mcb,
                   LocalDeliverCallback lcb, ErrorCallback ecb);
  virtual void NotifyInterfaceUp (uint32_t interface);
  virtual void NotifyInterfaceDown (uint32_t interface);
  virtual void NotifyAddAddress (uint32_t interface, Ipv4InterfaceAddress address);
  virtual void NotifyRemoveAddress (uint32_t interface, Ipv4InterfaceAddress address);
  virtual void SetIpv4 (Ptr<Ipv4> ipv4);
  virtual void PrintRoutingTable (Ptr<OutputStreamWrapper> stream, Time::Unit unit = Time::S) const;


 void SetPushBack()
 {
   for(int i = 0; i<500; i++)
   trans.push_back(0);
 }


  /**
   * Assign a fixed random variable stream number to the random variables
   * used by this model.  Return the number of streams (possibly zero) that
   * have been assigned.
   *
   * \param stream first stream index to use
   * \return the number of stream indices assigned by this model
   */
  int64_t AssignStreams (int64_t stream);

protected:
  virtual void DoInitialize (void);
  //int GetSpeed(int newx,int newy, int newtime, int x,int y, int time);
private:
  void HelloNodeId(void); //１秒毎の定期的なIDのブロードキャスト
  void SendXBroadcast(uint32_t id, uint32_t posx,uint32_t posy, 
  uint8_t hopcount, uint32_t time,uint8_t danger );//IDを受け取ったときのブロードキャスト
  void ReSendXBroadcast(uint32_t id, uint32_t posx,
   uint32_t posy, uint8_t hopcount, uint32_t time,uint8_t danger );
  //dangerheader を受け取ったとき
  void RecvShuto (Ptr<Socket> socket);
  
void SendTo(Ptr<Socket> socket, Ptr<Packet> packet, Ipv4Address destination);
//mapmethod
  void SaveXMap(int mapid,int mapxpoint);
  int GetXMap(int getid);
  void SaveYMap(int mapid,int mapypoint);
  int GetYMap(int getid);
  void SaveTMap(int mapid,int maptime);
  int GetTMap(int getid);
  void SaveDMap(int mapid,int mapdanger);
  int GetDMap(int getid);
  void SaveDMap(int id);
  void SetMyPos();

  bool has_key_using_count(std::map<int,int> &mxpoint,int n);
  /// IP protocol
  Ptr<Ipv4> m_ipv4;
  /// Nodes IP address
  Ipv4Address m_mainAddress;
  /// Raw unicast socket per each IP interface, map socket -> iface address (IP + mask)
  std::map< Ptr<Socket>, Ipv4InterfaceAddress > m_socketAddresses;
  /// Raw subnet directed broadcast socket per each IP interface, map socket -> iface address (IP + mask)
  std::map< Ptr<Socket>, Ipv4InterfaceAddress > m_socketSubnetBroadcastAddresses;
  std::map<int,int> mxpoint;//x座標保存用のマップ
  std::map<int,int> mypoint;//ｙ座標保存用のマップ
  std::map<int,int> mtime;//時間保存用のマップ
  std::map<int,int> mdanger;//警戒値保存用のマップ
  std::map<int,int> mtran; //<id,0or1> 初期値０　通信可能１　目的地到着２
  std::map<int,int> myxpoint; //自分のノードのx座標
  std::map<int,int> myypoint; //自分のノードのy座標
  std::map<int,int> myspeed; // 自分のスピード　
  std::map<int,int> myposcount; //ｘ秒ごとに自分が止まっている回数
  Ptr<NetDevice> m_lo;

  

 
  /// Provides uniform random variables.
  Ptr<UniformRandomVariable> m_uniformRandomVariable;

};

} //namespace shuto
} //namespace ns3

#endif /* SHUTOROUTINGPROTOCOL_H */
