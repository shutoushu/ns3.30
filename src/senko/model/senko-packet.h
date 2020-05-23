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
#ifndef SENKOPACKET_H
#define SENKOPACKET_H

#include <iostream>
#include "ns3/header.h"
#include "ns3/enum.h"
#include "ns3/ipv4-address.h"
#include <map>
#include "ns3/nstime.h"

namespace ns3 {
namespace senko {

/**
* \ingroup senko
* \brief MessageType enumeration
*/
enum MessageType
{
  //SENKOTYPE_RREQ  = 1,   //!< SENKOTYPE_RREQ
  //SENKOTYPE_RREP  = 2,   //!< SENKOTYPE_RREP
  //SENKOTYPE_RERR  = 3,   //!< SENKOTYPE_RERR
  //SENKOTYPE_RREP_ACK = 4, //!< SENKOTYPE_RREP_ACK
  SENKOTYPE_DANGER = 5,
  SENKOTYPE_HELLOID = 6
};

/**
* \ingroup senko
* \brief SENKO types
*/
class TypeHeader : public Header
{
public:
  /**
   * constructor
   * \param t the SENKO RREQ type
   */
  //TypeHeader (MessageType t = SENKOTYPE_RREQ);
  //TypeHeader (MessageType t = SENKOTYPE_DANGER);
  TypeHeader (MessageType t = SENKOTYPE_HELLOID);
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
  MessageType Get () const
  {
    return m_type;
  }
  /**
   * Check that type if valid
   * \returns true if the type is valid
   */
  bool IsValid () const
  {
    return m_valid;
  }
  /**
   * \brief Comparison operator
   * \param o header to compare
   * \return true if the headers are equal
   */
  bool operator== (TypeHeader const & o) const;
private:
  MessageType m_type; ///< type of the message
  bool m_valid; ///< Indicates if the message is valid
};

/**
  * \brief Stream output operator
  * \param os output stream
  * \return updated stream
  */
std::ostream & operator<< (std::ostream & os, TypeHeader const & h);







///////////////////////////////////////////////////////////////////////////////////////////////
//danger
/**
* \ingroup senko
* \brief dangerheader
  \verbatim
  0                   1                   2                   3
  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |     Type      |R|A|    Reserved     |Prefix Sz|   Hop Count   |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     Destination IP address                    |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                  Destination Sequence Number                  |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                    Originator IP address                      |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                           Lifetime                            |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  \endverbatim
*/


class DangerHeader : public Header
{
public:
/*
nodeid 定期的なIDの伝搬に使うID
point IDを受け取った車両の位置情報
hopCount ブロードキャストされた回数
recvtime 車両がIDを受け取った時間
danger 　警戒値
*/
DangerHeader (int32_t nodeid = 0, int32_t posx = 0, 
int32_t posy = 0, int8_t hopcount = 0,int32_t recvtime = 0,
int8_t danger = 0, int32_t myposx = 0, int32_t myposy = 0,
 int32_t pastmyposx = 0, int32_t pastmyposy = 0);

static TypeId GetTypeId ();
TypeId GetInstanceTypeId () const;
uint32_t GetSerializedSize () const;
void Serialize (Buffer::Iterator start) const;
uint32_t Deserialize (Buffer::Iterator start);
void Print (std::ostream &os) const;


void SetNodeId (uint32_t id)  //IDをセットする
  {
    m_nodeid = id;
  }

int32_t GetNodeId ()  const//IDを返す
  {
    return m_nodeid;
  }
void SetPosX (int32_t p)    //座標をセットする
  {  //座標をセットする
    m_posx = p;
  }

int32_t GetPosX ()  //座標の値を返す
  {
    return m_posx;
  }
void SetPosY (int32_t p)    //座標をセットする
  {  //座標をセットする
    m_posy = p;
  }

int32_t GetPosY()  //座標の値を返す
  {
    return m_posy;
  }
  void SetHopCount (int8_t count) //hopcountを足していく関数
  {
    m_hopcount = count;
  }

int8_t GetHopCount () const  //hopcount の値を返す
  {
    return m_hopcount;
  }

void SetDanger (int8_t dan) //警戒地をセットする
  {
    m_danger = dan;
  }

int8_t GetDanger ()  //警戒値を返す
  {
    return m_danger;
  }

void SetRecvTime (int32_t recvtime)
{
  m_recvtime = recvtime;
}

//Time GetRecvTime () const;
int32_t GetRecvTime() 
{
  return m_recvtime;
}

void SetMyPosX (int32_t p)    //座標をセットする
  {  //座標をセットする
    m_myposx = p;
  }

int32_t GetMyPosX ()  //座標の値を返す
  {
    return m_myposx;
  }
void SetMyPosY (int32_t p)    //座標をセットする
  {  //座標をセットする
    m_myposy = p;
  }

int32_t GetMyPosY()  //座標の値を返す
  {
    return m_myposy;
  }

void SetMyPastPosX (int32_t p)    //座標をセットする
  {  //座標をセットする
    m_pastmyposx = p;
  }

int32_t GetMyPastPosX()  //座標の値を返す
  {
    return m_pastmyposx;
  }

void SetMyPastPosY (int32_t p)    //座標をセットする
  {  //座標をセットする
    m_pastmyposy = p;
  }

int32_t GetMyPastPosY()  //座標の値を返す
  {
    return m_pastmyposy;
  }
  bool operator== (DangerHeader const & o);
private:
  int32_t     m_nodeid;    //ノードID
  int32_t     m_posx;     //座標
  int32_t     m_posy;
  int8_t      m_hopcount;  //hop数
  int32_t     m_recvtime;  //IDを受け取った時間
  int8_t      m_danger;    //警戒値
  int32_t     m_myposx; //自分のx座標
  int32_t     m_myposy; //時分のY座標
  int32_t     m_pastmyposx;//自分の過去のｘ座標
  int32_t     m_pastmyposy;//時分の過去のｙ座標

};






















std::ostream & operator<< (std::ostream & os, DangerHeader const &);
//std::ostream & operator<< (std::ostream & os, IdHeader const &);
//std::ostream & operator<< (std::ostream & os, IdHeader  &);





}  // namespace senko
}  // namespace ns3

#endif /* SENKOPACKET_H */
