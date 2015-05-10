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
#include "physical-path.h"
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
  Ptr<PhysicalPath> realPath = Create<PhysicalPath> ();

  // Get the path if it exists, return false otherwise
  if (!realPath->SetPathFromRonPath (m_source, path))
    return 0;

  // Verify that all links are up.
  for (LinkContainer::Iterator linkItr = realPath->LinksBegin ();
      linkItr != realPath->LinksEnd (); linkItr++)
  {
    if (!((linkItr->first)->IsUp (linkItr->second)))
      return 0;
  }

  // Verify that all nodes are up
  // NOTE: this is necessary as even though a failed node will
  // have all of its interfaces failed too, the failure mechanism
  // only handles one side of the link, which can lead to misleading
  // situations if only one end is checked for being up
  // FOLLOWUP NOTE: this is no longer necessary as we fixed the
  // failure model to fail both ends of a link when a LINK is failed.
  // This could still result in an issue if a NODE is failed on the
  // other end, but then we would have discovered that all the links
  // exiting that node would be down too!  So unless we have the possibility
  // of a final destination (server node? overlay?) failing, this is not an issue.
  // If it is, we should really only have to check the final node on the path anyway.
  //for (NodeContainer::Iterator nodeItr = nodes.Begin ();
  //nodeItr != nodes.End (); nodeItr++)
  //{
  //Ptr<Ipv4> ipv4 = (*nodeItr)->GetObject<Ipv4> ();
  //bool oneLinkUp = false; // if we find one link that's up, this node isn't failed
  //for (uint32_t nDevs = 0; nDevs < ipv4->GetNInterfaces (); nDevs++)
  //{
  //if (ipv4->IsUp (nDevs))
  //oneLinkUp = true;
  //}
  //if (!oneLinkUp)
  //return 0;
  //}

  // The path appears to be good!
  return 1.0;
}


//bool
//IdealRonPathHeuristic::IsLikelihoodUpdateNeeded (const Ptr<PeerDestination> destination)
//{
  //return false; // this should never need to happen until we get to dynamic failures
//}

