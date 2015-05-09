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

#include "physical-path.h"
#include "ns3/log.h"
#include "ns3/nix-vector-routing-module.h" // for SetPathFromRonPath()

namespace ns3
{
  NS_LOG_COMPONENT_DEFINE ("PhysicalPath");

PhysicalPath::PhysicalPath()
{}

PhysicalPath::PhysicalPath(Ptr<RonPeerEntry> source, Ptr<RonPath> other)
{
  SetPathFromRonPath (source, other);
}


void
PhysicalPath::AddHop (Ptr<Node> node, Link link)
{
  m_nodes.Add (node);
  m_links.Add (link);
}

bool
PhysicalPath::SetPathFromRonPath (Ptr<RonPeerEntry> source, Ptr<RonPath> ronPath)
{
  // Remember that we don't include sources in RonPaths but we do in PhysicalPaths
  Ptr<Node> node = source->node;

  // Clear the previous nodes and links.
  m_nodes = NodeContainer ();
  m_links = LinkContainer ();

  // Use NixVectorRouting to gather underlying physical topology info
  Ptr<Ipv4NixVectorRouting> nvr;

  // Iterate over each leg of the RonPath, starting with the source node,
  // to gather underlying physical path
  for (RonPath::Iterator pathItr = ronPath->Begin ();
      pathItr != ronPath->End (); pathItr++)
  {
    nvr = node->GetObject<Ipv4NixVectorRouting> ();
    NodeContainer nodes;
    Ipv4InterfaceContainer links;
    Ipv4Address addr = (*(*pathItr)->Begin ())->address;

    // Get the path if it exists, return false otherwise
    if (!nvr->GetPathFromIpv4Address (addr, nodes, links))
      return false;

    for (LinkContainer::Iterator linkItr = links.Begin ();
        linkItr != links.End (); linkItr++)
    {
      m_links.Add (*linkItr);
    }

    // need to add the start node of each PeerDestination as the physical
    // path returned by the NixVectorRouting doesn't include the source
    m_nodes.Add (node);
    for (NodeContainer::Iterator nodeItr = nodes.Begin ();
        nodeItr != nodes.End (); nodeItr++)
    {
      m_nodes.Add (*nodeItr);
    }

    // advance to next node in order to update source for NVR
    node = (*(*pathItr)->Begin ())->node;
  }
  m_nodes.Add (node); // don't forget the final destination!

  NS_ASSERT_MSG (m_nodes.GetN () == m_links.GetN () + 1,
      "Number of links and nodes are mismatched after creating a PhysicalPath from a RonPath! Got "
      << m_nodes.GetN () << " nodes and " << m_links.GetN () << " links");
  return true;
}


Ptr<Channel>
PhysicalPath::GetChannelForLink (const Link& link)
{
  return link.first->GetNetDevice (link.second)->GetChannel ();
}


Time
PhysicalPath::GetLatency ()
{
  Time latency (0);
  for (LinkContainer::Iterator itr = LinksBegin ();
      itr != LinksEnd (); itr++)
  {
      Ptr<Channel> channel = GetChannelForLink (*itr);
      TimeValue latencyValue;
      channel->GetAttribute ("Delay", latencyValue);
      latency += latencyValue.Get ();
  }
  return latency;
}


uint32_t
PhysicalPath::GetIntersectionSize (Ptr<PhysicalPath> other)
{
  uint32_t size = 0;
  // Need to do m*n comparison here
  // NOTE: we don't expect any loops in the path, which would
  // break this logic!
  for (NodeContainer::Iterator itr1 = NodesBegin ();
       itr1 != NodesEnd (); itr1++)
    {
      for (NodeContainer::Iterator itr2 = other->NodesBegin ();
          itr2 != other->NodesEnd (); itr2++)
        if (*(itr1) == *(itr2))
          size++;
    }
  for (LinkContainer::Iterator itr1 = LinksBegin ();
       itr1 != LinksEnd (); itr1++)
    {
      for (LinkContainer::Iterator itr2 = other->LinksBegin ();
          itr2 != other->LinksEnd (); itr2++)
        if (*(itr1) == *(itr2))
          size++;
    }
  return size;
}

//void
//PhysicalPath::Reverse ()
//{
//}


NodeContainer::Iterator
PhysicalPath::NodesBegin () const
{
  return m_nodes.Begin ();
}


NodeContainer::Iterator
PhysicalPath::NodesEnd () const
{
  return m_nodes.End ();
}


LinkContainer::Iterator
PhysicalPath::LinksBegin () const
{
  return m_links.Begin ();
}


LinkContainer::Iterator
PhysicalPath::LinksEnd () const
{
  return m_links.End ();
}


uint32_t
PhysicalPath::GetNNodes () const
{
  return m_nodes.GetN ();
}


uint32_t
PhysicalPath::GetNLinks () const
{
  return m_links.GetN ();
}


std::ostream&
operator<< (std::ostream& os, PhysicalPath const& path)
{
  bool first = true;
  for (NodeContainer::Iterator itr = path.NodesBegin ();
      itr != path.NodesEnd (); itr++)
  {
    if (first)
      first = false;
    else
      os << "-->";
    os << ((*itr)->GetId ());
  }
  return os;
}


bool
PhysicalPath::operator== (const PhysicalPath rhs) const
{
  if ((m_nodes.GetN () != rhs.GetNNodes ()) or (m_links.GetN () != rhs.GetNLinks ()))
    return false;
  for (NodeContainer::Iterator itr1 = NodesBegin (), itr2 = rhs.NodesBegin ();
       itr1 != NodesEnd () and itr2 != rhs.NodesEnd (); itr1++,itr2++)
    {
      if (*(itr1) != *(itr2))
        return false;
    }
  for (LinkContainer::Iterator itr1 = LinksBegin (), itr2 = rhs.LinksBegin ();
       itr1 != LinksEnd () and itr2 != rhs.LinksEnd (); itr1++,itr2++)
    {
      if (*(itr1) != *(itr2))
        return false;
    }
  return true;
}


bool
PhysicalPath::operator!=(const PhysicalPath rhs) const
{
  return !(*this == rhs);
}


//bool
//PhysicalPath::operator< (const PhysicalPath rhs) const
//{
  //PhysicalPath::ConstIterator itr1 = Begin (), itr2 = rhs.Begin ();
  //for (; itr1 != End () and itr2 != rhs.End (); itr1++,itr2++)
    //{
      //if ((*(*itr1)->Begin ())->id > (*(*itr2)->Begin ())->id)
        //return false;
    //}

  ////check sizes
  //if (GetN () > rhs.GetN ())
    //return false;
  //return true;
//}


// end of namespace ns3
}
