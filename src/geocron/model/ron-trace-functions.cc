#include "ron-trace-functions.h"
#include "failure-helper-functions.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("RonTracers");

void AckReceived (Ptr<OutputStreamWrapper> stream, Ptr<const Packet> p, uint32_t nodeId)
{
  RonHeader head;
  p->PeekHeader (head);
  bool usedOverlay = head.IsForward ();
  // NOTE: we want to get the node that ORIGINALLY sent this packet,
  // rather than the one that bounced it back, in this case
  Ptr<Node> destNode = GetNodeByIp (head.GetFinalDest ());
  
  std::stringstream s;
  s << GetNodeType (destNode) << " " << nodeId << " received " << (usedOverlay ? "indirect" : "direct") << " ACK at " << Simulator::Now ().GetSeconds ();

  NS_LOG_INFO (s.str ());
  *stream->GetStream () << s.str() << std::endl;
}


void PacketForwarded (Ptr<OutputStreamWrapper> stream, Ptr<const Packet> p, uint32_t nodeId) 
{
  RonHeader head;
  p->PeekHeader (head);
  Ptr<Node> originNode = GetNodeByIp (head.GetOrigin ());
  
  std::stringstream s;
  s << GetNodeType (originNode) << " " << nodeId << " forwarded packet (hop=" << (int)head.GetHop () << ") at " << Simulator::Now ().GetSeconds ()
    << " from " << head.GetOrigin () << " to " << head.GetNextDest () << " and eventually " << head.GetFinalDest ();

  NS_LOG_INFO (s.str ());
  *stream->GetStream () << s.str() << std::endl;
}


void PacketSent (Ptr<OutputStreamWrapper> stream, Ptr<const Packet> p, uint32_t nodeId)
{
  RonHeader head;
  p->PeekHeader (head);
  bool usedOverlay = head.IsForward ();
  Ptr<Node> originNode = GetNodeByIp (head.GetOrigin ());
  
  std::stringstream s;
  s << GetNodeType (originNode) << " " << nodeId << " sent " << (usedOverlay ? "indirect" : "direct")
    << " packet at " << Simulator::Now ().GetSeconds ();
  
  NS_LOG_INFO (s.str ());
  *stream->GetStream () << s.str() << std::endl;
}

void PacketReceivedAtServer (Ptr<OutputStreamWrapper> stream, Ptr<const Packet> p, uint32_t nodeId)
{
  RonHeader head;
  p->PeekHeader (head);
  bool usedOverlay = head.IsForward ();
  Ptr<Node> originNode = GetNodeByIp (head.GetOrigin ());

  // NOTE: we probably need to use the actual NodeId rather than its
  // topology file-given name because the original trace analyzer uses
  // a dict indexed by NodeId for tracking which packets were received.
  
  std::stringstream s;
  s << "Server " << nodeId << " received " << (usedOverlay ? "indirect" : "direct")
    << " packet at " << Simulator::Now ().GetSeconds ()
    << " from " << GetNodeType (originNode) << " " << originNode->GetId ();
  
  NS_LOG_INFO (s.str ());
  *stream->GetStream () << s.str() << std::endl;
}

} //ns3 namespace
