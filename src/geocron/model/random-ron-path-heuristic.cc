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

#include "random-ron-path-heuristic.h"

using namespace ns3;


NS_OBJECT_ENSURE_REGISTERED (RandomRonPathHeuristic);

RandomRonPathHeuristic::RandomRonPathHeuristic ()
{
}

TypeId
RandomRonPathHeuristic::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RandomRonPathHeuristic")
    .SetParent<RonPathHeuristic> ()
    .AddConstructor<RandomRonPathHeuristic> ()
    .SetGroupName ("RonPathHeuristics")
    .AddAttribute ("SummaryName", "Short name that summarizes parameters, aggregations, etc. to be used when creating filenames",
                   StringValue ("rand"),
                   MakeStringAccessor (&RandomRonPathHeuristic::m_summaryName),
                   MakeStringChecker ())
    .AddAttribute ("ShortName", "Short name that only captures the name of the heuristic",
                   StringValue ("rand"),
                   MakeStringAccessor (&RandomRonPathHeuristic::m_shortName),
                   MakeStringChecker ())
    ;

  //NS_ASSERT (tid.SetAttributeInitialValue ("ShortName", Create<StringValue> ("rand")));
  //tid.SetAttributeInitialValue ("SummaryName", Create<StringValue> ("rand"));

  return tid;
}


//bool
//RandomRonPathHeuristic::IsLikelihoodUpdateNeeded (const Ptr<PeerDestination> destination)
//{
  //return false; //cuz why would it be?
//}


double
RandomRonPathHeuristic::GetLikelihood (Ptr<RonPath> path)
{
  return m_random->GetValue ();
}
