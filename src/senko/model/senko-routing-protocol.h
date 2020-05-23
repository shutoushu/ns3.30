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
#ifndef SENKOROUTINGPROTOCOL_H
#define SENKOROUTINGPROTOCOL_H


#include "senko-packet.h"
#include "ns3/node.h"
#include "ns3/random-variable-stream.h"
#include "ns3/output-stream-wrapper.h"
#include "ns3/ipv4-routing-protocol.h"
#include "ns3/ipv4-interface.h"
#include "ns3/ipv4-l3-protocol.h"
#include <map>

namespace ns3 {
namespace senko {
/**
 * \ingroup senko
 *
 * \brief SENKO routing protocol
 */

class RoutingProtocol : public Ipv4RoutingProtocol
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  static const uint32_t SENKO_PORT;

  static std::vector<double> GodXposition;//神様視点から見たid x座標
  static std::vector<double> GodYposition;//神様視点から見たid y座標
  static std::vector<int> trans; //配列番号がID　値が通信の値　初期値０　動く１　止２
  static std::vector<int> broadcount;//ブロードキャストの回数をカウント
  static std::vector<int> rebroadcount;//リブロードキャストの回数をカウント
  static std::vector<int> vehiclecount; //配列番号に速度違反車両のID　値は速度違反だった回数
  static std::vector<int> detectdanger; //配列番号に速度違反車両のID　値は警戒値


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


  void SetGodposition(double x, double y) //神様視点でx座標をセット
  {
    int32_t id = m_ipv4->GetObject<Node> ()->GetId ();
    //std::cout<<"setgodposition id"<<id<<"\n";
    GodXposition[id]=x;
    GodYposition[id]=y;
  }
  void SetGodCall();

  void SetPushBack()
  {
      for(int i=0; i<150; i++)
  {
    GodXposition.push_back(0);
    GodYposition.push_back(0);
    trans.push_back(0);
    vehiclecount.push_back(0);
    detectdanger.push_back(0);
  }
  broadcount.push_back(0);
  rebroadcount.push_back(0);
  }



  /**
   * Assign a fixed random variable stream number to the random variables
   * used by this model.  Return the number of streams (possibly zero) that
   * have been assigned.
   *
   * \param stream first stream index to use
   * \return the number of stream indices assigned by this model
   */
  uint64_t AssignStreams (uint64_t stream);

protected:
  virtual void DoInitialize (void);
  //int GetSpeed(int newx,int newy, int newtime, int x,int y, int time);
private:
  void HelloNodeId(void); //１秒毎の定期的なIDのブロードキャスト
  void SendXBroadcast(int32_t id, int32_t posx,int32_t posy, 
  int8_t hopcount, int32_t time,int8_t danger, int sourcex, int sourcey);//IDを受け取ったときのブロードキャスト
  
  void ReSendXBroadcast(int32_t id, int32_t posx,int32_t posy, 
  int8_t hopcount, int32_t time,int8_t danger, int sourcex, int sourcey);
  //dangerheader を受け取ったとき
  void RecvSenko (Ptr<Socket> socket);
  
void SendTo(Ptr<Socket> socket, Ptr<Packet> packet, Ipv4Address destination, 
int hopcount, int sendid);
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
  void GetAngle(double x, double y, double x2, double y2,double recvid);
  void EraseAngle();//１秒毎に、角度テーブルを削除
  double GetDist(double x, double y, double x2, double y2);
  void GetGod();//全ノードの位置情報をマップに保存
  void GetAreaGod();//エリア内にいるIDと位置情報をマップに保存
  void Direction(double current_x, double current_y,
  double source_x, double source_y);//パケットからの方向を確認する

  void PauseTime();//ノードごとに待ち時間を計測する関数


  void PrintTime();//ｘ秒ごとに時間をプリントする


  bool has_key_using_count(std::map<int,int> &mxpoint,int n);
  /// IP protocol
  Ptr<Ipv4> m_ipv4;
  /// Nodes IP address
  Ipv4Address m_mainAddress;
  /// Raw unicast socket per each IP interface, map socket -> iface address (IP + mask)
  std::map< Ptr<Socket>, Ipv4InterfaceAddress > m_socketAddresses;
  /// Raw subnet directed broadcast socket per each IP interface, map socket -> iface address (IP + mask)
  std::map< Ptr<Socket>, Ipv4InterfaceAddress > m_socketSubnetBroadcastAddresses;
  std::map<int,int> mxpoint;//危険車両IDとx座標保存用のマップ
  std::map<int,int> mypoint;//危険車両IDｙ座標保存用のマップ
  std::map<int,int> mtime;//危険車両ID時間保存用のマップ
  std::map<int,int> mdanger;//危険車両ID警戒値保存用のマップ
  
  std::map<int,int> mtran; //<id,0or1> 初期値０　通信可能１　目的地到着２


  std::map<int,double> myxpoint; //自分のノードのx座標
  std::map<int,double> myypoint; //自分のノードのy座標
  std::map<int,double> mypastxpoint;// 過去の自分のノードのx座標
  std::map<int,double> mypastypoint;// 過去の自分のノードのy座標

  std::map<int,double> m_myspeed; // 自分のスピード　
  std::map<int,int> myposcount; //ｘ秒ごとに自分が止まっている回数
  std::map<int,int> imagex;  //画像処理で取得したID,ｘ座標
  std::map<int,int> imagey;  //画像処理で取得したID, y座標
  std::map<int,double> imageangle; //画像処理で取得した、IDと角度

  std::map<int,int> getimageX; //画像処理で実際に取得できたIDとｘ座標
  std::map<int,int> getimageY; //画像処理で実際に取得できたIDとｙ座標

  std::map<int,double> godx; //
  std::map<int,double> gody; //全ノードの位置情報を保存　神様視点
  std::map<int,double> godDist; //対象ノードから見た全ノードとの距離


  std::map<int,double> areax;//画像処理内にいるノードIDと座標
  std::map<int,double> areay;//画像処理内にいるノードIDと座標
  std::map<int,double> areaangle;//画像処理内にいるノードIDと角度
  std::map<int,double> areadist; //画像処理内にいるノードIDと距離
  std::map<int,double> areamyangle;
  //画像処理内にいる画像処理ノードの進行方向から見た、
  //近隣ノードのIDと角度
  std::map<int,int> sendid; //画像処理ノードがすでに送ったIDは１
  //まだ送っていなかったら0
  std::map<int,int> rebroadid;//リブロードで受け取ったIDとホップカウント
  std::map<int,int> rebroad_danger;//リブロードで受け取ったIDと警戒値
  std::map<int,int> detect_danger; 
  Ptr<NetDevice> m_lo;
  /// Provides uniform random variables.
  Ptr<UniformRandomVariable> m_uniformRandomVariable;

};

} //namespace senko
} //namespace ns3

#endif /* SENKOROUTINGPROTOCOL_H */
