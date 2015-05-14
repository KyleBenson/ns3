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

#include "ron-path-heuristic.h"
//#include "boost/bind.hpp"
#include <cmath>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("RonPathHeuristic");
NS_OBJECT_ENSURE_REGISTERED (RonPathHeuristic);

RonPathHeuristic::RonPathHeuristic ()
{
  m_masterLikelihoods = NULL;
  m_topLevel = NULL;
  m_peers = NULL;
  m_source = NULL;
}

TypeId
RonPathHeuristic::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RonPathHeuristic")
    .SetParent<Object> ()
    .AddConstructor<RonPathHeuristic> ()
    .AddAttribute ("Weight", "Value by which the likelihood this heuristic assigns is weighted.",
                   DoubleValue (1.0),
                   MakeDoubleAccessor (&RonPathHeuristic::m_weight),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("Random",
            "The random variable for random decisions.",
            StringValue ("ns3::UniformRandomVariable"),
            MakePointerAccessor (&RonPathHeuristic::m_random),
            MakePointerChecker<RandomVariableStream> ())
    /*.AddAttribute ("SummaryName", "Short name that summarizes parameters, aggregations, etc. to be used when creating filenames",
                   StringValue ("base"),
                   MakeStringAccessor (&RonPathHeuristic::m_summaryName),
                   MakeStringChecker ())
    .AddAttribute ("ShortName", "Short name that only captures the name of the heuristic",
                   StringValue ("base"),
                   MakeStringAccessor (&RonPathHeuristic::m_shortName),
                   MakeStringChecker ())*/
  ;
  return tid;
}

RonPathHeuristic::~RonPathHeuristic ()
{
  if (m_topLevel == this)
    delete m_masterLikelihoods;
} //required to keep gcc from complaining


void
RonPathHeuristic::MakeTopLevel ()
{
  NS_ASSERT_MSG (m_topLevel == NULL or m_topLevel == this,
                 "You can't make an already-aggregated heuristic into a top-level one!");
  m_topLevel = this;
  if (m_masterLikelihoods == NULL)
    m_masterLikelihoods = new MasterPathLikelihoodTable ();
}


void
RonPathHeuristic::AddPath (Ptr<RonPath> path)
{
  Ptr<PeerDestination> dest = path->GetDestination ();
  // If we are not top-level and the top-level has not been notified of this path,
  // we need to start at the top and work our way down, notifying along the way.
  if ((m_topLevel != (RonPathHeuristic*)this) and
      (m_masterLikelihoods->at (dest)[path] == NULL))
    m_topLevel->AddPath (path);

  // We need to add a new likelihood value to the Likelihood object in the master table,
  // then add the reference to this value to our table.
  else
    {
      // Check if a LH for this path exists already
      Ptr<Likelihood> lh = (*m_masterLikelihoods)[dest][path], newLh;
      // Add the path if it doesn't
      if (lh == NULL)
        {
          //this should only happen if we are top-level
          NS_ASSERT_MSG (m_topLevel == (RonPathHeuristic*)this,
                         "NULL Likelihood object (initial Create<> needed), but not at top-level!");
          newLh = Create<Likelihood> ();
          (*m_masterLikelihoods)[dest][path] = newLh;
        }
      else
        {
          newLh = lh->AddLh (0.0);
        }
      m_likelihoods[dest][path] = newLh;

      // Then recurse to the lower-level heuristics
      for (AggregateHeuristics::iterator others = m_aggregateHeuristics.begin ();
           others != m_aggregateHeuristics.end (); others++)
        (*others)->AddPath (path);
    }
}


void
RonPathHeuristic::EnsurePathRegistered (Ptr<RonPath> path)
{
  NS_ASSERT_MSG (path->GetN () > 0, "got 0-length path in EnsurePathRegistered");

  Ptr<PeerDestination> dest = path->GetDestination ();
  if (m_likelihoods[dest].count (path) == 0)
    AddPath (path);
}


