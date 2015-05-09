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

#ifndef PHYSICAL_PATH_H
#define PHYSICAL_PATH_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ipv4-interface-container.h"
#include "ron-path.h"
#include <list>

namespace ns3 {


  // We use these to represent Links in a topology.
  // Note that we'll also use Ptr<Channel> at times
  typedef Ipv4InterfaceContainer LinkContainer;
  typedef std::pair<Ptr<Ipv4>, uint32_t> Link;

/** This class provides a simple iterable container for storing the path of physical
 * network elements, including both links and nodes.  Note that, unlike a RonPath,
 * the source node and link ARE included in this path. */
class PhysicalPath : public SimpleRefCount<PhysicalPath>
{
private:
  LinkContainer m_links;
  NodeContainer m_nodes;

public:
  PhysicalPath();
  PhysicalPath(Ptr<RonPeerEntry> source, Ptr<RonPath> other);

  /** Adds a node and link to the path. */
  void AddHop (Ptr<Node> node, Link link);
  //void AddPath (Ptr<PhysicalPath> other);

  uint32_t GetNNodes () const;
  uint32_t GetNLinks () const;

  // Helper functions to be used by heuristics but contained here since
  // it's somewhat implementation-specific and clunky.  It's easier to
  // test if it's removed from the heuristics.
  /** Sets the path given the source peer and RonPath by looking at the
   * Ipv4NixVectorRouting objects of the nodes and gathering up the routes
   * they're expected to use.
   * Returns: true if the paths are found, false otherwise */
  bool SetPathFromRonPath (Ptr<RonPeerEntry> source, Ptr<RonPath> ronPath);

  /** Helper function for getting a channel given a Link. */
  Ptr<Channel> GetChannelForLink (const Link& link);

  /** Returns the total expected latency for the given path, taking into
   * account only link delays currently. */
  Time GetLatency ();

  /** Returns the number of components shared between the two paths,
   * including both links and nodes. */
  uint32_t GetIntersectionSize (Ptr<PhysicalPath> other);

  // Failure helper functions that can be implemented later if we
  // decide to remove the only existing logic for this from the
  // IdealRonPathHeuristic
  // IsLinkUp (uint32_t index);
  // IsNodeUp ...
  // AreLinksUp
  // AreNodesUp

  /** Reverses the ordering of the path. */
  //void Reverse ();

  /** Compare paths based on the contents of the pointers they contain */
  bool operator== (const PhysicalPath rhs) const;
  bool operator!=(const PhysicalPath rhs) const;

  /** Compare paths based on the contents of the pointers they contain
      In the event of a tie, the shorter path is considered less. */
  //bool operator< (PhysicalPath  rhs) const;

  NodeContainer::Iterator NodesBegin () const;
  NodeContainer::Iterator NodesEnd () const;
  LinkContainer::Iterator LinksBegin () const;
  LinkContainer::Iterator LinksEnd () const;
};

std::ostream& operator<< (std::ostream& os, PhysicalPath const& path);
}
#endif //PHYSICAL_PATH_H
