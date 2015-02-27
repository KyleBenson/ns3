/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "region-helper.h"

namespace ns3 {

Vector
GetLocation (Ptr<Node> node)
{
  return node->GetObject<MobilityModel> ()->GetPosition ();
}


bool
HasLocation (Ptr<Node> node)
{
  Ptr<MobilityModel> mob = node->GetObject<MobilityModel> ();
  if (mob == NULL)
    {
      NS_LOG_UNCOND ("Node " << node->GetId () << " has no MobilityModel!");
      return false;
    }
  if (mob->GetPosition () == NO_LOCATION_VECTOR)
    {
      return false;
    }
  return true;
}

} //namespace ns3