bool
RonPathHeuristic::ArePathsBuilt (Ptr<PeerDestination> destination)
{
  return m_pathsBuilt.count (destination);
}


void
RonPathHeuristic::BuildPaths (Ptr<PeerDestination> destination)
{
  DoBuildPaths (destination);
  for (AggregateHeuristics::iterator others = m_aggregateHeuristics.begin ();
       others != m_aggregateHeuristics.end (); others++)
    (*others)->BuildPaths (destination);
}


void
RonPathHeuristic::DoBuildPaths (Ptr<PeerDestination> destination)
{
  if (m_topLevel->ArePathsBuilt (destination) or ArePathsBuilt (destination))
    return;

  Ptr<RonPeerEntry> sourcePeer = GetSourcePeer ();
  NS_ASSERT_MSG (m_peers, "peer table in heuristic is NULL!");
  for (RonPeerTable::Iterator peerItr = m_peers->Begin ();
       peerItr != m_peers->End (); peerItr++)
    {
      //make sure not to have loops!
      //TODO: check all pieces of destination
      if (*(*peerItr) == *(*destination->Begin ()) or *(*peerItr) == *sourcePeer)
        continue;
      Ptr<RonPath> path = Create<RonPath> ();
      path->AddHop (Create<PeerDestination> (*peerItr));
      path->AddHop (destination);
      NS_ASSERT_MSG (path->GetN () > 1, "Built path of length " << path->GetN () << ", which is no good!");
      EnsurePathRegistered (path);
    }
  NS_LOG_LOGIC ("Created " << (*m_masterLikelihoods)[destination].size ()
      << " paths to destination " << *destination);
  m_pathsBuilt.insert (destination);
  /*NS_ASSERT_MSG (m_likelihoods[destination].size () == m_peers->GetN () - 2,
    "we should have two less paths than peers now...");*/
}


double
RonPathHeuristic::GetLikelihood (Ptr<RonPath> path)
{
  return 1.0;
}


double
RonPathHeuristic::GetLikelihood (Ptr<RonPath> path, RonPathContainer & currentMultiPath)
{
  return GetLikelihood (path);
}


bool
RonPathHeuristic::IsLikelihoodUpdateNeeded (const Ptr<PeerDestination> destination)
{
  return !m_updatedOnce.count (destination);
}


void
RonPathHeuristic::NotifyLikelihoodUpdateNeeded (const Ptr<PeerDestination> destination)
{
  m_updatedOnce.erase (destination);
}


void
RonPathHeuristic::NotifyLikelihoodUpdated (const Ptr<PeerDestination> destination)
{
  m_updatedOnce.insert (destination);
}


void
RonPathHeuristic::UpdateLikelihoods (const Ptr<PeerDestination> destination)
{
  //update us first, then the other aggregate heuristics (if any)
  DoUpdateLikelihoods (destination);
  for (AggregateHeuristics::iterator others = m_aggregateHeuristics.begin ();
       others != m_aggregateHeuristics.end (); others++)
    (*others)->UpdateLikelihoods (destination);
}


void
RonPathHeuristic::DoUpdateLikelihoods (const Ptr<PeerDestination> destination)
{
  if (IsLikelihoodUpdateNeeded (destination))
    {
      for (PathLikelihoodInnerTable::iterator path = m_likelihoods.at (destination).begin ();
           path != m_likelihoods[destination].end (); path++)
        SetLikelihood (path->first, GetLikelihood (path->first));
      NotifyLikelihoodUpdated (destination);
    }
}


void
RonPathHeuristic::ForceUpdateLikelihoods (const Ptr<PeerDestination> destination)
{
  NotifyLikelihoodUpdateNeeded (destination);
  m_pathsAttempted.clear ();
  m_currentMultiPath.clear ();
  UpdateLikelihoods (destination);
}

