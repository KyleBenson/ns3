#include "ron-trace-functions.h"
#include "failure-helper-functions.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("RonTracers");

void AckReceived (Ptr<OutputStreamWrapper> stream, Ptr<const Packet> p, uint32_t nodeId)
{
  RonHeader head;
  p->PeekHeader (head);
  bool usedOverlay = head.IsForward ();
  
  std::stringstream s;
  s << "Node " << nodeId << " received " << (usedOverlay ? "indirect" : "direct") << " ACK at " << Simulator::Now ().GetSeconds ();

  NS_LOG_INFO (s.str ());
  *stream->GetStream () << s.str() << std::endl;
}


void PacketForwarded (Ptr<OutputStreamWrapper> stream, Ptr<const Packet> p, uint32_t nodeId) 
{
  RonHeader head;
  p->PeekHeader (head);
  
  std::stringstream s;
  s << "Node " << nodeId << " forwarded packet (hop=" << (int)head.GetHop () << ") at " << Simulator::Now ().GetSeconds ()
    << " from " << head.GetOrigin () << " to " << head.GetNextDest () << " and eventually " << head.GetFinalDest ();

  NS_LOG_INFO (s.str ());
  *stream->GetStream () << s.str() << std::endl;
}


void PacketSent (Ptr<OutputStreamWrapper> stream, Ptr<const Packet> p, uint32_t nodeId)
{
  RonHeader head;
  p->PeekHeader (head);
  bool usedOverlay = head.IsForward ();
  
  std::stringstream s;
  s << "Node " << nodeId << " sent " << (usedOverlay ? "indirect" : "direct")
    << " packet at " << Simulator::Now ().GetSeconds ();
  
  NS_LOG_INFO (s.str ());
  *stream->GetStream () << s.str() << std::endl;
}

void PacketReceivedAtServer (Ptr<OutputStreamWrapper> stream, Ptr<const Packet> p, uint32_t nodeId)
{
  RonHeader head;
  p->PeekHeader (head);
  bool usedOverlay = head.IsForward ();
  uint32_t originId = GetNodeByIp (head.GetOrigin ())->GetId ();
  
  std::stringstream s;
  s << "Server " << nodeId << " received " << (usedOverlay ? "indirect" : "direct")
    << " packet at " << Simulator::Now ().GetSeconds ()
    << " from node " << originId;
  
  NS_LOG_INFO (s.str ());
  *stream->GetStream () << s.str() << std::endl;
}

} //ns3 namespace
