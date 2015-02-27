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

#include "closest-first-ron-path-heuristic.h"
#include <cmath>

using namespace ns3;


NS_OBJECT_ENSURE_REGISTERED (ClosestFirstRonPathHeuristic);

ClosestFirstRonPathHeuristic::ClosestFirstRonPathHeuristic ()
{
}

TypeId
ClosestFirstRonPathHeuristic::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ClosestFirstRonPathHeuristic")
    .AddConstructor<ClosestFirstRonPathHeuristic> ()
    .SetParent<RonPathHeuristic> ()
    .SetGroupName ("RonPathHeuristics")
    .AddAttribute ("SummaryName", "Short name that summarizes parameters, aggregations, etc. to be used when creating filenames",
                   StringValue ("close"),
                   MakeStringAccessor (&ClosestFirstRonPathHeuristic::m_summaryName),
                   MakeStringChecker ())
    .AddAttribute ("ShortName", "Short name that only captures the name of the heuristic",
                   StringValue ("close"),
                   MakeStringAccessor (&ClosestFirstRonPathHeuristic::m_shortName),
                   MakeStringChecker ())
    .AddAttribute ("MaxDistance", "Maximum distance we will permit for peers.",
                   DoubleValue (45.0),
                   MakeDoubleAccessor (&ClosestFirstRonPathHeuristic::m_maxDistance),
                   MakeDoubleChecker<double> ())
    ;
  //tid.SetAttributeInitialValue ("ShortName", Create<StringValue> ("ortho"));
  //tid.SetAttributeInitialValue ("SummaryName", Create<StringValue> ("ortho"));

  return tid;
}


double
ClosestFirstRonPathHeuristic::GetLikelihood (Ptr<RonPath> path)
{
  NS_ASSERT_MSG (m_source, "You must set the source peer before using the heuristic!");
  
  Vector3D va = m_source->location;
  Vector3D vb = (*(*path->Begin ())->Begin ())->location;
  double distance = CalculateDistance (va, vb);

  if (distance > m_maxDistance)
    return 0;

  //TODO: transform to a LH
  return m_maxDistance - distance;
}