bool
RonPathHeuristic::ShouldConsiderPath (Ptr<RonPath> path)
{
  //TODO: write tests?
  // We want to avoid getting the path going straight to the destination
  return !PathAttempted (path) and path->GetN () > 1;
}

double
RonPathHeuristic::GetTieBreaker (Ptr<RonPath> leftPath, Ptr<RonPath> rightPath)
{
  return m_random->GetValue (-0.5, 0.5);
}

Ptr<RonPath>
RonPathHeuristic::GetBestPath (Ptr<PeerDestination> destination)
{
  NS_ASSERT_MSG (m_source, "You must set the source peer before using the heuristic!");
  NS_ASSERT_MSG (destination, "You must specify a valid server to use the heuristic!");
  NS_ASSERT_MSG (m_masterLikelihoods, "Master likelihood table missing!  " \
                 "Make sure you aggregate heuristics to the top-level one first (before lower-levels) " \
                 "so that the shared data structures are properly organized.");

  // ensure paths built
  BuildPaths (destination);

  NS_ASSERT_MSG ((*m_masterLikelihoods)[destination].size () > 0, "empty master likelihood table!");

  //find the path with highest likelihood
  //TODO: cache up to MaxAttempts of them
  //TODO: check for valid size
  if ((*m_masterLikelihoods)[destination].size () == 0)
    throw NoValidPeerException();

  UpdateLikelihoods (destination);

  NS_LOG_LOGIC ("Choosing best path of " << (*m_masterLikelihoods)[destination].size () << " possibilities");
  MasterPathLikelihoodInnerTable::iterator probs = (*m_masterLikelihoods)[destination].begin ();
  Ptr<RonPath> bestPath = NULL;
  double bestLikelihood = 0;
  for (; probs != (*m_masterLikelihoods)[destination].end (); probs++)
    {
      double nextLh = probs->second->GetLh ();
      if (ShouldConsiderPath (probs->first))
      {
        if (nextLh > bestLikelihood)
        {
          bestPath = probs->first;
          bestLikelihood = nextLh;
        }
        // Here we break ties, either randomly or in a heuristic-specified
        // manner 
        else if (nextLh == bestLikelihood and bestPath != NULL and GetTieBreaker (bestPath, probs->first) <= 0.0)
        {
          bestPath = probs->first;
          bestLikelihood = nextLh;
        }
      }
    }

  if (bestLikelihood <= 0.0 or bestPath == NULL)
    throw NoValidPeerException();

  PathAttempted (bestPath, true);
  NS_ASSERT_MSG (bestPath->GetN () > 1, "Got path of length " << bestPath->GetN ()
      << " in GetBestPath: heuristics should only care about multi-peer paths!");
  return (bestPath);
}


RonPathContainer
RonPathHeuristic::GetBestMultiPath (Ptr<PeerDestination> destination, uint32_t multipathFanout,
    RonPathContainer currentMultiPath)
{
  NS_ASSERT_MSG (multipathFanout > 0, "multipathFanout <= 0, this won't return any peer choices!");

  // We want to let heuristics know what the currently selected RonPaths
  // are so that they can make decisions based on those.  Clear the
  // previous multipath and add the RonPath going straight to the
  // destination so we can try and avoid anything too similar.
  m_currentMultiPath = currentMultiPath;
  m_currentMultiPath.push_back (Create<RonPath> (destination));
  NotifyLikelihoodUpdateNeeded (destination); //don't use force or we will clear m_currentMultiPath

  RonPathContainer result;
  for (; multipathFanout > 0; multipathFanout--)
  {
    //TODO: decide how to tell heuristic that it shouldn't give us this same path again, possibly without using NotifyTimeout???
    Ptr<RonPath> nextDest = GetBestPath (destination);
    NS_ASSERT_MSG (nextDest->GetN () > 1, "Got path of length " << nextDest->GetN () << " in GetBestMultiPath, which is no good!");
    result.push_back (nextDest);
    m_currentMultiPath.push_back (nextDest);
    NotifyLikelihoodUpdateNeeded (destination); //don't use force or we may get duplicates
  }

#ifdef NS3_LOG_ENABLE
  uint32_t nitrs = 0;
  for (RonPathContainer::iterator itr = result.begin(); itr != result.end(); itr++)
  {
    NS_ASSERT_MSG ((*itr)->GetN () > 1, "Got path of length " << (*itr)->GetN () << " in GetBestMultiPath, which is no good!");
    nitrs++;
  }
  NS_ASSERT_MSG (nitrs == result.size(), "not enough path iterations!");
#endif

  return result;
  //TODO: perhaps ForceUpdateLikelihoods (destination) in order to
  //clear the attempted paths?
}


