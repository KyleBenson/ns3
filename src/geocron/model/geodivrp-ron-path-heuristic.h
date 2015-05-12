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

#ifndef GEODIVRP_RON_PATH_HEURISTIC_H
#define GEODIVRP_RON_PATH_HEURISTIC_H

#include "ron-path-heuristic.h"
#include "physical-path.h"

namespace ns3 {

/** This heuristic implements the logic from the Kansas University
 * ResiliNets group's various papers.  We call it GeoDivRP here
 * after their proposed routing protocol that would use geo-location
 * information as in these heuristics.  This is mostly taken from 
 * the Rohrer2014 paper "Path diversification for future internet
 * end-to-end resilience and survivability"
 *
 * This is a base heuristic that accumulates the diversity for the
 * given path by calling out to the pure virtual GetDiversity
 * function that is to be implemented in other classes since
 * they propose multiple possible heuristics.  In particular,
 * they propose one using only topology information and one
 * also using location information of the topology components.
 */
class GeodivrpRonPathHeuristic : public RonPathHeuristic
{
public:
  static TypeId GetTypeId (void);
  /** This function returns a value measuring the expected diversity
   * by comparing the two given paths.  We make it public for testing
   * purposes. */
  virtual double GetDiversity (Ptr<PhysicalPath> path1, Ptr<PhysicalPath> path2) = 0;

private:
  /** Accumulates the diversity for the given path as compared with all
   * of the currently chosen paths in the current multipath. It calls
   * out to the GetDiversity function to do this as it's based on the
   * effective path diversity (EPD) function (see definition 3 in the
   * Rohrer2014 paper), though decomposed somewhat to only account for
   * the additional benefit this path adds, rather than the entire
   * EPD for a set of multipaths. */
  virtual double GetLikelihood (Ptr<RonPath> path);

  /** Ties broken by path length: prefer shorter ones. */
  double GetTieBreaker (Ptr<RonPath> leftPath, Ptr<RonPath> rightPath);
  // Scaling constant.  See implementation in GetTypeId for details.
  // NOTE: we're not using this currently as the EPD is specifically
  // for calculating the diversity introduced by an entire multipath choice
  //double m_lambda;
};

} //namespace
#endif //GEODIVRP_RON_PATH_HEURISTIC_H
