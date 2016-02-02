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

#ifndef REGION_HELPER_H
#define REGION_HELPER_H

#include "ns3/core-module.h"
#include "region.h"

#include <boost/unordered_map.hpp>
#include <boost/lexical_cast.hpp>
#include <map>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

namespace ns3 {

  //NS_LOG_COMPONENT_DEFINE ("RegionHelper");

#define NO_LOCATION_VECTOR Vector (0.0, 0.0, -1.0)

#define NULL_REGION "NO_REGION"

  // HELPER FUNCTIONS

//these rely on the NO_LOCATION_VECTOR
Vector GetLocation (Ptr<Node> node);
bool HasLocation (Ptr<Node> node);

  // BEGIN ACTUAL REGION HELPER

  //TODO: some real inheritance instead of this wasteful silliness

/** Simple helper class for assigning Regions for specific location Vectors. */
class RegionHelper : public Object
{
  //boost::unordered_map<Vector, Location> regions;
  typedef std::map<Vector, Location> UnderlyingMapType;
  UnderlyingMapType regions;
public:
  typedef UnderlyingMapType::iterator Iterator;

  RegionHelper () {
    regions[NO_LOCATION_VECTOR] = NULL_REGION;
  }

  static TypeId GetTypeId () {
    static TypeId tid = TypeId ("ns3::RegionHelper")
      .SetParent<Object> ()
      .AddConstructor<RegionHelper> ()
      ;
    return tid;
  }

  virtual Location GetRegion (Vector loc) {
    NS_ASSERT_MSG(regions.count(loc), "No region for this location!");
    return regions[loc];
  }

  // Sets the Region associated with the given Vector location
  // Returns the old Location, if one existed, the new one otherwise
  virtual Location SetRegion (Vector loc, Location newRegion) {
    Location retLoc = (regions.count(loc) ? regions[loc] : newRegion);
    regions[loc] = newRegion;

    return retLoc;
  }

  virtual Vector GetLocation (Location reg) {
    for (Iterator itr = regions.begin(); itr != regions.end(); itr++)
      {
        if (itr->second == reg)
          {
            return itr->first;
          }
      }
    return NO_LOCATION_VECTOR;
  }
};  

// For Rocketfuel topologies, we have to explicitly set Region info anyway so...
typedef RegionHelper RocketfuelRegionHelper;

//TODO: different sized regions
class BriteRegionHelper : public RegionHelper {
  // size, one one edge, of the square representing the whole topology plane
  double boundaryLength;
  double regionSplitFactor;

  // in this RegionHelper, we just use the Vectors in the other one to represent the top-left 
public:

  BriteRegionHelper ()
  {
    //TODO: other values
    regionSplitFactor = 5;
    boundaryLength = -1;
  }

  static TypeId GetTypeId () {
    static TypeId tid = TypeId ("ns3::BriteRegionHelper")
      .SetParent<RegionHelper> ()
      .AddConstructor<BriteRegionHelper> ()
      ;
    return tid;
  }

  virtual Location GetRegion (Vector loc) {
    // get the region by first rounding the given vector to the responsible (next lowest) whole 'region cell number',
    // which is an ordered pair, and then make that into a string region

    //TODO: non-string region
    Location reg;

    uint32_t responsibleX = floor(loc.x / GetRegionSize ()),
      responsibleY = floor(loc.y / GetRegionSize ());

    // concatenate strings
    reg = boost::lexical_cast<std::string> (responsibleX) + "," + boost::lexical_cast<std::string> (responsibleY);

    return reg;
  }

  // Define new version here so we can compute the disaster location
  // directly from the region its specified in
  virtual Vector GetLocation (Location reg) {
    // Split the location into actual ints
    std::vector<std::string> locationParts;
    boost::algorithm::split (locationParts, reg,
        boost::algorithm::is_any_of (","));
    Vector gridCell (boost::lexical_cast<int> (locationParts[0]),
          boost::lexical_cast<int> (locationParts[0]), 0);

    // Now convert grid cell location into actual geographic location
    // of the cell's center.
    return Vector ((gridCell.x + 0.5) * GetRegionSize (),
           (gridCell.y + 0.5) * GetRegionSize (), 0);
  }

  double GetRegionSize ()
  {
    //TODO: different region numbers
    return boundaryLength / regionSplitFactor;
  }

  void SetTopologySize (double newBoundaryLength)
  {
    boundaryLength = newBoundaryLength;
  }
};

} //namespace
#endif //REGION_HELPER_H
