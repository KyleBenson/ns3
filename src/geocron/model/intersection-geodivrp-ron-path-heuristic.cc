
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

#include "intersection-geodivrp-ron-path-heuristic.h"
#include <algorithm>

using namespace ns3;

NS_OBJECT_ENSURE_REGISTERED (IntersectionGeodivrpRonPathHeuristic);
NS_LOG_COMPONENT_DEFINE ("IntersectionGeodivrpRonPathHeuristic");

IntersectionGeodivrpRonPathHeuristic::IntersectionGeodivrpRonPathHeuristic ()
{
}

TypeId
IntersectionGeodivrpRonPathHeuristic::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::IntersectionGeodivrpRonPathHeuristic")
    .AddConstructor<IntersectionGeodivrpRonPathHeuristic> ()
    .SetParent<GeodivrpRonPathHeuristic> ()
    .SetGroupName ("RonPathHeuristics")
    .AddAttribute ("SummaryName", "Short name that summarizes parameters, aggregations, etc. to be used when creating filenames",
                   StringValue ("intergeodivrp"),
                   MakeStringAccessor (&IntersectionGeodivrpRonPathHeuristic::m_summaryName),
                   MakeStringChecker ())
    .AddAttribute ("ShortName", "Short name that only captures the name of the heuristic",
                   StringValue ("intergeodivrp"),
                   MakeStringAccessor (&IntersectionGeodivrpRonPathHeuristic::m_shortName),
                   MakeStringChecker ())
    ;
  return tid;
}

double
IntersectionGeodivrpRonPathHeuristic::GetDiversity (Ptr<PhysicalPath> path1, Ptr<PhysicalPath> path2)
{
  // We substract 2 because we're ignoring the source and destination,
  // since we can assume they'll always be the same.
#ifdef NS3_LOG_ENABLE
  NodeContainer::Iterator itr1 = path1->NodesBegin (), itr2 = path2->NodesBegin ();
  NS_ASSERT_MSG (((*itr1)->GetId () == (*itr2)->GetId ()), "Source nodes unexpectedly not equal!");
  for (uint32_t i = 1; i < path1->GetNNodes (); itr1++, i++);
  for (uint32_t i = 1; i < path2->GetNNodes (); itr2++, i++);
  NS_ASSERT_MSG (((*itr1)->GetId () == (*itr2)->GetId ()), "Destination nodes unexpectedly not equal!");
#endif

  double overlap = ((double)path1->GetIntersectionSize (path2)) - 2;
  // Need to take the minimum length of the two paths as otherwise the order
  // becomes significant
  double scale = (std::min (path1->GetNNodes () + path1->GetNLinks (),
        path2->GetNNodes () + path2->GetNLinks ()));

  NS_LOG_DEBUG ("Paths " << *path1 << " and " << *path2
      << " have overlap of " << overlap << " and scale of " << scale);
  return 1 - overlap / scale;
}
