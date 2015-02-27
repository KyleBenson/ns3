/* 
 * File:   furtherest-first-ron-path-heuristic.cc
 * Author: zhipengh
 *
 * Created on June 10, 2013, 10:29 PM
 */

#include "furtherest-first-ron-path-heuristic.h"
#include <cmath>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("FurtherestFirstRonPathHeuristic");

NS_OBJECT_ENSURE_REGISTERED (FurtherestFirstRonPathHeuristic);

FurtherestFirstRonPathHeuristic::FurtherestFirstRonPathHeuristic ()
{
}

TypeId
FurtherestFirstRonPathHeuristic::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FurtherestFirstRonPathHeuristic")
    .AddConstructor<FurtherestFirstRonPathHeuristic> ()
    .SetParent<RonPathHeuristic> ()
    .SetGroupName ("RonPathHeuristics")
    .AddAttribute ("SummaryName", "Short name that summarizes parameters, aggregations, etc. to be used when creating filenames",
                   StringValue ("far"),
                   MakeStringAccessor (&FurtherestFirstRonPathHeuristic::m_summaryName),
                   MakeStringChecker ())
    .AddAttribute ("ShortName", "Short name that only captures the name of the heuristic",
                   StringValue ("far"),
                   MakeStringAccessor (&FurtherestFirstRonPathHeuristic::m_shortName),
                   MakeStringChecker ())
    .AddAttribute ("MinDistance", "Minimum distance we will permit for peers.",
                   DoubleValue (1.0),
                   MakeDoubleAccessor (&FurtherestFirstRonPathHeuristic::m_minDistance),
                   MakeDoubleChecker<double> ())
    ;
  //tid.SetAttributeInitialValue ("ShortName", Create<StringValue> ("ortho"));
  //tid.SetAttributeInitialValue ("SummaryName", Create<StringValue> ("ortho"));

  return tid;
}


double
FurtherestFirstRonPathHeuristic::GetLikelihood (Ptr<RonPath> path)
{
  NS_ASSERT_MSG (m_source, "You must set the source peer before using the heuristic!");
  
  Vector3D va = m_source->location;
  Vector3D vb = (*(*path->Begin ())->Begin ())->location;
  double distance = CalculateDistance (va, vb);
  double likelihood;
  
  NS_LOG_DEBUG ("the distance is "<<distance);

  if (distance < m_minDistance)
      return 0;  
  else 
  {
      likelihood = (distance - m_minDistance);
      NS_LOG_DEBUG ("the furtherest likelihood is "<<likelihood);
  }
  
  return likelihood;
}
