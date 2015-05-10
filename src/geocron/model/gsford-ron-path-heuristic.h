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

#ifndef GSFORD_RON_PATH_HEURISTIC_H
#define GSFORD_RON_PATH_HEURISTIC_H

#include "ron-path-heuristic.h"
#include "physical-path.h"

namespace ns3 {

/** This heuristic implements the logic from Kyungbaek Kim's 2010 paper
 * "Assessing the Impact of Geographically Correlated Failures on Overlay-Based Data Dissemination".
 * It picks multiple paths by comparing all of the nodes in one path with all the nodes in the other,
 * penalizing the path under consideration for each pair of nodes that are within some distance threshold
 * of each other. */
class GsfordRonPathHeuristic : public RonPathHeuristic
{
public:
  GsfordRonPathHeuristic ();
  static TypeId GetTypeId (void);
private:
  virtual double GetLikelihood (Ptr<RonPath> path);

  // Threshold at which two nodes are considered too close to be geo-diverse in paths
  double m_distanceThreshold;
};

} //namespace
#endif //GSFORD_RON_PATH_HEURISTIC_H
