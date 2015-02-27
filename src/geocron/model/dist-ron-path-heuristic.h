/* 
 * File:   dist-ron-path-heuristic.h
 * Author: zhipengh
 *
 * Created on June 12, 2013, 2:25 PM
 */

#ifndef DIST_RON_PATH_HEURISTIC_H
#define	DIST_RON_PATH_HEURISTIC_H

#include "ron-path-heuristic.h"

namespace ns3 {

class DistRonPathHeuristic : public RonPathHeuristic
{
public:
  DistRonPathHeuristic ();
  static TypeId GetTypeId (void);
private:
  /** Minimum distance we will permit for peers.
      Necessary for assigning a LH given the distance as we need a bound. */
  double m_minDistance;
  virtual double GetLikelihood (Ptr<RonPath> path);
};

} //namespace

#endif	/* DIST_RON_PATH_HEURISTIC_H */

