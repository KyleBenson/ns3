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

#ifndef RON_PATH_HEURISTIC_H
#define RON_PATH_HEURISTIC_H

#include "ns3/core-module.h"
#include "ns3/random-variable-stream.h"

#include "ron-peer-table.h"
#include "ron-path.h"

/*#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/identity.hpp>*/
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include <boost/functional/hash.hpp>
//#include "boost/function.hpp"

#include <set>
#include <list>
#include <vector>

namespace ns3 {

/** This class represents a heuristic for choosing overlay paths.  Derived classes must override the ComparePeers
    function to implement the actual heuristic logic.

    It includes logic for aggregating together multiple heuristics,
    which allows for composing meta-heuristics of multiple weighted
    heurstics.
    */
class RonPathHeuristic : public Object
{
public:
  static TypeId GetTypeId (void);

  RonPathHeuristic ();
  virtual ~RonPathHeuristic ();

  typedef std::vector<Ptr<RonPath> > RonPathContainer;
  /** Return the best path, according to the aggregate heuristics, to the destination. */
  Ptr<RonPath> GetBestPath (Ptr<PeerDestination> destination);
  /** Return the best multi-path, according to the aggregate heuristics, to the destination. */
  RonPathContainer GetBestMultiPath (Ptr<PeerDestination> destination, uint32_t multipathFanout);

  /** Return the best multicast path, according to the aggregate heuristics, to the destinations. */
  //TODO: Ptr<RonPath> GetBestMulticastPath (itr<Ptr<RonPeerEntry???> destinations);

  /** Return the best anycast path, according to the aggregate heuristics, to the destination. */
  //TODO: Ptr<RonPath> GetBestAnycastPath (itr<Ptr<RonPeerEntry???> destinations);

  /** Inform this heuristic that it will be the top-level for an aggregate heuristic group. */
  void MakeTopLevel ();

  void AddHeuristic (Ptr<RonPathHeuristic> other);

  /** Clear aggregate heuristics so that ns-3 can properly release their memory. */
  void Clear ();

  void SetPeerTable (Ptr<RonPeerTable> table);
  void SetSourcePeer (Ptr<RonPeerEntry> peer);
  Ptr<RonPeerEntry> GetSourcePeer ();

  ////////////////////  notifications / updates  /////////////////////////////////

  /** Application will notify heuristics about ACKs. */
  void NotifyAck (Ptr<RonPath> path, Time time);

  /** Override this function to control what happens when this heuristic receives an ACK notification.
      Default is to set likelihoods to 1 and update contact times. */
  virtual void DoNotifyAck (Ptr<RonPath> path, Time time);

  /** Application will notify heuristics about timeouts. */
  void NotifyTimeout (Ptr<RonPath> path, Time time);

  /** Override this function to control what happens when this heuristic receives a timeout notification.
      Default is to set first likelihood on path to 0, unless an ACK of the partial path occurred. */
  virtual void DoNotifyTimeout (Ptr<RonPath> path, Time time);

  class NoValidPeerException : public std::exception
  {
  public:
    virtual const char* what() const throw()
    {
      return "No valid peers could be chosen for the overlay.";
    }
  };

  //for whatever reason, friendship isn't working...
  friend class TestRonPathHeuristic; //testing framework
  //protected:

  ///////////////////////// likelihood manipulation ////////////////////

  /** Sets the estimated likelihood of reaching the specified peer */
  void SetLikelihood (Ptr<RonPath> path, double lh);

  /** Get the likelihood of path being a successful overlay for reaching destination.
      By default, always assumes destination is reachable, i.e. always returns 1.0.
      The MOST IMPORTANT method to override/implement.
   */
  virtual double GetLikelihood (Ptr<RonPath> path);

  /** Adds the given path to the master likelihood table of the top-level heuristic,
      updates all of the other likelhood tables. */
  void AddPath (Ptr<RonPath> path);

