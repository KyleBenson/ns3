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

#include "peer-area-ron-path-heuristic.h"
#include "physical-path.h" //for the tie-breaker that uses path length
#include "region-helper.h" //for GetLocation
#include <limits>

using namespace ns3;

NS_OBJECT_ENSURE_REGISTERED (PeerAreaRonPathHeuristic);
NS_LOG_COMPONENT_DEFINE ("PeerAreaRonPathHeuristic");

PeerAreaRonPathHeuristic::PeerAreaRonPathHeuristic ()
{
}

TypeId
PeerAreaRonPathHeuristic::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PeerAreaRonPathHeuristic")
    .AddConstructor<PeerAreaRonPathHeuristic> ()
    .SetParent<RonPathHeuristic> ()
    .SetGroupName ("RonPathHeuristics")
    .AddAttribute ("SummaryName", "Short name that summarizes parameters, aggregations, etc. to be used when creating filenames",
                   StringValue ("peerarea"),
                   MakeStringAccessor (&PeerAreaRonPathHeuristic::m_summaryName),
                   MakeStringChecker ())
    .AddAttribute ("ShortName", "Short name that only captures the name of the heuristic",
                   StringValue ("peerarea"),
                   MakeStringAccessor (&PeerAreaRonPathHeuristic::m_shortName),
                   MakeStringChecker ())
    //TODO: know the PhysicalPath representing the normal non-overlay path taken to the server
    //and use it for calculating area.  Not sure how to do that only for that one path:
    //perhaps assume that when comparing with a path of length 1 we must be dealing with the
    //normal non-overlay path
    //
    //TODO: not use GeoDivRP's method where the multipath diversity is defined as the minimum 
    //of all the diversities, but rather perhaps use some other accumulation function: sum squares?
    ;
  return tid;
}

double
PeerAreaRonPathHeuristic::GetArea (Ptr<RonPath> path1, Ptr<RonPath> path2)
{
  if (path1 == path2)
  {
    NS_LOG_LOGIC ("Got same path in GetArea! Returning 0");
    return 0;
  }

  // First, we need to combine the paths and store them in a vector
  // NOTE: this is trivial since we currently assume RonPaths are all
  // of length 2.  In the future we may want to borrow from the GeoDivRP
  // GetArea function...
  std::vector<Vector> points;
  points.push_back (GetSourcePeer ()->location);
  points.push_back (path1->GetFirstPeer ()->location);
  points.push_back (path1->GetDestinationPeer ()->location);
  points.push_back (path2->GetFirstPeer ()->location);

  // Now we compute the area using a simple algorithm that doesn't
  // handle the paths crossing each other...
  // Code based on that taken from http://alienryderflex.com/polygon_area/
  double area = 0;
  for (uint32_t i = 0, j = points.size () - 1; i < points.size (); i++)
  {
    area += (points[j].x + points[i].x) * (points[j].y - points[i].y);
    j = i;
  }
  area /= 2.0;

  // if the points are arranged counter-clockwise, the area will be
  // negative, so let's fix that
  area = std::abs (area);
  NS_LOG_DEBUG ("RonPath " << *path1 << " and " << *path2 << " area: " << area);
  return area;
}


double
PeerAreaRonPathHeuristic::GetTieBreaker (Ptr<RonPath> leftPath, Ptr<RonPath> rightPath)
{
  Ptr<PhysicalPath> leftPhysPath = Create<PhysicalPath> (GetSourcePeer (), leftPath),
    rightPhysPath = Create<PhysicalPath> (GetSourcePeer (), rightPath);
  uint32_t leftLen = leftPhysPath->GetNNodes (), rightLen = rightPhysPath->GetNNodes ();
  if (leftLen == rightLen)
    return m_random->GetValue (-0.5, 0.5);
  return leftLen - rightLen;
}


double
PeerAreaRonPathHeuristic::GetLikelihood (Ptr<RonPath> path)
{
  if (GetCurrentMultipath ().size () < 1)
  {
    NS_LOG_LOGIC ("Empty current multipath: setting all LHs to 1");
    return 1.0;
  }

  double diversity = std::numeric_limits<double>::max ();

  // Get the minimum diversity value for comparing this path with all the ones
  // currently selected for the multipath.
  for (RonPathContainer::const_iterator pathItr = GetCurrentMultipath ().begin ();
      pathItr != GetCurrentMultipath ().end (); pathItr++)
  {
    double thisDiversity = GetArea (path, *pathItr);
    if (thisDiversity < diversity)
      diversity = thisDiversity;
  }

  NS_ASSERT_MSG (diversity < std::numeric_limits<double>::max (), "diversity value unchanged: empty GetCurrentMultipath??");
  return diversity;
}
