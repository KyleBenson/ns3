/** Functions for helping fail nodes/links during ns-3 simulations. */

#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"

#ifndef FAILURE_HELPER_FUNCTIONS_H
#define FAILURE_HELPER_FUNCTIONS_H

//global helper functions must be in ns3 namespace
namespace ns3 {
  void FailIpv4 (Ptr<Ipv4> ipv4, uint32_t iface);
  void UnfailIpv4 (Ptr<Ipv4> ipv4, uint32_t iface);
  void FailNode (Ptr<Node> node);
  void UnfailNode (Ptr<Node> node, Time appStopTime);

  Ipv4Address GetNodeAddress(Ptr<Node> node);
  uint32_t GetNodeDegree(Ptr<Node> node);
} //namespace ns3


#endif //FAILURE_HELPER_FUNCTIONS_H
