#include "failure-helper-functions.h"

#ifndef nslog
#define nslog(x) NS_LOG_UNCOND(x);
#endif

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("FailureHelperFunctions");

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

Ipv4Address GetNodeAddress(Ptr<Node> node)
{
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

}//namespace
