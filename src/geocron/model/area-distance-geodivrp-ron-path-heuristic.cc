
/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright 2015 Kyle Benson
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

#include "area-distance-geodivrp-ron-path-heuristic.h"
#include "region-helper.h" //for GetLocation
#include <limits>

using namespace ns3;

NS_OBJECT_ENSURE_REGISTERED (AreaDistanceGeodivrpRonPathHeuristic);
NS_LOG_COMPONENT_DEFINE ("AreaDistanceGeodivrpRonPathHeuristic");

AreaDistanceGeodivrpRonPathHeuristic::AreaDistanceGeodivrpRonPathHeuristic ()
{
}

TypeId
AreaDistanceGeodivrpRonPathHeuristic::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::AreaDistanceGeodivrpRonPathHeuristic")
    .AddConstructor<AreaDistanceGeodivrpRonPathHeuristic> ()
    .SetParent<GeodivrpRonPathHeuristic> ()
    .SetGroupName ("RonPathHeuristics")
    .AddAttribute ("SummaryName", "Short name that summarizes parameters, aggregations, etc. to be used when creating filenames",
                   StringValue ("areageodivrp"),
                   MakeStringAccessor (&AreaDistanceGeodivrpRonPathHeuristic::m_summaryName),
                   MakeStringChecker ())
    .AddAttribute ("ShortName", "Short name that only captures the name of the heuristic",
                   StringValue ("areageodivrp"),
                   MakeStringAccessor (&AreaDistanceGeodivrpRonPathHeuristic::m_shortName),
                   MakeStringChecker ())
    .AddAttribute ("A", "Scaling factor alpha for the minimum distance squared component of this heuristic's diversity measurement",
                   DoubleValue (1.0),
                   MakeDoubleAccessor (&AreaDistanceGeodivrpRonPathHeuristic::m_alpha),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("B", "Scaling factor beta for the minimum distance squared component of this heuristic's diversity measurement",
                   DoubleValue (1.0),
                   MakeDoubleAccessor (&AreaDistanceGeodivrpRonPathHeuristic::m_beta),
                   MakeDoubleChecker<double> ())
    ;
  return tid;
}

double
AreaDistanceGeodivrpRonPathHeuristic::GetArea (Ptr<PhysicalPath> path1, Ptr<PhysicalPath> path2)
{
  if (path1 == path2)
  {
    NS_LOG_LOGIC ("Got same path in GetArea! Returning 0");
    return 0;
  }

  // First, we need to combine the paths and store them in a vector
  // Start by adding all but the first hop from path1
  std::vector<Vector> points;
  for (NodeContainer::Iterator itr = ++path1->NodesBegin ();
      itr != path1->NodesEnd (); itr++)
  {
    Vector loc = GetLocation (*itr);
    NS_ASSERT_MSG (loc != NO_LOCATION_VECTOR, "Node " << (*itr)->GetId () << " has no location!");
    points.push_back (loc);
  }
  // Pop the back to avoid adding the dest twice, reverse the path, 
  // then add the vertices from path2 to finish up
  points.pop_back ();
  std::reverse (points.begin (), points.end ());
  for (NodeContainer::Iterator itr = path2->NodesBegin ();
      itr != path2->NodesEnd (); itr++)
  {
    Vector loc = GetLocation (*itr);
    NS_ASSERT_MSG (loc != NO_LOCATION_VECTOR, "Node " << (*itr)->GetId () << " has no location!");
    points.push_back (loc);
  }

  // Now we compute the area using a simple algorithm that doesn't
  // handle the paths crossing each other...
  // Code based on that taken from http://alienryderflex.com/polygon_area/
  double area = 0;
  for (uint32_t i = 0, j = points.size () - 1; i < points.size (); i++)
  {
    area += (points[j].x + points[i].x) * (points[j].y - points[i].y);
    j = i;
  }
  area /= 2.0;

  // if the points are arranged counter-clockwise, the area will be
  // negative, so let's fix that
  area = std::abs (area);
  NS_LOG_LOGIC ("Area of " << area);
  return area;
}

double
AreaDistanceGeodivrpRonPathHeuristic::GetMinDistance (Ptr<PhysicalPath> path1, Ptr<PhysicalPath> path2)
{
  double minDistance = std::numeric_limits<double>::max ();

  // avoid comparing start with end since they'll ALWAYS have dist 0
  NodeContainer::Iterator thisLastNodeItr = path1->NodesBegin (), otherLastNodeItr = path2->NodesBegin ();
  for (uint32_t i = 1; i < path1->GetNNodes (); thisLastNodeItr++, i++);
  for (uint32_t i = 1; i < path2->GetNNodes (); otherLastNodeItr++, i++);

  for (NodeContainer::Iterator thisNodeItr = path1->NodesBegin ();
      thisNodeItr != path1->NodesEnd (); thisNodeItr++)
  {
    for (NodeContainer::Iterator otherNodeItr = path2->NodesBegin ();
        otherNodeItr != path2->NodesEnd (); otherNodeItr++)
    {
      // Skip comparing the source nodes and destination nodes as they'll
      // always have a distance of 0
      if ((thisNodeItr == thisLastNodeItr and otherNodeItr == otherLastNodeItr) or
          (thisNodeItr == path1->NodesBegin () and otherNodeItr == path2->NodesBegin ()))
        continue;

      Vector3D va = GetLocation (*thisNodeItr);
      Vector3D vb = GetLocation (*otherNodeItr);
      double distance = CalculateDistance (va, vb);

      if (distance < minDistance)
        minDistance = distance;
    }
  }
  NS_ASSERT_MSG (minDistance < std::numeric_limits<double>::max (), "minDistance is unchanged: empty paths?");
  return minDistance;
}

double
AreaDistanceGeodivrpRonPathHeuristic::GetDiversity (Ptr<PhysicalPath> path1, Ptr<PhysicalPath> path2)
{
  double area = GetArea (path1, path2);
  double minDist = GetMinDistance (path1, path2);
  NS_LOG_DEBUG ("Path " << *path1 << " and " << *path2 << " area: " << area << ", minDist: " << minDist);
  return m_alpha * minDist * minDist + m_beta * area;
}
