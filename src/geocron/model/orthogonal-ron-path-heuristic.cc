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

#include "orthogonal-ron-path-heuristic.h"
#include <cmath>

using namespace ns3;


NS_LOG_COMPONENT_DEFINE ("OrthogonalRonPathHeuristic");
NS_OBJECT_ENSURE_REGISTERED (OrthogonalRonPathHeuristic);

OrthogonalRonPathHeuristic::OrthogonalRonPathHeuristic ()
{
}

TypeId
OrthogonalRonPathHeuristic::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::OrthogonalRonPathHeuristic")
    .AddConstructor<OrthogonalRonPathHeuristic> ()
    .SetParent<RonPathHeuristic> ()
    .SetGroupName ("RonPathHeuristics")
    .AddAttribute ("SummaryName", "Short name that summarizes parameters, aggregations, etc. to be used when creating filenames",
                   StringValue ("ortho"),
                   MakeStringAccessor (&OrthogonalRonPathHeuristic::m_summaryName),
                   MakeStringChecker ())
    .AddAttribute ("ShortName", "Short name that only captures the name of the heuristic",
                   StringValue ("ortho"),
                   MakeStringAccessor (&OrthogonalRonPathHeuristic::m_shortName),
                   MakeStringChecker ())
    ;
  //tid.SetAttributeInitialValue ("ShortName", Create<StringValue> ("ortho"));
  //tid.SetAttributeInitialValue ("SummaryName", Create<StringValue> ("ortho"));

  return tid;
}


double
OrthogonalRonPathHeuristic::GetLikelihood (Ptr<RonPath> path)
{
  NS_ASSERT_MSG (m_source, "You must set the source peer(s) before using the heuristic!");

  Ptr<RonPeerEntry> peer = (*(*path->Begin ())->Begin ());
  Ptr<RonPeerEntry> destination = *path->GetDestination ()->Begin ();

  // don't bother solving if peer is within the source or destination's region
  if (SameRegion (m_source, peer) or SameRegion (destination, peer))
    {
      //std::cout << "Ignoring peer at location (" << (*peer)->location.x << "," << (*peer)->location.y << ")" <<std::endl;
      SetLikelihood (path, 0);
      return 0;
    }

  double pi = 3.14159265;
  double orthogonal = pi/2.0;

  /*  We have a triangle where the source is point a, destination is point b, and overlay peer is c.
      We compute angle c and want it to be as close to right as possible (hence orthogonal).
      We also want the distance of the line from c to a point on line ab such that the lines are perpendicular. */
  Vector3D va = m_source->location;
  Vector3D vb = destination->location;
  Vector3D vc = peer->location;

  double ab_dist = CalculateDistance (va, vb);
  double ac_dist = CalculateDistance (va, vc);
  double bc_dist = CalculateDistance (vb, vc);

  // ideal distance is when c is located halfway between a and b,
  // which would make an isosceles triangle, so legs are easy to compute
  // double half_ab_dist = 0.5*ab_dist;
  // double ac_ideal_dist = sqrt (2.0) * half_ab_dist;
  // double ideal_dist = sqrt (ac_ideal_dist * ac_ideal_dist - half_ab_dist * half_ab_dist);
  double ideal_dist = fabs(0.5*ab_dist);

  // compute angles with law of cosines
  double c_ang = acos ((ac_dist * ac_dist + bc_dist * bc_dist - ab_dist * ab_dist) /
                       (2.0 * ac_dist * bc_dist));

  // compute bc angles for finding the perpendicular distance
  double a_ang = acos ((ac_dist * ac_dist + ab_dist * ab_dist - bc_dist * bc_dist) /
                       (2.0 * ac_dist * ab_dist));

  // find perpendicular distances using law of sines
  double perpDist = fabs(ac_dist * sin (a_ang));

  //std::cout << "Path from " << m_source->location << " to " << destination->location << " thru " << peer->location <<
  //" makes C angle " << c_ang << std::endl;

  // Throw away obtuse triangles and ensure perpDist is in bounds
  //we pick the max distance to be twice the ideal (i.e. ab_dist) so we can easily normalize the error
  //TODO: come up with something better, perhaps exponential or other super-linear function?
  if (a_ang > orthogonal or (c_ang + a_ang < orthogonal) or perpDist > ab_dist)
    {
      SetLikelihood (path, 0);
      return 0;
    }

  // find 'normalized (0<=e<=1) percent error' from ideals
  //-exp ensures 0<=e<=1 and further penalizes deviations from the ideal
  // std::cout << "ideal_dist=" << ideal_dist <<std::endl;
  // std::cout << "c_ang=" << c_ang <<std::endl;
  // double ang_err = exp(-1*fabs((c_ang - orthogonal) / orthogonal));
  // double dist_err = exp(-1*fabs((perpDist - ideal_dist) / ideal_dist));

  //since we threw away obtuse triangles, the max angular error is 45 deg
  //double ang_err = (fabs((c_ang - orthogonal) / (pi/4.0)));
  //double ideal_ang = orthogonal / 2.0; //taking angle a
  double ideal_ang = orthogonal; //taking angle c
  //double ang_err = fabs(ideal_ang - a_ang);
  double ang_err = fabs(ideal_ang - c_ang);
  double norm_ang_err = (ang_err) / ideal_ang;
  norm_ang_err = norm_ang_err * norm_ang_err; //square to further penalize ones farther away from ideal

  NS_ASSERT_MSG (norm_ang_err <= 1.0, "angle error not properly normalized: " << norm_ang_err);
  NS_ASSERT_MSG (norm_ang_err >= 0.0, "angle error not properly normalized: " << norm_ang_err);

  double dist_err = fabs(fabs(perpDist) - fabs(ideal_dist));
  double norm_dist_err = (dist_err) / ideal_dist;
  norm_dist_err = norm_dist_err * norm_dist_err; //square to further penalize ones farther away from ideal

  NS_ASSERT_MSG (norm_dist_err <= 1.0, "distance error not properly normalized: " << norm_dist_err);
  NS_ASSERT_MSG (norm_dist_err >= 0.0, "distance error not properly normalized: " << norm_dist_err);

  //std::cout << "dist_err=" << dist_err << ", ang_err=" << ang_err << std::endl;
  //std::cout << "orthogonal=" << orthogonal << ", ang_err=" << ang_err << std::endl;

  double newLikelihood = 0.5*((1.0 - norm_dist_err) + (1.0 - norm_ang_err));
  //double newLikelihood = (0.8*(1.0 - norm_dist_err) + 0.2*(1.0 - norm_ang_err));

  NS_LOG_DEBUG (
  //std::cout << 
    "source(" << m_source->location.x << "," << m_source->location.y
            << "), dest(" << destination->location.x << "," << destination->location.y
            << "), peer(" << peer->location.x << "," << peer->location.y
            << ")'s LH = " << newLikelihood
    //<<std::endl;
                );

  return newLikelihood;
}
