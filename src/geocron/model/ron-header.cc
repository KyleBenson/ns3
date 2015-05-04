/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/uinteger.h"
#include "ron-header.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("RonHeader");
NS_OBJECT_ENSURE_REGISTERED (RonHeader);

RonHeader::RonHeader ()
{
  NS_LOG_FUNCTION_NOARGS ();

  m_forward = false;
  m_nHops = 0;
  m_origin = 0;
  m_seq = 0;

  m_dest = 0;
}

RonHeader::RonHeader (Ipv4Address destination, Ipv4Address intermediate /*= Ipv4Address((uint32_t)0)*/)
  : m_nHops (0), m_seq (0), m_origin (0)
{
  NS_LOG_FUNCTION (destination << " and " << intermediate);

  m_dest = destination.Get ();
  
  if (intermediate.Get () != 0)
    {
      AddDest (intermediate);
      m_forward = true;
    }
  else
    {
      m_forward = false;
    }
}

  /*RonHeader::RonHeader (const RonHeader& original)
{
  NS_LOG_FUNCTION (original.GetFinalDest ());

  m_forward = original.m_forward;
  m_nHops = original.m_nHops;
  m_nIps = original.m_nIps;
  m_seq = original.m_seq;
  m_dest = original.m_dest;
  m_origin = original.m_origin;
  
  // Deep copy buffer
  if (original.m_ips)
    {
      m_ips = new uint32_t[m_nIps];
      memcpy (m_ips, original.m_ips, m_nIps*4);
    }
  else
    {
      m_ips = NULL;
    }
    }*/

RonHeader&
RonHeader::operator=(const RonHeader& original)
{
  NS_LOG_FUNCTION_NOARGS ();

  RonHeader temp ((const RonHeader)original);

  std::swap(m_forward, temp.m_forward);
  std::swap(m_nHops, temp.m_nHops);
  std::swap(m_seq, temp.m_seq);
  std::swap(m_dest, temp.m_dest);
  std::swap(m_origin, temp.m_origin);
  std::swap(m_ips, temp.m_ips);

  return *this;
}

/*
RonHeader::RonHeader (Ipv4Address destination, 
{
  RonHeader (destination);
  m_forward = true;
  m_nIps = 1;
  m_ips = new uint32_t[1];
  m_ips[0] = intermediate.Get ();
  }*/

TypeId
RonHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RonHeader")
    .SetParent<Header> ()
    .AddConstructor<RonHeader> ()
  ;
  return tid;
}

TypeId
RonHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint8_t
RonHeader::GetHop (void) const
{
  return m_nHops;
}

Ipv4Address
RonHeader::GetFinalDest (void) const
{
  return Ipv4Address (m_dest);
}
 
Ipv4Address
RonHeader::GetNextDest (void) const
{
  NS_LOG_FUNCTION_NOARGS ();

  if (m_nHops < m_ips.size ())
    {
      uint32_t next = m_ips[m_nHops];
      return Ipv4Address (next);
    }
  else
    return GetFinalDest ();
}

Ipv4Address
RonHeader::GetOrigin (void) const
{
  return Ipv4Address (m_origin);
}

uint8_t
RonHeader::IncrHops (void)
{
  return ++m_nHops;
}

uint32_t
RonHeader::GetSeq (void) const
{
  return m_seq;
}

void
RonHeader::SetSeq (uint32_t seq)
{
  m_seq = seq;
}

bool
RonHeader::IsForward (void) const
{
  return m_forward;
}

void
RonHeader::AddDest (Ipv4Address addr)
{
  NS_LOG_FUNCTION (addr);
  if (!m_forward)
    m_forward = true;

  uint32_t next = addr.Get ();
  m_ips.push_back (next);
}

void
RonHeader::SetDestination (Ipv4Address dest)
{
  m_dest = dest.Get ();
}

void
RonHeader::SetOrigin (Ipv4Address origin)
{
  m_origin = origin.Get ();
}

RonHeader::PathIterator
RonHeader::GetPathBegin () const
{
  return m_ips.begin ();
}

RonHeader::PathIterator
RonHeader::GetPathEnd () const
{
  return m_ips.end ();
}

