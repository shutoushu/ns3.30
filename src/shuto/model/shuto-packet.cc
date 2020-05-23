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
#include "shuto-packet.h"
#include "ns3/address-utils.h"
#include "ns3/packet.h"

namespace ns3 {
namespace shuto {

NS_OBJECT_ENSURE_REGISTERED (TypeHeader);

TypeHeader::TypeHeader (MessageType t)
  : m_type (t),
    m_valid (true)
{
}

TypeId
TypeHeader::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::shuto::TypeHeader")
    .SetParent<Header> ()
    .SetGroupName ("Shuto")
    .AddConstructor<TypeHeader> ()
  ;
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
    //case SHUTOTYPE_RREQ:
    //case SHUTOTYPE_RREP:
    //case SHUTOTYPE_RERR:
    //case SHUTOTYPE_RREP_ACK:
    case SHUTOTYPE_DANGER:
    case SHUTOTYPE_HELLOID:
      {
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
      /*
    case SHUTOTYPE_RREQ:
      {
        os << "RREQ";
        break;
      }
    case SHUTOTYPE_RREP:
      {
        os << "RREP";
        break;
      }
    case SHUTOTYPE_RERR:
      {
        os << "RERR";
        break;
      }
    case SHUTOTYPE_RREP_ACK:
      {
        os << "RREP_ACK";
        break;
      }
      */
    case SHUTOTYPE_DANGER:
      {
        os << "DANGER";
        break;
      }
    case SHUTOTYPE_HELLOID:
      {
        os<< "HELLOID";
        break;
      }  
    default:
      os << "UNKNOWN_TYPE";
    }
}

bool
TypeHeader::operator== (TypeHeader const & o) const
{
  return (m_type == o.m_type && m_valid == o.m_valid);
}

std::ostream &
operator<< (std::ostream & os, TypeHeader const & h)
{
  h.Print (os);
  return os;
}




//----------------------------------------------------------------------------
//ID
//----------------------------------------------------------------------------



IdHeader::IdHeader (uint32_t id ,uint32_t xpoint,uint32_t ypoint)
: m_helloid (id),
  m_helloxpoint(xpoint),
  m_helloypoint(ypoint)
{
 
}


NS_OBJECT_ENSURE_REGISTERED (IdHeader);

TypeId
IdHeader::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::shuto::IdHeader")
    .SetParent<Header> ()
    .SetGroupName ("Shuto")
    .AddConstructor<IdHeader> ()
  ;
  return tid;
}

TypeId
IdHeader::GetInstanceTypeId () const
{
  return GetTypeId ();
}

uint32_t
IdHeader::GetSerializedSize () const
{
  //return 10;
  return 12;
}

void
IdHeader::Serialize (Buffer::Iterator i) const  //シリアル化
{
  i.WriteHtonU32 (m_helloid);
  i.WriteHtonU32 (m_helloxpoint);
  i.WriteHtonU32 (m_helloypoint);
  //std::cout<<"serialize"<<"\n\n";

}

uint32_t
IdHeader::Deserialize (Buffer::Iterator start)   //逆シリアル化
{
  Buffer::Iterator i = start;

  m_helloid = i.ReadNtohU32 ();
  m_helloxpoint = i.ReadNtohU32 ();
  m_helloypoint = i.ReadNtohU32 ();
  //std::cout <<"deserialize m_helloid"<<m_helloid<<"\n\n";

  uint32_t dist3 = i.GetDistanceFrom (start);

  //std::cout<< "   Id   header       getdistanceFrom  dst3="<<dist3<<"\n\n";
  NS_ASSERT (dist3 == GetSerializedSize ());
  return dist3;
}

void
IdHeader::Print (std::ostream &os) const
{
  os << "m_helloid " << m_helloid ;
  os << "m_helloxpoint " << m_helloxpoint ;
  os << "m_helloypoint " << m_helloypoint ;
  
}
std::ostream &
operator<< (std::ostream & os, IdHeader const & h)
{
  h.Print (os);
  return os;
}












