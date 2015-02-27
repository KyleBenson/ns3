#include "failure-helper-functions.h"

#ifndef nslog
#define nslog(x) NS_LOG_UNCOND(x);
#endif

namespace ns3 {

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

}//namespace
