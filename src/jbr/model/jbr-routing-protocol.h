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
#ifndef JBRROUTINGPROTOCOL_H
#define JBRROUTINGPROTOCOL_H

#include "jbr-packet.h"
#include "ns3/node.h"
#include "ns3/random-variable-stream.h"
#include "ns3/output-stream-wrapper.h"
#include "ns3/ipv4-routing-protocol.h"
#include "ns3/ipv4-interface.h"
#include "ns3/ipv4-l3-protocol.h"
#include <map>

// #define SimTime 10
#define StartTime 0 //broadcast 開始時刻　秒
#define lossValue 4 //loss値にかける値

namespace ns3 {
namespace jbr {
/**
 * \ingroup jbr
 *
 * \brief JBR routing protocol
 */
class RoutingProtocol : public Ipv4RoutingProtocol
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  static const uint32_t JBR_PORT;

  //自作グローバル変数　
  static std::map<std::string, double> m_junction_x; // key junction id value xposition
  static std::map<std::string, double> m_junction_y; // key junction id value yposition
  static std::map<std::string, std::string> m_road_from_to; // key road id value junction from to


  //test recv count
  static std::map<int, int> recvCount; //key recvnode id value num recv

  /// constructor
  RoutingProtocol ();
  virtual ~RoutingProtocol ();
  virtual void DoDispose ();

  // Inherited from Ipv4RoutingProtocol
  Ptr<Ipv4Route> RouteOutput (Ptr<Packet> p, const Ipv4Header &header, Ptr<NetDevice> oif,
                              Socket::SocketErrno &sockerr);
  bool RouteInput (Ptr<const Packet> p, const Ipv4Header &header, Ptr<const NetDevice> idev,
                   UnicastForwardCallback ucb, MulticastForwardCallback mcb,
                   LocalDeliverCallback lcb, ErrorCallback ecb);
  virtual void NotifyInterfaceUp (uint32_t interface);
  virtual void NotifyInterfaceDown (uint32_t interface);
  virtual void NotifyAddAddress (uint32_t interface, Ipv4InterfaceAddress address);
  virtual void NotifyRemoveAddress (uint32_t interface, Ipv4InterfaceAddress address);
  virtual void SetIpv4 (Ptr<Ipv4> ipv4);
  virtual void PrintRoutingTable (Ptr<OutputStreamWrapper> stream, Time::Unit unit = Time::S) const;

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

private:
  void SendXBroadcast (void);
  void CallSendXUnicast (void);
  void SendXUnicast (int32_t one_before_x, int32_t one_before_y, int32_t local_source_x, 
  int32_t local_source_y, int32_t previous_x, int32_t previous_y, 
  int32_t des_id, int32_t des_x, int32_t des_y, int32_t hop);
  void RecvJbr (Ptr<Socket> socket);

  //**自作メソッド**//
  //road 判定 method
  int ReadSumoFile (void); //sumoからnetfileを読み込む
  double lineDistance (double line_x1, double line_y1, double line_x2, double line_y2, 
  double dot_x, double dot_y); //線分と座標の距離を返す
  std::string distinctionRoad (int x_point, int y_point); //x座標 y座標からroad id or junction idを返す関数
  int judgeIntersection (int x_point, int y_point); //x座標 y座標から　1(no intersection) or 0(intersection)を返す

  double getDistance (double x, double y, double x2, double y2);

  void SendHelloPacket (void); //hello packet を broadcast するメソッド
  void SendToHello (Ptr<Socket> socket, Ptr<Packet> packet,
                  Ipv4Address destination); //Hello packet の送信

  void SimulationResult (void); //シミュレーション結果を出力する

  /// IP protocol
  Ptr<Ipv4> m_ipv4;
  /// Nodes IP address
  Ipv4Address m_mainAddress;
  /// Raw unicast socket per each IP interface, map socket -> iface address (IP + mask)
  std::map<Ptr<Socket>, Ipv4InterfaceAddress> m_socketAddresses;
  /// Raw subnet directed broadcast socket per each IP interface, map socket -> iface address (IP + mask)
  std::map<Ptr<Socket>, Ipv4InterfaceAddress> m_socketSubnetBroadcastAddresses;

  Ptr<NetDevice> m_lo;

  /// Provides uniform random variables.
  Ptr<UniformRandomVariable> m_uniformRandomVariable;
};

} //namespace jbr
} //namespace ns3

#endif /* JBRROUTINGPROTOCOL_H */
