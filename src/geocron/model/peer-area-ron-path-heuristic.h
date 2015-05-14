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

#ifndef PEER_AREA_RON_PATH_HEURISTIC_H
#define PEER_AREA_RON_PATH_HEURISTIC_H

#include "ron-path-heuristic.h"

namespace ns3 {

/** Chooses diverse paths based on the area of the polygon formed
 * by the RonPaths under consideration.  It's similar in spirit to the
 * AreaDistanceGeodivrpRonPathHeuristic, but doesn't rely on knowledge
 * of the underlying physical topology and its routers' locations. */
class PeerAreaRonPathHeuristic : public RonPathHeuristic
{
public:
  PeerAreaRonPathHeuristic ();
  static TypeId GetTypeId (void);
  /** Assigns geo-diversity based on the area within the polygon
   * formed by the two RonPaths under consideration.  It uses the
   * same mechanism as the GeoDivRP heuristic for handling the choices
   * in the current multipath, but we repeat the logic somewhat as here
   * we consider RonPaths, not PhysicalPaths. */
  virtual double GetLikelihood (Ptr<RonPath> path);
  /** Returns the area within the polygon formed by the loctions of the
   * peers only on the two given paths (NO physical path use!).  This is
   * a more reasonable assumption than having information about the physical
   * locations of all the routers along the PhysicalPath. */
  double GetArea (Ptr<RonPath> path1, Ptr<RonPath> path2);
  /** Break ties by the length of the physical path.  We assume that
   * this is feasible in modern systems by using traceroute. */
  virtual double GetTieBreaker (Ptr<RonPath> leftPath, Ptr<RonPath> rightPath);
};

} //namespace
#endif //PEER_AREA_RON_PATH_HEURISTIC_H
