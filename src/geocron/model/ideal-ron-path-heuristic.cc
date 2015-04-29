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

#include "ideal-ron-path-heuristic.h"
#include "ns3/nix-vector-routing-module.h"

using namespace ns3;


NS_OBJECT_ENSURE_REGISTERED (IdealRonPathHeuristic);

IdealRonPathHeuristic::IdealRonPathHeuristic ()
{
}

TypeId
IdealRonPathHeuristic::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::IdealRonPathHeuristic")
    .SetParent<RonPathHeuristic> ()
    .AddConstructor<IdealRonPathHeuristic> ()
    .SetGroupName ("RonPathHeuristics")
    .AddAttribute ("SummaryName", "Short name that summarizes parameters, aggregations, etc. to be used when creating filenames",
                   StringValue ("ideal"),
                   MakeStringAccessor (&IdealRonPathHeuristic::m_summaryName),
                   MakeStringChecker ())
    .AddAttribute ("ShortName", "Short name that only captures the name of the heuristic",
                   StringValue ("ideal"),
                   MakeStringAccessor (&IdealRonPathHeuristic::m_shortName),
                   MakeStringChecker ())
    ;


  return tid;
}


double
IdealRonPathHeuristic::GetLikelihood (Ptr<RonPath> path)
{
  // Use NixVectorRouting to gather underlying physical topology info
  Ptr<Node> node = m_source->node;
  Ptr<Ipv4NixVectorRouting> nvr;

  // Iterate over each leg of the RonPath, starting with the source node,
  // to gather underlying physical path and check if it's unfailed
  for (RonPath::Iterator pathItr = path->Begin ();
      pathItr != path->End (); pathItr++)
  {
    nvr = node->GetObject<Ipv4NixVectorRouting> ();
    NodeContainer nodes;
    NetDeviceContainer links;
    Ipv4Address addr = (*(*pathItr)->Begin ())->address;
    nvr->GetPathFromIpv4Address (addr, nodes, links);

    // Verify that all links are up.
    for (NetDeviceContainer::Iterator linkItr = links.Begin ();
        linkItr != links.End (); linkItr++)
    {
      if (!(*linkItr)->IsLinkUp ())
        return 0;
    }

    // Verify that all nodes are up
    // NOTE: this shouldn't actually be necessary as a failed node will
    // have all of its interfaces failed too
    nodes.Add (node);
    for (NodeContainer::Iterator nodeItr = nodes.Begin ();
        nodeItr != nodes.End (); nodeItr++)
    {
      Ptr<Ipv4> ipv4 = (*nodeItr)->GetObject<Ipv4> ();
      for (uint32_t nDevs = 0; nDevs < ipv4->GetNInterfaces (); nDevs++)
      {
        if (!ipv4->IsUp (nDevs))
          return 0;
      }
    }

    // advance to next node in order to update source for NVR
    node = (*(*pathItr)->Begin ())->node;
  }

  // The path appears to be good!
  return 1.0;
}