//----------------------------------------------------------------------------
//danger
//----------------------------------------------------------------------------

DangerHeader::DangerHeader (uint32_t nodeid, uint32_t posx, uint32_t posy, uint8_t hopcount, uint32_t recvtime, uint8_t danger)
   : m_nodeid(nodeid),
     m_posx(posx),
     m_posy(posy),
     m_hopcount((unsigned)hopcount),
     m_recvtime(recvtime),
     m_danger((unsigned)danger) 
{
  
  //std::cout<<"danger = "<<(unsigned)m_danger<<"\n";
  //std::cout<<"hopcount = "<<(unsigned)m_hopcount<<"\n";
  //std::cout<<"recvtime = "<<m_recvtime<<"\n";

  //uint8_t test = 1;
   //std::cout<<"test = "<<(unsigned)test<<"\n";

  /*std::cout<<"constrouctor-------------------------------------------------------\n";
  std::cout<<"constrouctor m_point"<<m_point<<"\n";
  std::cout<<"constrouctor m_hopcount"<<m_hopcount<<"\n";
  std::cout<<"constrouctor m_danger"<<m_danger<<"\n";
  std::cout<<"constrouctor-------------------------------------------------------\n";
  */
}
NS_OBJECT_ENSURE_REGISTERED (DangerHeader);

TypeId
DangerHeader::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::shuto::DangerHeader")
    .SetParent<Header> ()
    .SetGroupName ("Shuto")
    .AddConstructor<DangerHeader> ()
  ;
  return tid;
}

TypeId
DangerHeader::GetInstanceTypeId () const
{
  return GetTypeId ();
}

uint32_t
DangerHeader::GetSerializedSize () const
{
 // return 19;
  //return 14;
  return  18;
 // return 24;
}

void
DangerHeader::Serialize (Buffer::Iterator i) const  //シリアル化
{
  i.WriteHtonU32 (m_nodeid);
  i.WriteHtonU32 (m_posx);
  i.WriteHtonU32 (m_posy);
  i.WriteU8(m_hopcount);
  i.WriteU8 (m_danger);
  i.WriteHtonU32 (m_recvtime);
  

  
  //std::cout<<"     DangerHeader               serialize \n";
}

uint32_t
DangerHeader::Deserialize (Buffer::Iterator start)   //逆シリアル化
{
  Buffer::Iterator i = start;
  
  m_nodeid = i.ReadNtohU32 ();
  m_posx = i.ReadNtohU32 ();
  m_posy = i.ReadNtohU32 ();
  m_hopcount = i.ReadU8 ();
  m_danger = i.ReadU8 ();
  m_recvtime = i.ReadNtohU32 ();

  uint32_t dist2 = i.GetDistanceFrom (start);

  NS_ASSERT (dist2 == GetSerializedSize ());
  return dist2;
}

void
DangerHeader::Print (std::ostream &os) const
{
  os << "NodeId " << m_nodeid << "NodePointX " <<m_posx;
  os << "NodePointY" << m_posy << "HopValue" << m_hopcount;
  os <<  "ReceiveTime" << m_recvtime<< "DangerValue" << m_danger;
}
/*
void
DangerHeader::SetRecvTime (uint32_t recvtime)
{
  m_recvtime = recvtime;
}

uint32_t
DangerHeader::GetRecvTime () 
{
  //Time t (MilliSeconds (m_recvtime));
  return m_recvtime;
}
*/
std::ostream &
operator<< (std::ostream & os, DangerHeader const & h)
{
  h.Print (os);
  return os;
}








