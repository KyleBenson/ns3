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

#ifndef INTERSECTION_GEODIVRP_RON_PATH_HEURISTIC_H
#define INTERSECTION_GEODIVRP_RON_PATH_HEURISTIC_H

#include "geodivrp-ron-path-heuristic.h"

namespace ns3 {

/** This Geodivrp heuristic assigns diversity values to pairs of paths
 * based on the size of their intersection, scaled by the path size to
 * prevent bias towards super-long paths. */
class IntersectionGeodivrpRonPathHeuristic : public GeodivrpRonPathHeuristic
{
public:
  IntersectionGeodivrpRonPathHeuristic ();
  static TypeId GetTypeId (void);
  /** Assigns diversity based on the size of the paths' intersection. */
  virtual double GetDiversity (Ptr<PhysicalPath> path1, Ptr<PhysicalPath> path2);
};

} //namespace
#endif //INTERSECTION_GEODIVRP_RON_PATH_HEURISTIC_H