Ptr<RonPath>
RonHeader::GetPath () const
{
  // Add each of the peers' IP addresses, looked up using the RonPeerTable master table, to a RonPath object
  Ptr<RonPath> path = Create<RonPath> ();
  Ptr<RonPeerEntry> peer;
  for (PathIterator addrItr = GetPathBegin ();
       addrItr != GetPathEnd (); addrItr++)
    {
      peer = RonPeerTable::GetMaster ()->GetPeerByAddress ((Ipv4Address)*addrItr);
      NS_ASSERT_MSG (peer != NULL, "null peer in master table with address " << *addrItr);
      path->AddHop (peer);
    }
  peer = RonPeerTable::GetMaster ()->GetPeerByAddress ((Ipv4Address)GetFinalDest ());

  //if (GetFinalDest ().Get () == 0)
  //{
    //NS_LOG_WARN ("Final destination in RonHeader is 0.0.0.0!  Returning a dummy final destination in GetPath ()");
    //peer = CreateObject<RonPeerEntry> ();
    //peer->address = (Ipv4Address)(uint32_t)0;
  //}

  NS_ASSERT_MSG (peer, "In GetPath, final destination peer at " << GetFinalDest () << " couldn't be found in master table.");

  path->AddHop (peer);
  return path;
}

void
RonHeader::SetPath (Ptr<RonPath> path)
{
  NS_ASSERT_MSG (path->GetN (), "Can't set an empty path in RonHeader!");

  // for now, we only want to add the path up to destination as that is stored separately
  // TODO: simply store a whole path duh!
  // start by adding first hop...
  RonPath::Iterator itr = path->Begin ();

  //loop up until we have one left
  for (; (itr != path->End ()) and (*itr != path->GetDestination ()); itr++)
    {
      AddDest ((*(*itr)->Begin ())->address);
    }
  SetDestination ((*(*itr)->Begin ())->address);

  //ensure correct path size, which should include the Destination
  NS_ASSERT (path->GetN () == m_ips.size () + 1);
}

void
RonHeader::ReversePath (void)
{
  NS_LOG_FUNCTION_NOARGS ();

  // Iterate over buffer and swap elements
  for (uint8_t i = 0; i < m_ips.size ()/2; i++)
    {
      uint32_t tmp = m_ips[i];
      m_ips[i] = m_ips[m_ips.size () - 1 - i];
      m_ips[m_ips.size () - 1 - i] = tmp;
    }

  uint32_t oldDest = m_dest;
  m_dest = m_origin;
  m_origin = oldDest;
  m_nHops = 0;
}

void
RonHeader::Print (std::ostream &os) const
{
  // This method is invoked by the packet printing
  // routines to print the content of my header.
  //os << "data=" << m_data << std::endl;
  os << "Packet " << m_seq << " from " << Ipv4Address (m_origin) << " to " << Ipv4Address (m_dest);
  if (m_forward)
    {
      os << " on hop # " << m_nHops << " with next destination " << GetNextDest ();
    }
}

uint32_t
RonHeader::GetSerializedSize (void) const
{
  // we reserve 2 bytes for our header.
  return RON_HEADER_SIZE(m_ips.size ());
}

void
RonHeader::Serialize (Buffer::Iterator start) const
{
  NS_LOG_FUNCTION_NOARGS ();

  start.WriteU8 (m_forward);
  start.WriteU8 (m_nHops);
  start.WriteU8 ((unsigned char)m_ips.size ());
  start.WriteU32 (m_seq);
  start.WriteU32 (m_dest);
  start.WriteU32 (m_origin);
  
  for (PathIterator hops = GetPathBegin (); hops != GetPathEnd (); hops++)
    {
      start.WriteU32 (*hops);
    }
}

uint32_t
RonHeader::Deserialize (Buffer::Iterator start)
{
  NS_LOG_FUNCTION_NOARGS ();

  m_forward = (bool)start.ReadU8 ();
  m_nHops = start.ReadU8 ();
  uint8_t nIps = start.ReadU8 ();
  m_seq = start.ReadU32 ();
  m_dest = start.ReadU32 ();
  m_origin = start.ReadU32 ();

  if (m_ips.size ())
    m_ips.clear ();
  
  for (uint32_t i = 0; i < (uint32_t)nIps; i++)
    {
      m_ips.push_back (start.ReadU32 ());
    }

  NS_ASSERT (nIps == m_ips.size ());

  // we return the number of bytes effectively read.
  return RON_HEADER_SIZE(m_ips.size ());
}

} //namespace ns3
