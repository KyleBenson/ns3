
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

#include "geodivrp-ron-path-heuristic.h"
#include "region-helper.h" //for GetLocation

#include <limits>

using namespace ns3;

NS_OBJECT_ENSURE_REGISTERED (GeodivrpRonPathHeuristic);
NS_LOG_COMPONENT_DEFINE ("GeodivrpRonPathHeuristic");

TypeId
GeodivrpRonPathHeuristic::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::GeodivrpRonPathHeuristic")
    .SetParent<RonPathHeuristic> ()
    .SetGroupName ("RonPathHeuristics")
    //.AddAttribute ("SummaryName", "Short name that summarizes parameters, aggregations, etc. to be used when creating filenames",
                   //StringValue ("geodivrp"),
                   //MakeStringAccessor (&GeodivrpRonPathHeuristic::m_summaryName),
                   //MakeStringChecker ())
    //.AddAttribute ("ShortName", "Short name that only captures the name of the heuristic",
                   //StringValue ("geodivrp"),
                   //MakeStringAccessor (&GeodivrpRonPathHeuristic::m_shortName),
                   //MakeStringChecker ())
    //.AddAttribute ("Lambda", "Constant scaling factor. >1 means lower marginal utilities for additional paths; "
        //"<1 means higher marginal utility for additional paths.",
                   //DoubleValue (1.0),
                   //MakeDoubleAccessor (&GeodivrpRonPathHeuristic::m_lambda),
                   //MakeDoubleChecker<double> ())
    ;
  return tid;
}

double
GeodivrpRonPathHeuristic::GetTieBreaker (Ptr<RonPath> leftPath, Ptr<RonPath> rightPath)
{
  Ptr<PhysicalPath> leftPhysPath = Create<PhysicalPath> (GetSourcePeer (), leftPath),
    rightPhysPath = Create<PhysicalPath> (GetSourcePeer (), rightPath);
  uint32_t leftLen = leftPhysPath->GetNNodes (), rightLen = rightPhysPath->GetNNodes ();
  if (leftLen == rightLen)
    return m_random->GetValue (-0.5, 0.5);
  return leftLen - rightLen;
}


double
GeodivrpRonPathHeuristic::GetLikelihood (Ptr<RonPath> path)
{
  if (GetCurrentMultipath ().size () < 1)
  {
    NS_LOG_LOGIC ("Empty current multipath: setting all LHs to 1");
    return 1.0;
  }

  Ptr<PhysicalPath> thisPhysPath = Create<PhysicalPath> (GetSourcePeer (), path);
  double diversity = std::numeric_limits<double>::max ();

  // Get the minimum diversity value for comparing this path with all the ones
  // currently selected for the multipath.
  for (RonPathContainer::const_iterator pathItr = GetCurrentMultipath ().begin ();
      pathItr != GetCurrentMultipath ().end (); pathItr++)
  {
    Ptr<PhysicalPath> otherPhysPath = Create<PhysicalPath> (GetSourcePeer (), *pathItr);
    double thisDiversity = GetDiversity (thisPhysPath, otherPhysPath);
    if (thisDiversity < diversity)
      diversity = thisDiversity;
  }

  NS_LOG_DEBUG (" Path " << *thisPhysPath << " has diversity " << diversity);
  NS_ASSERT_MSG (diversity < std::numeric_limits<double>::max (), "diversity value unchanged: empty GetCurrentMultipath??");
  return diversity;
}
