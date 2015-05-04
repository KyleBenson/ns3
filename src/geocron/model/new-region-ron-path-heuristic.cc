
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

#include "new-region-ron-path-heuristic.h"

using namespace ns3;

NS_OBJECT_ENSURE_REGISTERED (NewRegionRonPathHeuristic);

NewRegionRonPathHeuristic::NewRegionRonPathHeuristic ()
{
}

TypeId
NewRegionRonPathHeuristic::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::NewRegionRonPathHeuristic")
    .AddConstructor<NewRegionRonPathHeuristic> ()
    .SetParent<RonPathHeuristic> ()
    .SetGroupName ("RonPathHeuristics")
    .AddAttribute ("SummaryName", "Short name that summarizes parameters, aggregations, etc. to be used when creating filenames",
                   StringValue ("newreg"),
                   MakeStringAccessor (&NewRegionRonPathHeuristic::m_summaryName),
                   MakeStringChecker ())
    .AddAttribute ("ShortName", "Short name that only captures the name of the heuristic",
                   StringValue ("newreg"),
                   MakeStringAccessor (&NewRegionRonPathHeuristic::m_shortName),
                   MakeStringChecker ())
    ;
  return tid;
}


double
NewRegionRonPathHeuristic::GetLikelihood (Ptr<RonPath> path)
{
  if (m_regionsAttempted.count ((*(*path->Begin ())->Begin ())->region))
    return 0.0;
  return 1.0;
}


void
NewRegionRonPathHeuristic::DoNotifyTimeout (Ptr<RonPath> path, Time time)
{
  //need to update all paths to reflect the newly black-listed region
  //TODO: cache them and do this more efficiently?  perhaps override DoUpdateLikelihoods
  if (!m_regionsAttempted.count ((*(*path->Begin ())->Begin ())->region))
  {
    m_regionsAttempted.insert ((*(*path->Begin ())->Begin ())->region);
    ForceUpdateLikelihoods (path->GetDestination ());
  }
  SetLikelihood (path, 0.0);
}
