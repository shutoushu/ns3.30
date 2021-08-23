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
#ifndef JBRPACKET_H
#define JBRPACKET_H

#include <iostream>
#include "ns3/header.h"
#include "ns3/enum.h"
#include "ns3/ipv4-address.h"
#include <map>
#include "ns3/nstime.h"

namespace ns3 {
namespace jbr {

/**
* \ingroup jbr
* \brief MessageType enumeration
*/
enum MessageType {
  JBRTYPE_RECOVER = 1,
  JBRTYPE_HELLO = 2
};

/**
* \ingroup jbr
* \brief JBR types
*/
class TypeHeader : public Header
{
public:
  /**
   * constructor
   * \param t the JBR RREQ type
   */
  TypeHeader (MessageType t = JBRTYPE_HELLO);

  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId ();
  TypeId GetInstanceTypeId () const;
  uint32_t GetSerializedSize () const;
  void Serialize (Buffer::Iterator start) const;
  uint32_t Deserialize (Buffer::Iterator start);
  void Print (std::ostream &os) const;

  /**
   * \returns the type
   */
  MessageType
  Get () const
  {
    return m_type;
  }
  /**
   * Check that type if valid
   * \returns true if the type is valid
   */
  bool
  IsValid () const
  {
    return m_valid;
  }
  /**
   * \brief Comparison operator
   * \param o header to compare
   * \return true if the headers are equal
   */
  bool operator== (TypeHeader const &o) const;

private:
  MessageType m_type; ///< type of the message
  bool m_valid; ///< Indicates if the message is valid
};

/**
  * \brief Stream output operator
  * \param os output stream
  * \return updated stream
  */
std::ostream &operator<< (std::ostream &os, TypeHeader const &h);

///////////////////////////////////////////////////////////////////////////////////////////////
//*******************************start Hello JBR***********************************************/
/**
* \ingroup jbr
* \brief jbrheader
  \verbatim
  0                   1                   2                   3
  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     send Node ID                              |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     Send Node Xpostion                        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     Send Node Yposition                       |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  \endverbatim
*/

class HelloHeader : public Header
{
public:
  /*
nodeid Hellomessageを送信する車両のID
posx posy Hellomessageを送信する車両の位置情報
*/
  HelloHeader (int32_t nodeid = 0, int32_t posx = 0, int32_t posy = 0);

  static TypeId GetTypeId ();
  TypeId GetInstanceTypeId () const;
  uint32_t GetSerializedSize () const;
  void Serialize (Buffer::Iterator start) const;
  uint32_t Deserialize (Buffer::Iterator start);
  void Print (std::ostream &os) const;

  void
  SetNodeId (uint32_t id) //IDをセットする
  {
    m_nodeid = id;
  }

  int32_t
  GetNodeId () const //IDを返す
  {
    return m_nodeid;
  }
  void
  SetPosX (int32_t p) //座標をセットする
  { //座標をセットする
    m_posx = p;
  }

  int32_t
  GetPosX () //座標の値を返す
  {
    return m_posx;
  }
  void
  SetPosY (int32_t p) //座標をセットする
  { //座標をセットする
    m_posy = p;
  }

  int32_t
  GetPosY () //座標の値を返す
  {
    return m_posy;
  }

  bool operator== (HelloHeader const &o);

private:
  int32_t m_nodeid; //ノードID
  int32_t m_posx; // x座標
  int32_t m_posy; // y座標
};

//*******************************end Hello Jbr***********************************************/


///////////////////////////////////////////////////////////////////////////////////////////////
//*******************************start JBR recovery unicast***********************************************/
/**
* \ingroup jbr
* \brief jbrheader
  \verbatim
  0                   1                   2                   3
  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     send Node ID                              |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     Send Node Xpostion                        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     Send Node Yposition                       |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     Next Node ID                              |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     Local source Node Xpostion                |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     Local source Node Ypostion                |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     Previous Node  Xposition                  |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     Previous Node  Yposition                  |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     Destination Node  ID                      |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     Destination Node  Xposition               |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     Destinatino Node  Yposition               |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     hopcount                                  |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  \endverbatim
*/

class JbrHeader : public Header
{
public:
  /*
*/
  JbrHeader (int32_t send_id = 0, int32_t send_x = 0, int32_t send_y = 0,
  int32_t next_id = 0, int32_t local_source_x = 0, int32_t local_source_y = 0,
  int32_t previous_x = 0, int32_t previous_y = 0, int32_t des_id = 0,
  int32_t des_x = 0, int32_t des_y = 0, int32_t hop = 0);

