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

#include "ron-path.h"

using namespace ns3;

RonPath::RonPath()
{}

RonPath::RonPath(Ptr<RonPeerEntry> peer)
{
  AddHop (Create<PeerDestination> (peer));
}


RonPath::RonPath (Ptr<PeerDestination> dest)
{
  AddHop (dest);
}


void
RonPath::AddHop (Ptr<PeerDestination> dest, RonPath::Iterator index)
{
  if (index != (Iterator)NULL)
    {
      if (index != Begin ())
        NS_ASSERT_MSG (false, "You can't specify an index other than Begin() for inserting hops yet.");
      m_path.push_front (dest);
    }
  else
    m_path.push_back (dest);
}


void
RonPath::AddHop (Ptr<RonPeerEntry> dest, RonPath::Iterator index)
{
  AddHop (Create<PeerDestination> (dest), index);
}


void
RonPath::Reverse ()
{
  m_path.reverse ();
}


RonPath::ConstIterator
RonPath::Begin () const
{
  return m_path.begin ();
}


RonPath::ConstIterator
RonPath::End () const
{
  return m_path.end ();
}


RonPath::Iterator
RonPath::Begin ()
{
  return m_path.begin ();
}


RonPath::Iterator
RonPath::End ()
{
  return m_path.end ();
}


uint32_t
RonPath::GetN () const
{
  return m_path.size ();
}


Ptr<PeerDestination>
RonPath::GetDestination () const
{
  return m_path.back ();
}


bool
RonPath::operator== (const RonPath rhs) const
{
  if (GetN () != rhs.GetN ())
    return false;
  for (RonPath::ConstIterator itr1 = Begin (), itr2 = rhs.Begin ();
       itr1 != End () and itr2 != rhs.End (); itr1++,itr2++)
    {
      //compare the objects inside the pointers inside the iterators
      if (*(*itr1) != *(*itr2))
        return false;
    }
  return true;
}


bool
RonPath::operator!=(const RonPath rhs) const
{
  return !(*this == rhs);
}


bool
RonPath::operator< (const RonPath rhs) const
{
  RonPath::ConstIterator itr1 = Begin (), itr2 = rhs.Begin ();
  for (; itr1 != End () and itr2 != rhs.End (); itr1++,itr2++)
    {
      if ((*(*itr1)->Begin ())->id > (*(*itr2)->Begin ())->id)
        return false;
    }

  //check sizes
  if (GetN () > rhs.GetN ())
    return false;
  return true;
}


////////////////////////////////////////////////////////////////////////////////
////////////////////  PeerDestination  /////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


PeerDestination::PeerDestination (Ptr<RonPeerEntry> peer/*, flags = 0*/)
{
  AddPeer (peer);
}

void
PeerDestination::AddPeer (Ptr<RonPeerEntry> peer/*, flags = 0*/)
{
  m_peers.push_back (peer);
}

uint32_t
PeerDestination::GetN () const
{
  return m_peers.size ();
}

PeerDestination::Iterator
PeerDestination::Begin ()
{
  return m_peers.begin ();
}

PeerDestination::Iterator
PeerDestination::End ()
{
  return m_peers.end ();
}

PeerDestination::ConstIterator
PeerDestination::Begin () const
{
  return m_peers.begin ();
}


PeerDestination::ConstIterator
PeerDestination::End () const
{
  return m_peers.end ();
}


bool
PeerDestination::operator==(const PeerDestination rhs) const
{
  if (GetN () != rhs.GetN ())
    return false;

  for (ConstIterator itr1 = Begin (), itr2 = rhs.Begin ();
       itr1 != End () and itr2 != rhs.End (); itr1++, itr2++)
    {
      //compare the objects inside the pointers inside the iterators
      if (((*(*itr1)) != (*(*itr2))))
        return false;
    }
  return true;
}


bool
PeerDestination::operator!=(const PeerDestination rhs) const
{
  return !(*this == rhs);
}
