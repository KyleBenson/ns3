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

#ifndef RON_PEER_TABLE_H
#define RON_PEER_TABLE_H

#include "ns3/internet-module.h"
#include "ns3/core-module.h"
#include "ns3/mobility-module.h"

#include "region.h"

#include <boost/range/adaptor/map.hpp>
#include <boost/unordered_map.hpp>
#include <map>

//TODO: enum for choosing which heuristic?

namespace ns3 {

/** We make this an object so that we can aggregate one to each node. */
class RonPeerEntry : public Object
{
public:
  RonPeerEntry ();
  RonPeerEntry (Ptr<Node> node);
  static TypeId GetTypeId ();

  bool operator== (RonPeerEntry rhs) const;
  bool operator!= (RonPeerEntry rhs) const;
  bool operator< (RonPeerEntry rhs) const;

  //Ron Attributes
  uint32_t id;
  Ipv4Address address;
  Ptr<Node> node;
  Time lastContact;
  //TODO: label enum for grouping?

  //Geocron Attributes
  Vector location;
  Location region;
  //TODO: failures reported counter / timer
};  

class RonPeerTable : public SimpleRefCount<RonPeerTable>
{
 private:
  //typedef boost::unordered_map<uint32_t, Ptr<RonPeerEntry> > underlyingMapType;
  typedef boost::unordered_map<uint32_t, Ptr<RonPeerEntry> > underlyingMapType;
  typedef boost::range_detail::select_second_mutable_range<underlyingMapType> underlyingIterator;
  typedef boost::range_detail::select_second_const_range<underlyingMapType> underlyingConstIterator;
 public:
  typedef boost::range_iterator<underlyingIterator>::type Iterator;
  typedef boost::range_iterator<underlyingConstIterator>::type ConstIterator;

  /** We store all of the peers in a single master peer table for efficiency purposes.
      This should save a lot on memory and
      TODO will incorporate a 'mirroring' feature
      in which subsets are generated efficiently and contain copy-on-write peer entries. */
  static Ptr<RonPeerTable> GetMaster ();

  bool operator== (const RonPeerTable rhs) const;
  bool operator!= (const RonPeerTable rhs) const;

  uint32_t GetN () const;

  /** Returns old entry if it existed, else new one. */
  Ptr<RonPeerEntry> AddPeer (Ptr<RonPeerEntry> entry);
  /** Returns old entry if it existed, else new one. */
  Ptr<RonPeerEntry> AddPeer (Ptr<Node> node);
  /** Returns true if entry existed. */
  bool RemovePeer (uint32_t id);
  /** Returns requested entry, NULL if unavailable. Use IsInTable to verify its prescence in the table. */
  Ptr<RonPeerEntry> GetPeer (uint32_t id) const;
  /** Returns requested entry, NULL if unavailable. Use IsInTable to verify its prescence in the table. */
  Ptr<RonPeerEntry> GetPeerByAddress (Ipv4Address address) const;

  bool IsInTable (uint32_t id) const;
  bool IsInTable (Iterator itr) const;

  /** Drop all RonPeerEntry entries from the table, updating all data structures in the table accordingly. */
  void Clear ();

  //TODO: other forms of get/remove
  Iterator Begin ();
  Iterator End ();
  ConstIterator Begin () const;
  ConstIterator End () const;
  static Ptr<RonPeerTable> m_master;

 private:
  underlyingMapType m_peers;
  boost::unordered_map<uint32_t, Ptr<RonPeerEntry> > m_peersByAddress;
};

} //namespace
#endif //RON_PEER_TABLE_H