const RonPathContainer&
RonPathHeuristic::GetCurrentMultipath () const
{
  return m_currentMultiPath;
}


////////////////////////////////////////////////////////////////////////////////
////////////////////  notifications / updates  /////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


void
RonPathHeuristic::NotifyAck (Ptr<RonPath> path, Time time)
{
  DoNotifyAck (path, time);
  for (AggregateHeuristics::iterator heuristic = m_aggregateHeuristics.begin ();
       heuristic != m_aggregateHeuristics.end (); heuristic++)
    {
      (*heuristic)->NotifyAck (path, time);
    }
}


void
RonPathHeuristic::DoNotifyAck (Ptr<RonPath> path, Time time)
{
  EnsurePathRegistered (path);
  SetLikelihood (path, 1.0);
  //TODO: record partial path ACKs
  //TODO: record parents (Markov chain)
}


void
RonPathHeuristic::NotifyTimeout (Ptr<RonPath> path, Time time)
{
  DoNotifyTimeout (path, time);
  for (AggregateHeuristics::iterator heuristic = m_aggregateHeuristics.begin ();
       heuristic != m_aggregateHeuristics.end (); heuristic++)
    {
      (*heuristic)->NotifyTimeout (path, time);
    }
  
  NS_ASSERT_MSG (m_masterLikelihoods->at (path->GetDestination ())[path]->GetLh () == 0.0,
                 "m_masterLikelihoods Likelihood != 0 after timeout notification!");
  NS_ASSERT_MSG (m_likelihoods.at (path->GetDestination ())[path]->GetLh () == 0.0,
  "m_masterLikelihoods Likelihood != 0 after timeout notification!");
}


void
RonPathHeuristic::DoNotifyTimeout (Ptr<RonPath> path, Time time)
{
  //TODO: handle partial path ACKs
  EnsurePathRegistered (path);
  SetLikelihood (path, 0.0);
}


////////////////////////////////////////////////////////////////////////////////
////////////////////  mutators / accessors  ////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


void
RonPathHeuristic::AddHeuristic (Ptr<RonPathHeuristic> other)
{
  NS_ASSERT_MSG (this != other, "What are you doing? You can't aggregate a heuristic to itself...");

  other->m_topLevel = m_topLevel;
  other->m_masterLikelihoods = m_topLevel->m_masterLikelihoods;
  m_aggregateHeuristics.push_back (other);

  //give new heuristic likelihoods for current paths
  for (MasterPathLikelihoodTable::iterator tables = m_masterLikelihoods->begin ();
       tables != m_masterLikelihoods->end (); tables++)
    {
      PathLikelihoodInnerTable * newTable = &(other->m_likelihoods[tables->first]);
      for (MasterPathLikelihoodInnerTable::iterator pathLh = tables->second.begin ();
           pathLh != tables->second.end (); pathLh++)
        {
          Ptr<RonPath> path = pathLh->first;
          Ptr<Likelihood> lh = (pathLh->second);
          Ptr<Likelihood> newLh = lh->AddLh ();
          (*newTable)[path] = newLh;
        }
    }
}


