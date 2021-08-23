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
#include "jbr-packet.h"
#include "ns3/address-utils.h"
#include "ns3/packet.h"

namespace ns3 {
namespace jbr {

NS_OBJECT_ENSURE_REGISTERED (TypeHeader);

TypeHeader::TypeHeader (MessageType t) : m_type (t), m_valid (true)
{
}

TypeId
TypeHeader::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::jbr::TypeHeader")
                          .SetParent<Header> ()
                          .SetGroupName ("Jbr")
                          .AddConstructor<TypeHeader> ();
  return tid;
}

TypeId
TypeHeader::GetInstanceTypeId () const
{
  return GetTypeId ();
}

uint32_t
TypeHeader::GetSerializedSize () const
{
  return 1;
}

void
TypeHeader::Serialize (Buffer::Iterator i) const
{
  i.WriteU8 ((uint8_t) m_type);
}

uint32_t
TypeHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  uint8_t type = i.ReadU8 ();
  m_valid = true;
  switch (type)
    {
    case JBRTYPE_RECOVER: //breakまでにすべてのパケットtypeを記載
    case JBRTYPE_HELLO:{
        m_type = (MessageType) type;
        break;
      }
    default:
      m_valid = false;
    }
  uint32_t dist = i.GetDistanceFrom (start);
  NS_ASSERT (dist == GetSerializedSize ());
  return dist;
}

void
TypeHeader::Print (std::ostream &os) const
{
  switch (m_type)
    {
      // case JBRTYPE_RREQ: {
      //   os << "RREQ";
      //   break;
      // }
      // case JBRTYPE_RREP: {
      //   os << "RREP";
      //   break;
      // }
      // case JBRTYPE_RERR: {
      //   os << "RERR";
      //   break;
      // }
      // case JBRTYPE_RREP_ACK: {
      //   os << "RREP_ACK";
      //   break;
      // }
      case JBRTYPE_HELLO: {
        os << "HELLO";
        break;
      }
      case JBRTYPE_RECOVER: {
        os << "RECOVER";
        break;
      }
    default:
      os << "UNKNOWN_TYPE";
    }
}

bool
TypeHeader::operator== (TypeHeader const &o) const
{
  return (m_type == o.m_type && m_valid == o.m_valid);
}

std::ostream &
operator<< (std::ostream &os, TypeHeader const &h)
{
  h.Print (os);
  return os;
}


// ***********************start Jbr_HELLO*************************************//
HelloHeader::HelloHeader (int32_t nodeid, int32_t posx, int32_t posy)
    : m_nodeid (nodeid), m_posx (posx), m_posy (posy)
{
}
NS_OBJECT_ENSURE_REGISTERED (HelloHeader);

TypeId
HelloHeader::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::jbr::HelloHeader")
                          .SetParent<Header> ()
                          .SetGroupName ("Jbr")
                          .AddConstructor<HelloHeader> ();
  return tid;
}

TypeId
HelloHeader::GetInstanceTypeId () const
{
  return GetTypeId ();
}

uint32_t
HelloHeader::GetSerializedSize () const
{
  return 12;
}

void
HelloHeader::Serialize (Buffer::Iterator i) const //シリアル化
{
  i.WriteHtonU32 (m_nodeid);
  i.WriteHtonU32 (m_posx);
  i.WriteHtonU32 (m_posy);
}

uint32_t
HelloHeader::Deserialize (Buffer::Iterator start) //逆シリアル化
{
  Buffer::Iterator i = start;

  m_nodeid = i.ReadNtohU32 ();
  m_posx = i.ReadNtohU32 ();
  m_posy = i.ReadNtohU32 ();

  uint32_t dist2 = i.GetDistanceFrom (start);

  NS_ASSERT (dist2 == GetSerializedSize ());
  return dist2;
}

void
HelloHeader::Print (std::ostream &os) const
{
  // os << "NodeId " << m_nodeid;
  // os << "NodePointX " << m_posx;
  // os << "NodePointY" << m_posy;
}
std::ostream &
operator<< (std::ostream &os, HelloHeader const &h)
{
  h.Print (os);
  return os;
}

// ***********************end Jbr_HELLO*************************************//


// *********************** jbr recovery unicast*****************************//
JbrHeader::JbrHeader (int32_t send_id, int32_t send_x, int32_t send_y,
  int32_t next_id, int32_t local_source_x, int32_t local_source_y,
  int32_t previous_x, int32_t previous_y, int32_t des_id,
  int32_t des_x, int32_t des_y, int32_t hop)
: m_send_id (send_id), m_send_x (send_x), m_send_y (send_y), m_next_id (next_id),
m_local_source_x (local_source_x), m_local_source_y (local_source_y), 
m_previous_x (previous_x), m_previous_y (previous_y), m_des_id (des_id),
m_des_x (des_x), m_des_y (des_y), m_hop(hop)
{
}
NS_OBJECT_ENSURE_REGISTERED (JbrHeader);

TypeId
JbrHeader::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::jbr::JbrHeader")
                          .SetParent<Header> ()
                          .SetGroupName ("Jbr")
                          .AddConstructor<JbrHeader> ();
  return tid;
}

TypeId
JbrHeader::GetInstanceTypeId () const
{
  return GetTypeId ();
}

uint32_t
JbrHeader::GetSerializedSize () const
{
  return 48;
}

void
JbrHeader::Serialize (Buffer::Iterator i) const //シリアル化
{
  i.WriteHtonU32 (m_send_id);
  i.WriteHtonU32 (m_send_x);
  i.WriteHtonU32 (m_send_y);
  i.WriteHtonU32 (m_next_id);
  i.WriteHtonU32 (m_local_source_x);
  i.WriteHtonU32 (m_local_source_y);
  i.WriteHtonU32 (m_previous_x);
  i.WriteHtonU32 (m_previous_y);
  i.WriteHtonU32 (m_des_id);
  i.WriteHtonU32 (m_des_x);
  i.WriteHtonU32 (m_des_y);
  i.WriteHtonU32 (m_hop);
}

uint32_t
JbrHeader::Deserialize (Buffer::Iterator start) //逆シリアル化
{
  Buffer::Iterator i = start;

  m_send_id = i.ReadNtohU32 ();
  m_send_x = i.ReadNtohU32 ();
  m_send_y = i.ReadNtohU32 ();
  m_next_id = i.ReadNtohU32 ();
  m_local_source_x = i.ReadNtohU32 ();
  m_local_source_y = i.ReadNtohU32 ();  
  m_previous_x = i.ReadNtohU32 ();
  m_previous_y = i.ReadNtohU32 ();
  m_des_id = i.ReadNtohU32 ();
  m_des_x = i.ReadNtohU32 ();
  m_des_y = i.ReadNtohU32 ();
  m_hop = i.ReadNtohU32 ();

  uint32_t dist2 = i.GetDistanceFrom (start);

  NS_ASSERT (dist2 == GetSerializedSize ());
  return dist2;
}

void
JbrHeader::Print (std::ostream &os) const
{
  // os << "NodeId " << m_nodeid;
  // os << "NodePointX " << m_posx;
  // os << "NodePointY" << m_posy;
}
std::ostream &
operator<< (std::ostream &os, JbrHeader const &h)
{
  h.Print (os);
  return os;
}

// ***********************end recovery unicast*************************************//



} // namespace jbr
} // namespace ns3
