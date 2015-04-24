/* 
 * File:   furthest-first-ron-path-heuristic.h
 * Author: zhipengh
 *
 * Created on June 10, 2013, 10:29 PM
 */

#ifndef FARTHEST_FIRST_RON_PATH_HEURISTIC_H
#define	FARTHEST_FIRST_RON_PATH_HEURISTIC_H

#include "ron-path-heuristic.h"

namespace ns3 {

class FarthestFirstRonPathHeuristic : public RonPathHeuristic
{
public:
  FarthestFirstRonPathHeuristic ();
  static TypeId GetTypeId (void);
private:
  /** Minimum distance we will permit for peers.
      Necessary for assigning a LH given the distance as we need a bound. */
  double m_minDistance;
  virtual double GetLikelihood (Ptr<RonPath> path);
};

} //namespace

#endif	/* FARTHEST_FIRST_RON_PATH_HEURISTIC_H */

