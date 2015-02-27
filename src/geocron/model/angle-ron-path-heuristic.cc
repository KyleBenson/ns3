/* 
 * File:   angle-ron-path-heuristic.cc
 * Author: zhipengh
 *
 * Created on June 10, 2013, 9:52 PM
 */

#include "angle-ron-path-heuristic.h"
#include <cmath>

using namespace ns3;

double pastLikelihood = 0.0;

NS_LOG_COMPONENT_DEFINE ("AngleRonPathHeuristic");
NS_OBJECT_ENSURE_REGISTERED (AngleRonPathHeuristic);

AngleRonPathHeuristic::AngleRonPathHeuristic ()
{
}

TypeId
AngleRonPathHeuristic::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::AngleRonPathHeuristic")
    .AddConstructor<AngleRonPathHeuristic> ()
    .SetParent<RonPathHeuristic> ()
    .SetGroupName ("RonPathHeuristics")
    .AddAttribute ("SummaryName", "Short name that summarizes parameters, aggregations, etc. to be used when creating filenames",
                   StringValue ("angle"),
                   MakeStringAccessor (&AngleRonPathHeuristic::m_summaryName),
                   MakeStringChecker ())
    .AddAttribute ("ShortName", "Short name that only captures the name of the heuristic",
                   StringValue ("angle"),
                   MakeStringAccessor (&AngleRonPathHeuristic::m_shortName),
                   MakeStringChecker ())
    ;
  //tid.SetAttributeInitialValue ("ShortName", Create<StringValue> ("ortho"));
  //tid.SetAttributeInitialValue ("SummaryName", Create<StringValue> ("ortho"));

  return tid;
}

//The function that calculate the angle
double AngleRonPathHeuristic::GetDistance (double d1, double d2, double d3)
{
  if (d1 == 0 or d2 == 0 or d3 == 0)
    return 0;

  //std::cout << "locations: " << d1 << ", " << d2 << ", " << d3 << std::endl;
  double ang = acos ((d2 * d2 + d1 * d1 - d3 * d3) /
                       (2.0 * d2 * d1));
  //std::cout << "angle: " << ang << std::endl;
  return ang;
}

//function for the initial likelihood to be used
double AngleRonPathHeuristic::GetInitialLikelihood (double ang)
{
  double likelihood;
  double pi = 3.14159265;

  if(ang < pi)
     likelihood = fabs(cos(ang - pi*0.25));

  else if (ang > pi) {
     ang = pi*2 - ang;
     likelihood = fabs(cos(2*(ang - pi*0.25)/3));
  }

  return likelihood;
}

double AngleRonPathHeuristic::GetLikelihood (Ptr<RonPath> path)
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
  //double orthogonal = pi/2.0;

  /*  We have a triangle where the source is point a, destination is point b, and overlay peer is c.
      We compute angle c and want it to be as close to right as possible (hence orthogonal).
      We also want the distance of the line from c to a point on line ab such that the lines are perpendicular. */
  Vector3D va = m_source->location;
  Vector3D vb = destination->location;
  Vector3D vc = peer->location;

  double ab_dist = CalculateDistance (va, vb);
  double ac_dist = CalculateDistance (va, vc);
  double bc_dist = CalculateDistance (vb, vc);

  // compute angles with law of cosines
  double a_ang = GetDistance(ab_dist, ac_dist, bc_dist);
  //std::cout<<"the angle is "<<a_ang<<std::endl;
  
  // Throw away obtuse triangles and ensure perpDist is in bounds
  //we pick the max distance to be twice the ideal (i.e. ab_dist) so we can easily normalize the error
  //TODO: come up with something better, perhaps exponential or other super-linear function?
  if ((a_ang == pi) or a_ang == 0.0)
    {
      SetLikelihood (path, 0);
      return 0;
    }

  //double ideal_ang = orthogonal / 2; //taking angle c
  
  double newLikelihood = 0.0;

  newLikelihood = GetInitialLikelihood(a_ang);
  //std::cout<<"initial likelihood = "<<newLikelihood<<std::endl;
  NS_ASSERT_MSG (newLikelihood, "newLikelihood gives the wrong answer");

  // set a point to represent the current path being considered
  vb = peer->location;
 
  // scale likelihood based on the previous attempts
  for (PathsAttemptedIterator itr = m_pathsAttempted.begin();
       itr != m_pathsAttempted.end (); itr++)
    {
      // get a point to represent the current attempted path under consideration
      Ptr<PeerDestination> thisDest = *((*itr)->Begin ());
      vc = (*(thisDest->Begin ()))->location;
      
      //std::cout << "vectors are: " << va << ", " << vb << ", " << vc << std::endl;

      ab_dist = CalculateDistance (va, vb);
      ac_dist = CalculateDistance (va, vc);
      bc_dist = CalculateDistance (vb, vc);
      
      double angle = GetDistance (ab_dist, ac_dist, bc_dist);
      if (bc_dist == 0)
	angle = 0;
      double thisLh = GetAngleLikelihood (angle);

      //std::cout << "scaling by LH " << thisLh << std::endl;

      newLikelihood *= thisLh;
    }

  NS_LOG_DEBUG (
  //std::cout <<
		"source(" << m_source->location.x << "," << m_source->location.y
                << "), dest(" << destination->location.x << "," << destination->location.y
                << "), peer(" << peer->location.x << "," << peer->location.y
                << ")'s LH = " << newLikelihood
		//<< std::endl;
  );

  return newLikelihood;
}

double
AngleRonPathHeuristic::GetAngleLikelihood (double ang)
{
  //TODO: implement
  return fabs(sin(ang/2.0));
}


void
AngleRonPathHeuristic::DoNotifyTimeout (Ptr<RonPath> path, Time time)
{
  EnsurePathRegistered (path);
  SetLikelihood (path, 0.0);
  m_updatedOnce = false; //so the LHs will be updated next time GetBestPath is called
}