  /** Runs DoBuildPaths on all heuristics. */
  void BuildPaths (Ptr<PeerDestination> destination);

  /** Generate enough paths for the heuristics to use.
      By default, it will generate all possible one-hop paths to the destination the first
      time it sees that destination.  It will NOT generate duplicates.
      Override for multi-hop paths or to prune this search tree.
      NOTE: make sure you check for duplicate paths when using aggregate heuristics! */
  virtual void DoBuildPaths (Ptr<PeerDestination> destination);

  /** Handles adding a path to the master likelihood table and updating all other heuristics
      and their associated likelihood tables. */
  void AddPath (RonPath path);

  /** Adds the path to the heuristic if it does not exist already,
      ensuring we don't get Null pointers when accessing Likelihoods. */
  void EnsurePathRegistered (Ptr<RonPath> path);

  /** Set the estimated likelihood of reaching the destination through each peer. 
      Assumes that we only need to assign random probs once, considered an off-line heuristic.
      Also assumes we should set LH to 0 if we failed to reach a peer.
      Override to change these assumptions (i.e. on-line heuristics),
      but take care to ensure good algorithmic complexity!
  */
  virtual void DoUpdateLikelihoods (Ptr<PeerDestination> destination);
  /** Ensures that DoUpdateLikelihoods is run for each aggregate heuristic */
  void UpdateLikelihoods (Ptr<PeerDestination> destination);

  bool SameRegion (Ptr<RonPeerEntry> peer1, Ptr<RonPeerEntry> peer2);

  /** Returns true if the given path has been attempted, false otherwise.
      Specifying the recordPath argument will add the given path to the set of
      attempted paths if true. */
  bool PathAttempted (Ptr<RonPath> path, bool recordPath=false);

  /** May only need to assign likelihoods once.
      Set this to true to avoid clearing LHs after each newly chosen peer. */
  bool m_updatedOnce;
  Ptr<RonPeerTable> m_peers;
  Ptr<UniformRandomVariable> m_random; //for random decisions
  Ptr<RonPeerEntry> m_source;
  std::string m_summaryName;
  std::string m_shortName;
  double m_weight;

  /** This  helps us aggregate the likelihoods together from each of the
      aggregate heuristics. */
  class Likelihood : public SimpleRefCount<Likelihood>
  {
  private:
    double m_likelihood;
    typedef std::list<Ptr<Likelihood> > UnderlyingContainer;
    UnderlyingContainer m_otherLikelihoods;

  public:
    Likelihood ();
    Likelihood (double lh);
    Ptr<Likelihood> AddLh (double lh = 0.0);
    double GetLh ();
    void SetLh (double newLh);
    uint32_t GetN ();
  };

  typedef std::list<Ptr<RonPathHeuristic> > AggregateHeuristics;
  AggregateHeuristics m_aggregateHeuristics;

  /** The likelihoods are indexed by the destination and then the path proposed for the destination. */
  //private:
  struct PathTestEqual
  {
    inline bool operator() (const Ptr<RonPath> path1, const Ptr<RonPath> path2) const
    {
      return *path1 == *path2;
    }
  };

  struct PeerDestinationHasher
  {
    inline size_t operator()(const Ptr<PeerDestination> hop) const
    {
      //TODO: combine all peers on a destination and whatever else makes it unique....
      NS_ASSERT_MSG (hop->GetN (), "can't hash an empty destination!");
      boost::hash<uint32_t> hasher;
      Ptr<RonPeerEntry> firstPeer = (*hop->Begin ());
      //uint32_t uniqueValue = firstPeer->id;
      uint32_t uniqueValue = firstPeer->address.Get ();

      //TODO: because we currently don't store the full peer inside headers, we don't always have the id
      //and so we're using the address for now instead.... id would be more robust for multiple interfaces
      //but we currently don't have that feature so no worries... for now
      return hasher (uniqueValue);
    }
  };