  static TypeId GetTypeId ();
  TypeId GetInstanceTypeId () const;
  uint32_t GetSerializedSize () const;
  void Serialize (Buffer::Iterator start) const;
  uint32_t Deserialize (Buffer::Iterator start);
  void Print (std::ostream &os) const;

  void
  SetSendId (uint32_t id) //IDをセットする
  {
    m_send_id = id;
  }

  int32_t
  GetSendId () const //IDを返す
  {
    return m_send_id;
  }
  void
  SetSendX (int32_t p) //座標をセットする
  { //座標をセットする
    m_send_x = p;
  }
  int32_t
  GetSendX () //座標の値を返す
  {
    return m_send_x;
  }
  void
  SetSendY (int32_t p) //座標をセットする
  { //座標をセットする
    m_send_y = p;
  }
  int32_t
  GetSendY () //座標の値を返す
  {
    return m_send_y;
  }
  void
  SetNextId (uint32_t id) //IDをセットする
  {
    m_next_id = id;
  }
  int32_t
  GetNextId () const //IDを返す
  {
    return m_next_id;
  } 

  void
  SetLocalSourceX (int32_t p) //座標をセットする
  { //座標をセットする
    m_local_source_x = p;
  }
  int32_t
  GetLocalSourceX () //座標の値を返す
  {
    return m_local_source_x;
  }
  void
  SetLocalSourceY (int32_t p) //座標をセットする
  { //座標をセットする
    m_local_source_y = p;
  }
  int32_t
  GetLocalSourceY () //座標の値を返す
  {
    return m_local_source_y;
  }

  void
  SetPreviousX (int32_t p) //座標をセットする
  { //座標をセットする
    m_previous_x = p;
  }
  int32_t
  GetPreviousX () //座標の値を返す
  {
    return m_previous_x;
    
  }
  void
  SetPreviousY (int32_t p) //座標をセットする
  { //座標をセットする
    m_previous_y = p;
  }
  int32_t
  GetPreviousY () //座標の値を返す
  {
    return m_previous_y;
  }

  void
  SetDesId (uint32_t id) //IDをセットする
  {
    m_des_id = id;
  }

  int32_t
  GetDesId () const //IDを返す
  {
    return m_des_id;
  }
  void
  SetDesX (int32_t p) //座標をセットする
  { //座標をセットする
    m_des_x = p;
  }
  int32_t
  GetDesX () //座標の値を返す
  {
    return m_des_x;
  }
  void
  SetDesY (int32_t p) //座標をセットする
  { //座標をセットする
    m_des_y = p;
  }
  int32_t
  GetDesY () //座標の値を返す
  {
    return m_des_y;
  }

  void
  SetHop (int32_t p) //座標をセットする
  { //座標をセットする
    m_hop = p;
  }
  int32_t
  GetHop () //座標の値を返す
  {
    return m_hop;
  }

  bool operator== (JbrHeader const &o);

private:
  int32_t m_send_id; // 送信ノードID
  int32_t m_send_x; // 送信ノードx座標
  int32_t m_send_y; // 送信ノードy座標
  int32_t m_next_id; // ネクストホップID
  int32_t m_local_source_x; //local source node xposition
  int32_t m_local_source_y; //local source node yposition
  int32_t m_previous_x; //previous node xposition
  int32_t m_previous_y; //previous node yposition
  int32_t m_des_id; //destination node id
  int32_t m_des_x;  //destination node x
  int32_t m_des_y;  //destination node y
  int32_t m_hop; //hopcount 
};

//*******************************end Hello Jbr***********************************************/

} // namespace jbr
} // namespace ns3

#endif /* JBRPACKET_H */