/*

//-----------------------------------------------------------------------------
// RREP
//-----------------------------------------------------------------------------


RrepHeader::RrepHeader (uint8_t prefixSize, uint8_t hopCount, Ipv4Address dst,
                        uint32_t dstSeqNo, Ipv4Address origin, Time lifeTime)
  : m_flags (0),
    m_prefixSize (prefixSize),
    m_hopCount (hopCount),
    m_dst (dst),
    m_dstSeqNo (dstSeqNo),
    m_origin (origin)
{
  m_lifeTime = uint32_t (lifeTime.GetMilliSeconds ());
}




NS_OBJECT_ENSURE_REGISTERED (RrepHeader);

TypeId
RrepHeader::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::shuto::RrepHeader")
    .SetParent<Header> ()
    .SetGroupName ("Shuto")
    .AddConstructor<RrepHeader> ()
  ;
  return tid;
}

TypeId
RrepHeader::GetInstanceTypeId () const
{
  return GetTypeId ();
}

uint32_t
RrepHeader::GetSerializedSize () const
{
  //return 19;
  return 19;
}

void
RrepHeader::Serialize (Buffer::Iterator i) const
{
  i.WriteU8 (m_flags);
  i.WriteU8 (m_prefixSize);
  i.WriteU8 (m_hopCount);
  WriteTo (i, m_dst);
  i.WriteHtonU32 (m_dstSeqNo);
  WriteTo (i, m_origin);
  i.WriteHtonU32 (m_lifeTime);
  //std::cout<<" serialize \n";
  
}

uint32_t
RrepHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;

  m_flags = i.ReadU8 ();//1byte
  m_prefixSize = i.ReadU8 ();//1byte
  m_hopCount = i.ReadU8 ();//1byte
  ReadFrom (i, m_dst);  // 4byte
  m_dstSeqNo = i.ReadNtohU32 ();//4byte
  ReadFrom (i, m_origin);    //4byte
  m_lifeTime = i.ReadNtohU32 ();//4byre
  
  uint32_t dist = i.GetDistanceFrom (start);

  std::cout<<" getdistanceFrom -- rrepheader"<<dist<<"\n\n";
  return dist;
}

void
RrepHeader::Print (std::ostream &os) const
{
  os << "destination: ipv4 " << m_dst << " sequence number " << m_dstSeqNo;
  if (m_prefixSize != 0)
    {
      os << " prefix size " << m_prefixSize;
    }
  os << " source ipv4 " << m_origin << " lifetime " << m_lifeTime
     << " acknowledgment required flag " << (*this).GetAckRequired ();
}

void
RrepHeader::SetLifeTime (Time t)
{
  m_lifeTime = t.GetMilliSeconds ();
}

Time
RrepHeader::GetLifeTime () const
{
  Time t (MilliSeconds (m_lifeTime));
  return t;
}

void
RrepHeader::SetAckRequired (bool f)
{
  if (f)
    {
      m_flags |= (1 << 6);
    }
  else
    {
      m_flags &= ~(1 << 6);
    }
}

bool
RrepHeader::GetAckRequired () const
{
  return (m_flags & (1 << 6));
}

void
RrepHeader::SetPrefixSize (uint8_t sz)
{
  m_prefixSize = sz;
}

uint8_t
RrepHeader::GetPrefixSize () const
{
  return m_prefixSize;
}

bool
RrepHeader::operator== (RrepHeader const & o) const
{
  return (m_flags == o.m_flags && m_prefixSize == o.m_prefixSize
          && m_hopCount == o.m_hopCount && m_dst == o.m_dst && m_dstSeqNo == o.m_dstSeqNo
          && m_origin == o.m_origin && m_lifeTime == o.m_lifeTime);
}

void
RrepHeader::SetHello (Ipv4Address origin, uint32_t srcSeqNo, Time lifetime)
{
  m_flags = 0;
  m_prefixSize = 0;
  m_hopCount = 0;
  m_dst = origin;
  m_dstSeqNo = srcSeqNo;
  m_origin = origin;
  m_lifeTime = lifetime.GetMilliSeconds ();
}

std::ostream &
operator<< (std::ostream & os, RrepHeader const & h)
{
  h.Print (os);
  return os;
}
*/
}
}
