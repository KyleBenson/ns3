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

#ifndef RON_PATH_H
#define RON_PATH_H

#include "ron-peer-table.h"
#include "ns3/core-module.h"
#include <list>

namespace ns3 {

  /** This subclass represents a single hop along the overlay path,
      which may be unicast, multicast, or broadcast and holds some other parameters
      related to that particular hop, such as probabilities for probabilistic algorithms, etc. */
class PeerDestination : public SimpleRefCount<PeerDestination>
{
private:
  typedef std::list<Ptr<RonPeerEntry> > DestinationContainer;
  DestinationContainer m_peers;
public:
  PeerDestination (Ptr<RonPeerEntry> peer/*, flags = 0*/);
  //TODO: flags for diff casts
  void AddPeer (Ptr<RonPeerEntry> peer/*, flags = 0*/);
  uint32_t GetN () const;

  bool operator==(const PeerDestination rhs) const;
  bool operator!=(const PeerDestination rhs) const;
  
  typedef DestinationContainer::iterator Iterator;
  typedef DestinationContainer::const_iterator ConstIterator;
  Iterator Begin ();
  Iterator End ();
  ConstIterator Begin () const;
  ConstIterator End () const;
};

/** This class provides a simple iterable container for storing the path of peer entries. */
class RonPath : public SimpleRefCount<RonPath>
{
private:
  typedef std::list<Ptr<PeerDestination> > underlyingContainer;
  underlyingContainer m_path;

public:
  RonPath();
  RonPath(Ptr<RonPeerEntry> peer);
  RonPath(Ptr<PeerDestination> dest);

  typedef underlyingContainer::iterator Iterator;
  typedef underlyingContainer::const_iterator ConstIterator;

  /** Adds a PeerDestination at the point in the path pointed to by index,
      which defaults to the end of the path. */
  void AddHop (Ptr<PeerDestination> dest, Iterator index = (Iterator)NULL);
  void AddHop (Ptr<RonPeerEntry> dest, Iterator index = (Iterator)NULL);

  /** Returns the final destination peer(s), which is currently just the last added destination. */
  //TODO: labels to identify which hop... or is that just peerID?
  Ptr<PeerDestination> GetDestination () const;

  uint32_t GetN () const;

  //TODO: handle splicing/forking/adding destinations instead of peers

  //TODO: question: should the path include the source?

  /** Reverses the ordering of the path. */
  void Reverse ();

  /** Compare paths based on the contents of the pointers they contain,
      in case there exist multiple references to the same RonPeerEntry. */
  bool operator== (const RonPath rhs) const;
  bool operator!=(const RonPath rhs) const;

  /** Compare paths based on the contents of the pointers they contain,
      in case there exist multiple references to the same RonPeerEntry.
      In the event of a tie, the shorter path is considered less. */
  bool operator< (RonPath  rhs) const;

  ConstIterator Begin () const;
  ConstIterator End () const;
  Iterator Begin ();
  Iterator End ();
};

std::ostream& operator<< (std::ostream& os, RonPath const& path);
std::ostream& operator<< (std::ostream& os, PeerDestination const& dest);
}
#endif //RON_PATH_H