  struct PathHasher
  {
    inline size_t operator()(const Ptr<RonPath> path) const
    {
      NS_ASSERT_MSG (path->GetN (), "can't hash an empty path!");
      PeerDestinationHasher hasher;

      std::size_t hashValue = 0;
      for (RonPath::Iterator itr = path->Begin ();
           itr != path->End (); itr++)
        boost::hash_combine (hashValue, hasher (*itr));

      return hashValue;
    }
  };

  typedef boost::unordered_set<Ptr<RonPath>, PathHasher, PathTestEqual> PathsAttempted;
  typedef PathsAttempted::iterator PathsAttemptedIterator;
  //typedef std::set<Ptr<RonPath>> PathsAttempted;
  PathsAttempted m_pathsAttempted;

  typedef Ptr<RonPath> PathLikelihoodInnerTableKey;
  typedef PathTestEqual PathLikelihoodInnerTableTestEqual;
  typedef PathHasher PathLikelihoodInnerTableHasher;

  //outer table
  
  typedef Ptr<PeerDestination> PathLikelihoodTableKey;
  //compare the pointers by attributes of their contained elements
  struct PathLikelihoodTableTestEqual
  {
    bool operator() (const PathLikelihoodTableKey key1, const PathLikelihoodTableKey key2) const
    {
      return (*key1->Begin ())->id == (*key2->Begin ())->id;
    }
  };

  //hash 
  typedef PeerDestinationHasher PathLikelihoodTableHasher;

  //table definitions

  typedef boost::unordered_map<PathLikelihoodInnerTableKey, Ptr<Likelihood>,
                               PathLikelihoodInnerTableHasher, PathLikelihoodInnerTableTestEqual> PathLikelihoodInnerTable;

  //typedef std::map<RonPath, double *> PathLikelihoodInnerTable;
  typedef boost::unordered_map<PathLikelihoodTableKey, PathLikelihoodInnerTable,
                               PathLikelihoodTableHasher, PathLikelihoodTableTestEqual> PathLikelihoodTable;
  //typedef std::map<Ptr<PeerDestination>, PathLikelihoodInnerTable> PathLikelihoodTable;
  PathLikelihoodTable m_likelihoods;

  /** Only the top-level of a group of aggregate heuristics should manage this. */
  typedef boost::unordered_map<PathLikelihoodInnerTableKey, Ptr<Likelihood>,
                               PathLikelihoodInnerTableHasher, PathLikelihoodInnerTableTestEqual> MasterPathLikelihoodInnerTable;
  //typedef std::map<RonPath, std::list<double> > MasterPathLikelihoodInnerTable;
  typedef boost::unordered_map<PathLikelihoodTableKey, MasterPathLikelihoodInnerTable,
                               PathLikelihoodTableHasher, PathLikelihoodTableTestEqual> MasterPathLikelihoodTable;
  //typedef std::map<Ptr<PeerDestination>, MasterPathLikelihoodInnerTable> MasterPathLikelihoodTable;
  MasterPathLikelihoodTable * m_masterLikelihoods;

  Ptr<RonPathHeuristic> m_topLevel;







 
  //TODO: move this to the PeerTable when it's COW
  //likelihoods are addressed in several ways by different heuristics
  /*typedef
  boost::multi_index_container<
    std::pair<double, Ptr<RonPeerEntry> >,
    indexed_by<
      ordered_unique<identity<double> >,
      hashed_unique<Ptr<RonPeerEntry> >,
      >
      >*/

  // typedef LikelihoodTable::nth_index<0>::type::iterator LikelihoodIterator;
  // typedef LikelihoodTable::nth_index<1>::type::iterator LikelihoodPeerIterator;
  // typedef LikelihoodTable::nth_index<1>::type LikelihoodPeerMap;  

  //boost::function<bool (RonPeerEntry peer1, RonPeerEntry peer2)> GetPeerComparator (Ptr<RonPeerEntry> destination = NULL);
};
} //namespace
#endif //RON_PATH_HEURISTIC_H