void
RonPathHeuristic::Clear ()
{
  m_topLevel = NULL;
  m_aggregateHeuristics.clear ();
  m_likelihoods.clear ();
  m_masterLikelihoods->clear ();
}

void
RonPathHeuristic::SetPeerTable (Ptr<RonPeerTable> table)
{
  m_peers = table;
}


void
RonPathHeuristic::SetSourcePeer (Ptr<RonPeerEntry> peer)
{
  m_source = peer;
}


void
RonPathHeuristic::SetLikelihood (Ptr<RonPath> path, double lh)
{
  NS_ASSERT_MSG (path->GetN () > 0, "got 0 length path from in SetLikelihood");

  EnsurePathRegistered (path);

  Ptr<PeerDestination> dest = path->GetDestination ();
  //NS_ASSERT (0.0 <= lh and lh <= 1.0);
  NS_LOG_LOGIC ("Path " << *path << " has LH " << lh);
  PathLikelihoodInnerTable * inner = &(m_likelihoods[dest]);
  NS_ASSERT_MSG (inner->size () > 0, "setting likelhiood but inner table is empty!");
  double realLh = lh*m_weight;
  (*inner)[path]->SetLh (realLh);
}


Ptr<RonPeerEntry>
RonPathHeuristic::GetSourcePeer ()
{
  return m_source;
}


// boost::function<bool (RonPeerEntry peer1, RonPeerEntry peer2)>
// //void *
// RonPathHeuristic::GetPeerComparator (Ptr<RonPeerEntry> destination /* = NULL*/)
// {
//   return boost::bind (&RonPathHeuristic::ComparePeers, this, destination, _1, _2);
// }


//////////////////// helper functions ////////////////////


bool
RonPathHeuristic::SameRegion (Ptr<RonPeerEntry> peer1, Ptr<RonPeerEntry> peer2)
{
  Vector3D loc1 = peer1->location;
  Vector3D loc2 = peer2->location;
  return (loc1.x == loc2.x and loc1.y == loc2.y) or
    (peer1->region == peer2->region);
  //TODO: actually define regions to be more than a coordinate
  /* test this later
     return peer1->GetObject<RonPeerEntry> ()->region == peer2->GetObject<RonPeerEntry> ()->region; */
}


bool
RonPathHeuristic::PathAttempted (Ptr<RonPath> path, bool recordPath)
{
  if (recordPath)
    {
      m_pathsAttempted.insert (path);
      return true;
    }
  else
    {
      return m_pathsAttempted.count (path);
    }
}


////////////////////////////// Likelihood object //////////////////////////////
RonPathHeuristic::Likelihood::Likelihood ()
{
  m_likelihood = 0.0;
}

RonPathHeuristic::Likelihood::Likelihood (double lh)
{
  m_likelihood = lh;
}

Ptr<RonPathHeuristic::Likelihood>
RonPathHeuristic::Likelihood::AddLh (double lh)
{
  Ptr<RonPathHeuristic::Likelihood> newLh = Create<RonPathHeuristic::Likelihood> ();
  newLh->SetLh (lh);
  m_otherLikelihoods.push_back (newLh);
  return newLh;
}

double
RonPathHeuristic::Likelihood::GetLh ()
{
  double total = m_likelihood;
  for (UnderlyingContainer::iterator itr = m_otherLikelihoods.begin ();
       itr != m_otherLikelihoods.end (); itr++)
    total += (*itr)->GetLh ();

  return total;
}

void
RonPathHeuristic::Likelihood::SetLh (double newLh)
{
  m_likelihood = newLh;
}

uint32_t
RonPathHeuristic::Likelihood::GetN ()
{
  uint32_t total = 1;
  for (UnderlyingContainer::iterator itr = m_otherLikelihoods.begin ();
       itr != m_otherLikelihoods.end (); itr++)
    total += (*itr)->GetN ();
  return total;
}
