////////////////////////////////////////////////////////////////////////////////
//////////////////////    Traced source callbacks   ////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include <sstream>

/* REMOVE THESE INCLUDES WHEN MOVED OUT OF SCRATCH DIRECTORY!!!!!!!!!!! */
#include "ron-header.h"
#include "ron-helper.h"
#include "ron-client.h"
#include "ron-server.h"

#ifndef RON_TRACE_FUNCTIONS_H
#define RON_TRACE_FUNCTIONS_H

namespace ns3 {

  //class RonTraceFunctions
  //{
void AckReceived (Ptr<OutputStreamWrapper> stream, Ptr<const Packet> p, uint32_t nodeId);
void PacketForwarded (Ptr<OutputStreamWrapper> stream, Ptr<const Packet> p, uint32_t nodeId);
void PacketSent (Ptr<OutputStreamWrapper> stream, Ptr<const Packet> p, uint32_t nodeId);
  
  //}; //class

} //namespace ns3
#endif //RON_TRACE_FUNCTIONS_H
