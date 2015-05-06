/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright 2007 University of Washington
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
 */

#include "ron-peer-table.h"
#include "failure-helper-functions.h"
#include "geocron-experiment.h"

#include <boost/range/functions.hpp>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE ("RonPeerTable");

RonPeerEntry::RonPeerEntry () {}

RonPeerEntry::RonPeerEntry (Ptr<Node> node)
{
    this->node = node;
    id = node-> GetId ();
    address = GetNodeAddress(node);

    NS_ASSERT_MSG (address.Get () != (uint32_t)0, "got a 0 Ipv4Address!");

    Ptr<MobilityModel> mobility = node->GetObject<MobilityModel> ();
    NS_ASSERT_MSG (mobility != NULL, "Geocron nodes need MobilityModels for determining locations!");    
    location = mobility->GetPosition ();

    //NS_ASSERT_MSG (location.x != 0 or location.y != 0, "location is 0,0!");
}

TypeId
RonPeerEntry::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::RonPeerEntry")
    .SetParent<Object> ()
    .AddConstructor<RonPeerEntry> ()
  ;
  return tid;
}


bool
RonPeerEntry::operator== (const RonPeerEntry rhs) const
{
  return id == rhs.id;
}


bool
RonPeerEntry::operator!= (const RonPeerEntry rhs) const
{
  return !(*this == rhs);
}

bool
RonPeerEntry::operator< (RonPeerEntry rhs) const
{
  return id < rhs.id;
}

std::ostream&
operator<< (std::ostream& os, RonPeerEntry const& peer)
{
  return os << peer.id;
}

////////////////////////////////////////////////////////////////////////////////

Ptr<RonPeerTable> RonPeerTable::m_master = NULL;

Ptr<RonPeerTable>
RonPeerTable::GetMaster ()
{
  if (m_master == NULL)
    m_master = Create<RonPeerTable> ();
  return m_master;
}


uint32_t
RonPeerTable::GetN () const
{
  NS_ASSERT_MSG (m_peersByAddress.size () == m_peers.size (), "Corrupted peer table (sizes not same)!");
  return m_peers.size ();
}


bool
RonPeerTable::operator== (const RonPeerTable rhs) const
{
  // if they're same size, all from one must be in the other (order not mattering)
  if (GetN () != rhs.GetN ())
      return false;
  for (ConstIterator itr = Begin ();
       itr != End (); itr++)
    {
      if (!rhs.IsInTable ((*itr)->id))
        return false;
      Ptr<RonPeerEntry> peer = rhs.GetPeer ((*itr)->id);
      if (*(*itr) != *peer)
        return false;
    }
  return true;
}


bool
RonPeerTable::operator!= (const RonPeerTable rhs) const
{
  return !(*this == rhs);
}



Ptr<RonPeerEntry>
RonPeerTable::AddPeer (Ptr<RonPeerEntry> entry)
{
  Ptr<RonPeerEntry> returnValue;
  // If this peer already exists in the table, update the pointer with the new object
  if (m_peers.count (entry->id) != 0)
    {
      Ptr<RonPeerEntry> temp = (*(m_peers.find (entry->id))).second;
      m_peers[entry->id] = entry;
      returnValue = temp;
      NS_LOG_LOGIC ("Changing peer " << entry->id << " with address " << entry->address << " in table.");
    }
  else
    returnValue = m_peers[entry->id] = entry;

  m_peersByAddress[(entry->address.Get ())] = entry;

  return returnValue;
}


Ptr<RonPeerEntry>
RonPeerTable::AddPeer (Ptr<Node> node)
{
  Ptr<RonPeerEntry> newEntry = Create<RonPeerEntry> (node);
  //Ptr<RonPeerEntry> newEntry = GetMaster ()->GetPeer (node->GetId ());
  //if (newEntry == NULL)
  //{
    //newEntry = Create<RonPeerEntry> (node);
    // Cache the peer we create here since we're assuming PeerEntries are essentially const objects
    //GetMaster ()->AddPeer (newEntry);
    //NS_LOG_LOGIC ("Caching peer with ID " << newEntry->id << " and IP address " << newEntry->address);
  //}
  return AddPeer (newEntry);
}


void
RonPeerTable::AddPeers (NodeContainer nodes)
{
  for (NodeContainer::Iterator nodeItr = nodes.Begin ();
      nodeItr != nodes.End (); nodeItr++)
  {
    AddPeer (*nodeItr);
  }
}


void
RonPeerTable::AddPeers (Ptr<RonPeerTable> peers)
{
  for (RonPeerTable::Iterator itr = peers->Begin ();
      itr != peers->End (); itr++)
  {
    AddPeer (*itr);
  }
}


bool
RonPeerTable::RemovePeer (uint32_t id)
{
  bool retValue = false;
  if (m_peers.count (id))
    retValue = true;
  Ipv4Address addr = GetPeer (id)->address;
  m_peers.erase (m_peers.find (id));
  m_peersByAddress.erase (m_peersByAddress.find (addr.Get ()));
  return retValue;
}


Ptr<RonPeerEntry>
RonPeerTable::GetPeer (uint32_t id) const
{
  if (m_peers.count (id))
    return  ((*(m_peers.find (id))).second);
  else
    return NULL;
}


Ptr<RonPeerEntry>
RonPeerTable::GetPeerByAddress (Ipv4Address address) const
{
  if (m_peersByAddress.count (address.Get ()))
    //bracket operator isn't const
    return m_peersByAddress.at (address.Get ());
  else
    return NULL;
}


bool
RonPeerTable::IsInTable (uint32_t id) const
{
  return m_peers.count (id);
}


bool
RonPeerTable::IsInTable (Ptr<RonPeerEntry> peer) const
{
  return IsInTable (peer->id);
}


bool
RonPeerTable::IsInTable (Iterator itr) const
{
  return IsInTable ((*itr)->id);
}


void
RonPeerTable::Clear ()
{
  m_peers.clear ();
  m_peersByAddress.clear ();
}


RonPeerTable::Iterator
RonPeerTable::Begin ()
{
  return boost::begin (boost::adaptors::values (m_peers));
/*template<typename T1, typename T2> T2& take_second(const std::pair<T1, T2> &a_pair)
{
  return a_pair.second;
}
boost::make_transform_iterator(a_map.begin(), take_second<int, string>),*/
}


RonPeerTable::Iterator
RonPeerTable::End ()
{
  return boost::end (boost::adaptors::values (m_peers));
}


RonPeerTable::ConstIterator
RonPeerTable::Begin () const
{
  return boost::begin (boost::adaptors::values (m_peers));
}


RonPeerTable::ConstIterator
RonPeerTable::End () const
{
  return boost::end (boost::adaptors::values (m_peers));
}

} // namespace ns3
