/* 
 * File:   angle-ron-path-heuristic.h
 * Author: zhipengh
 *
 * Created on June 10, 2013, 9:52 PM
 */

#ifndef ANGLE_RON_PATH_HEURISTIC_H
#define	ANGLE_RON_PATH_HEURISTIC_H

#include "ron-path-heuristic.h"

namespace ns3 {

class AngleRonPathHeuristic : public RonPathHeuristic
{
public:
  AngleRonPathHeuristic ();
  static TypeId GetTypeId (void);
private:
  virtual double GetLikelihood (Ptr<RonPath> path);
  double GetDistance(double d1, double d2, double d3);
  double GetInitialLikelihood (double ang);
  double GetAngleLikelihood (double ang);
  void DoNotifyTimeout (Ptr<RonPath> path, Time time);
};

} //namespace

#endif	/* ANGLE_RON_PATH_HEURISTIC_H */

