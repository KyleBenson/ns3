#include "failure-helper-functions.h"
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

#ifndef nslog
#define nslog(x) NS_LOG_UNCOND(x);
#endif

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("FailureHelperFunctions");

////////////////////////////////////////////////////////////////////////
////////////   Actual failure-related functions     ////////////////////
////////////////////////////////////////////////////////////////////////

// Fail links by turning off the net devices at each end
void FailIpv4 (Ptr<Ipv4> ipv4, uint32_t iface)
{
  ipv4->SetDown (iface);
  ipv4->SetForwarding (iface, false);
}

void UnfailIpv4 (Ptr<Ipv4> ipv4, uint32_t iface)
{
  ipv4->SetUp (iface);
  ipv4->SetForwarding (iface, true);
}

// Fail nodes by turning off all its net devices and prevent applications from running
// (must be called before starting simulator!)
void FailNode (Ptr<Node> node)
{
  Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();

  for (uint32_t iface = 0; iface < ipv4->GetNInterfaces (); iface++)
    {
      FailIpv4 (ipv4, iface);
    }

  for (uint32_t app = 0; app < node->GetNApplications (); app++)
    {
      node->GetApplication (app)->SetAttribute ("StopTime", (TimeValue(Time(0))));
    }
}

void UnfailNode (Ptr<Node> node, Time appStopTime)
{
  Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();

  for (uint32_t iface = 0; iface < ipv4->GetNInterfaces (); iface++)
    {
      UnfailIpv4 (ipv4, iface);
    }

  for (uint32_t app = 0; app < node->GetNApplications (); app++)
    {
      node->GetApplication (app)->SetAttribute ("StopTime", (TimeValue(appStopTime)));
    }
}


////////////////////////////////////////////////////////////////////////
////////////   Node helper functions     ///////////////////////////////
////////////////////////////////////////////////////////////////////////


Ipv4Address GetNodeAddress(Ptr<Node> node)
{
  NS_ASSERT_MSG (node->GetNDevices() > 1, "Node " << Names::FindName (node) << " only has a loopback device, cannot get its Address!");
  return ((Ipv4Address)(node->GetObject<Ipv4>()->GetAddress(1,0).GetLocal()));
  // for interfaces: //ronServer.Install (router_interfaces.Get (0).first->GetNetDevice (0)->GetNode ());
}

uint32_t GetNodeDegree(Ptr<Node> node)
{
  return node->GetNDevices() - 1; //assumes PPP links
}

Ptr<NetDevice> GetOtherNetDevice (Ptr<NetDevice> thisDev)
{
  NS_ASSERT_MSG (thisDev->IsPointToPoint (), "GetOtherNetDevice assumes PointToPointNetDevices!");
  Ptr<Channel> channel = thisDev->GetChannel ();
  return (channel->GetDevice (0) == thisDev ? channel->GetDevice (1) : channel->GetDevice (0));
}

// This is ripped directly from Ipv4NixVectorRouting and repeated here for convenience
Ptr<Node> GetNodeByIp (Ipv4Address dest)
{
  NodeContainer allNodes = NodeContainer::GetGlobal ();
  Ptr<Node> destNode;

  for (NodeContainer::Iterator i = allNodes.Begin (); i != allNodes.End (); ++i)
    {
      Ptr<Node> node = *i;
      Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
      if (ipv4->GetInterfaceForAddress (dest) != -1)
        {
          destNode = node;
          break;
        }
    }

  if (!destNode)
    {
      NS_LOG_ERROR ("Couldn't find dest node given the IP" << dest);
      return 0;
    }

  return destNode;
}

////////////////////////////////////////////////////////////////////////
////////////   Geocron Node type helper functions     //////////////////
////////////////////////////////////////////////////////////////////////

std::string GetNodeType (Ptr<Node> node)
{
  // First, get the name without any of the hierarchy
  std::string nodeName = GetNodeName (node);

  // Next we trim off all the numbers (the name/node id)
  std::vector<std::string> nameParts;
  boost::algorithm::split (nameParts, nodeName,
      boost::algorithm::is_any_of ("0123456789"));
  std::string result = nameParts[0];

  //NS_LOG_UNCOND ("Type for node " << nodeName << " is " << result);
  return result;
}

std::string GetNodeName (Ptr<Node> node)
{
  std::string nodeName = Names::FindPath (node);
  // Split off just the part of the name that represents the node category.
  // We do this by first trimming off everything before the '/'
  std::vector<std::string> nameParts;
  boost::algorithm::split (nameParts, nodeName,
      boost::algorithm::is_any_of ("/"));
  std::string result = nameParts[2];
  return result;
}

bool IsSeismicSensor (Ptr<Node> node)
{
  return GetNodeType (node) == "seismicSensor";
}

bool IsWaterSensor (Ptr<Node> node)
{
  return GetNodeType (node) == "waterSensor";
}

bool IsBasestation (Ptr<Node> node)
{
  return GetNodeType (node) == "basestation";
}

bool IsServer (Ptr<Node> node)
{
  return GetNodeType (node) == "server";
}

bool IsRouter (Ptr<Node> node)
{
  std::string nodeType = GetNodeType (node);
  return nodeType == "gatewayRouter" or nodeType == "backboneRouter";
}

}//namespace
