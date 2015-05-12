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

#ifndef AREA_DISTANCE_GEODIVRP_RON_PATH_HEURISTIC_H
#define AREA_DISTANCE_GEODIVRP_RON_PATH_HEURISTIC_H

#include "geodivrp-ron-path-heuristic.h"

namespace ns3 {

/** This Geodivrp heuristic assigns diversity values to pairs of paths
 * based on the size of their intersection, scaled by the path size to
 * prevent bias towards super-long paths. */
class AreaDistanceGeodivrpRonPathHeuristic : public GeodivrpRonPathHeuristic
{
public:
  AreaDistanceGeodivrpRonPathHeuristic ();
  static TypeId GetTypeId (void);
  /** Assigns diversity based on the sum of two weighted components:
   * the squared minimum distance of any components in the path and
   * the area covered by the polygon formed by the two paths. */
  virtual double GetDiversity (Ptr<PhysicalPath> path1, Ptr<PhysicalPath> path2);
  static double GetArea (Ptr<PhysicalPath> path1, Ptr<PhysicalPath> path2);
  static double GetMinDistance (Ptr<PhysicalPath> path1, Ptr<PhysicalPath> path2);
private:
  // scaling factors for GetDiversity()
  double m_alpha; //distance
  double m_beta; //area
};

} //namespace
#endif //AREA_DISTANCE_GEODIVRP_RON_PATH_HEURISTIC_H
