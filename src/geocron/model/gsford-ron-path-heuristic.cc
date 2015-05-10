
/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright 2015 Kyle Benson
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

#include "gsford-ron-path-heuristic.h"
#include "region-helper.h" //for GetLocation

using namespace ns3;

NS_OBJECT_ENSURE_REGISTERED (GsfordRonPathHeuristic);
NS_LOG_COMPONENT_DEFINE ("GsfordRonPathHeuristic");

GsfordRonPathHeuristic::GsfordRonPathHeuristic ()
{
}

TypeId
GsfordRonPathHeuristic::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::GsfordRonPathHeuristic")
    .AddConstructor<GsfordRonPathHeuristic> ()
    .SetParent<RonPathHeuristic> ()
    .SetGroupName ("RonPathHeuristics")
    .AddAttribute ("SummaryName", "Short name that summarizes parameters, aggregations, etc. to be used when creating filenames",
                   StringValue ("gsford"),
                   MakeStringAccessor (&GsfordRonPathHeuristic::m_summaryName),
                   MakeStringChecker ())
    .AddAttribute ("ShortName", "Short name that only captures the name of the heuristic",
                   StringValue ("gsford"),
                   MakeStringAccessor (&GsfordRonPathHeuristic::m_shortName),
                   MakeStringChecker ())
    .AddAttribute ("D", "Threshold at which two nodes are considered too close to be geo-diverse in path",
                   DoubleValue (50.0),
                   MakeDoubleAccessor (&GsfordRonPathHeuristic::m_distanceThreshold),
                   MakeDoubleChecker<double> ())
    ;
  return tid;
}

    //Time lowestLatency = Time (9999999); // this should be big enough unless you're doing some weird interplanetary network simulation or have a straight up bug
    //Ptr<Node> closestPeer = NULL;

    ////TODO: THIS SHOULD GO IN THE GETMULTIPATH!  or we can set the LH by getting a double representig the delay and doing 1/that so that low latency values give high LHs
    //for (RonPath::Iterator itr = path->Begin ();
        //itr != path->End (); itr++)
    //{
      //Ptr<PhysicalPath> physPath = Create<PhysicalPath> (path);
      //Time thisLatency = physPath->GetLatency ();
      //if (thisLatency < lowestLatency)
      //{
        //lowestLatency = thisLatency;
        //closestPeer = path->Begin ()->Begin ();
      //}
    //}
    //NS_ASSERT_MSG (closestPeer, "Didn't find a closestPeer, must not be any options!");

double
GsfordRonPathHeuristic::GetLikelihood (Ptr<RonPath> path)
{
  // We start by choosing the peer with the lowest latency if no
  // other paths have been chosen yet (other than the one
  // representing the direct path to the destination)
  if (GetCurrentMultipath ().size () <= 1)
  {
      Ptr<PhysicalPath> physPath = Create<PhysicalPath> (GetSourcePeer (), *path->Begin ());
      Time thisLatency = physPath->GetLatency ();
      // Need to do 1/latency so that low latency paths get higher likelihood
      uint64_t millis = thisLatency.GetMilliSeconds ();
      NS_ASSERT_MSG (millis != 0, "Link with 0ms latency, this will cause a divide by zero error!");
      //NS_LOG_UNCOND ("Latency LH for peer " << *(*(*path->Begin ())->Begin ()) << " is " << 1.0/millis);
      return 1.0/millis;
  }

  // Otherwise, iterate over each of the currently chosen paths,
  // penalizing this path for each pair of nodes that are within the
  // threshold distance of each other.
  else
  {
    // for each path we compare to, we know that the source and
    // destination will be the exact same as that of the path under consideration
    int penalty = -2 * GetCurrentMultipath ().size ();
    Ptr<PhysicalPath> thisPhysPath = Create<PhysicalPath> (GetSourcePeer (), path);

    //TODO: skip comparing first and last component since they'll all be the same!
    //or maybe we want to just manually remove that value from the penalty since
    //it is possible that a node on the path's interior is close to the source/dest?
    for (RonPathContainer::const_iterator pathItr = GetCurrentMultipath ().begin ();
        pathItr != GetCurrentMultipath ().end (); pathItr++)
    {
      // Do NxN cross-wise comparison between all the nodes on the two paths,
      // building up the total penalty for the current path under consideration
      Ptr<PhysicalPath> otherPhysPath = Create<PhysicalPath> (GetSourcePeer (), *pathItr);

      for (NodeContainer::Iterator thisNodeItr = thisPhysPath->NodesBegin ();
          thisNodeItr != thisPhysPath->NodesEnd (); thisNodeItr++)
      {
        for (NodeContainer::Iterator otherNodeItr = otherPhysPath->NodesBegin ();
            otherNodeItr != otherPhysPath->NodesEnd (); otherNodeItr++)
        {
          Vector3D va = GetLocation (*thisNodeItr);
          Vector3D vb = GetLocation (*otherNodeItr);
          double distance = CalculateDistance (va, vb);

          if (distance <= m_distanceThreshold)
            penalty++;
        }
      }
      NS_LOG_DEBUG ("Penalty after checking path " << *otherPhysPath << " is " << penalty);
    }
    NS_LOG_DEBUG ("Penalty for path " << *thisPhysPath << " (peer " << *(*(*path->Begin ())->Begin ()) << ") is " << penalty);
    // We need to ensure penalty is at least 1 to avoid division by 0 if this path is fully disjoint
    NS_ASSERT_MSG (penalty >= 0, "penalty should be at least 0 since we accounted for shared physical path with both source and destination");
    // We use the latency as a tie-breaker, so we need to normalize the
    // latency value (inversely so >latency == < likelihood) to fit in
    // between this penalty value and the next highest possible one,
    // hence breaking the tie.
    double lh = 1.0/(penalty + 2), nextLh = 1.0/(penalty + 1); // careful for divide by 0!
    double deltaLh = nextLh - lh;

    //NS_ASSERT_MSG (penalty < 10, "Penalty unexpectedly large for path #" << GetCurrentMultipath ().size ());
    NS_ASSERT_MSG (deltaLh > 0, "Delta LH value <= 0!");

    uint64_t latency = thisPhysPath->GetLatency ().GetMilliSeconds ();
    NS_ASSERT_MSG (latency > 0, "Latency value <= 0!  Please don't break the laws of physics in this simulation");

    // We add one to the latency to ensure lhLatencyComponent fits in range [0, deltaLh)
    double lhLatencyComponent = deltaLh / (latency + 1);
    double finalLh = lh + lhLatencyComponent;
    NS_ASSERT_MSG (lhLatencyComponent >= 0 and lhLatencyComponent < deltaLh, "Likelihood latency component not in expected range!");
    NS_LOG_DEBUG ("LH for peer " << *(*(*path->Begin ())->Begin ()) << " is " << finalLh);
    return finalLh;
  }
}
