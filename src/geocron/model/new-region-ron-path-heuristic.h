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

#ifndef NEW_REGION_RON_PATH_HEURISTIC_H
#define NEW_REGION_RON_PATH_HEURISTIC_H

#include "ron-path-heuristic.h"

#include <boost/unordered_set.hpp>

namespace ns3 {

/** This heuristic gives 1 LH to any path whose first hop is in a region we have not
    yet attempted to contact, and 0 LH to any that we have attempted to reach and
    timed out when doing so. */
class NewRegionRonPathHeuristic : public RonPathHeuristic
{
public:
  NewRegionRonPathHeuristic ();
  static TypeId GetTypeId (void);
private:
  virtual double GetLikelihood (Ptr<RonPath> path);
  virtual void DoNotifyTimeout (Ptr<RonPath> path, Time time); 
  boost::unordered_set<Location> m_regionsAttempted;
};

} //namespace
#endif //NEW_REGION_RON_PATH_HEURISTIC_H
