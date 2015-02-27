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

#ifndef REGION_H
#define REGION_H

#include "ns3/internet-module.h"
#include "ns3/core-module.h"
#include "ns3/mobility-module.h"

#include "region.h"

#include <boost/range/adaptor/map.hpp>
#include <map>

//TODO: enum for choosing which heuristic?

namespace ns3 {
  typedef std::string Location; //TODO: make an object
  /*class Region : public SimpleRefCount<Region>
{
public:
  Region ();
  Region (Ptr<Node> node);
  };  */

} //namespace
#endif //REGION_H
