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

#ifndef ORTHOGONAL_RON_PATH_HEURISTIC_H
#define ORTHOGONAL_RON_PATH_HEURISTIC_H

#include "ron-path-heuristic.h"

namespace ns3 {

class OrthogonalRonPathHeuristic : public RonPathHeuristic
{
public:
  OrthogonalRonPathHeuristic ();
  static TypeId GetTypeId (void);
private:
  virtual double GetLikelihood (Ptr<RonPath> path);
};

} //namespace
#endif //ORTHOGONAL_RON_PATH_HEURISTIC_H
