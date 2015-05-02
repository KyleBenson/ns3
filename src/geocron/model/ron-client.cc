/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright 2007 University of Washington
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/address-utils.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/socket.h"
#include "ns3/udp-socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/uinteger.h"
#include "ns3/ipv4.h"
#include "ns3/object.h" //for schedulign Initialize() method

#include "ron-trace-functions.h"
#include "ron-client.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("RonClientApplication");
NS_OBJECT_ENSURE_REGISTERED (RonClient);


TypeId
RonClient::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RonClient")
    .SetParent<Application> ()
    .AddConstructor<RonClient> ()
    .AddAttribute ("Port", "Port on which we listen for incoming packets from overlay nodes.",
                   UintegerValue (9),
                   MakeUintegerAccessor (&RonClient::m_port),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("Timeout", "Time to wait for a reply before trying to resend or send along an overlay node.",
                   TimeValue (Seconds (3)),
                   MakeTimeAccessor (&RonClient::m_timeout),
                   MakeTimeChecker ())
    .AddAttribute ("MaxPackets", 
                   "The maximum number of packets the application will send",
                   UintegerValue (100),
                   MakeUintegerAccessor (&RonClient::m_count),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Interval", 
                   "The time to wait between packets",
                   TimeValue (Seconds (1.0)),
                   MakeTimeAccessor (&RonClient::m_interval),
                   MakeTimeChecker ())
    .AddAttribute ("PacketSize", "Size of data in outbound packets",
                   UintegerValue (100),
                   MakeUintegerAccessor (&RonClient::SetDataSize,
                                         &RonClient::GetDataSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("MultipathFanout", "Number of multiple paths to send a message out on when using the overlay",
                   UintegerValue (1),
                   MakeUintegerAccessor (&RonClient::m_multipathFanout),
                   MakeUintegerChecker<uint32_t> ())
    .AddTraceSource ("Send", "A new packet is created and is sent to the server or an overlay node",
                     MakeTraceSourceAccessor (&RonClient::m_sendTrace))
    .AddTraceSource ("Ack", "An ACK packet originating from the server is received",
                     MakeTraceSourceAccessor (&RonClient::m_ackTrace))
    .AddTraceSource ("Forward", "A packet originating from an overlay node is received and forwarded to the server",
                     MakeTraceSourceAccessor (&RonClient::m_forwardTrace))
  ;
  return tid;
}

RonClient::RonClient ()
{
  NS_LOG_FUNCTION_NOARGS ();
  SetDefaults ();

  m_data = NULL;
  m_peers = Create<RonPeerTable> ();
  m_address = Ipv4Address ((uint32_t)0);
}

void
RonClient::SetDefaults ()
{
  m_sent = 0;
  m_socket = 0;
  m_nextPeer = 0;
  m_count = 0;

  //need to call Clear to remove circular references and allow smart pointers to be Unref'ed
  if (m_heuristic != (RonPathHeuristic*)NULL)
    {
      m_heuristic->Clear ();
    }

  m_heuristic = NULL;
}


void
RonClient::DoReset ()
{
  NS_LOG_FUNCTION_NOARGS ();

  SetDefaults ();

  // Reschedule start so that StartApplication gets rescheduled
  // WARNING: was ScheduleWithContext (GetId (), args...) for in a node
  Simulator::Schedule (Seconds (0.0), 
                       &Application::Initialize, this);

  //Ipv4Address m_servAddress; //HANDLE SETTING!!
  CancelEvents ();
  m_outstandingSeqs.clear ();
}

RonClient::~RonClient()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_socket = 0;

  delete [] m_data;

  m_data = 0;
  m_dataSize = 0;
}

void
RonClient::DoDispose (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  StopApplication ();
  Application::DoDispose ();
}

void 
RonClient::StartApplication (void)
{
  NS_LOG_FUNCTION_NOARGS ();

  if (m_startTime >= m_stopTime)
    {
      NS_LOG_LOGIC ("Cancelling application (start > stop)");
      //DoDispose ();
      return;
    }

  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");

  if (m_socket == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tid);
      InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), m_port);
      m_socket->Bind (local);

      NS_LOG_LOGIC ("Socket bound");

      /*if (addressUtils::IsMulticast (m_local))
        {
          Ptr<UdpSocket> udpSocket = DynamicCast<UdpSocket> (m_socket);
          if (udpSocket)
            {
              // equivalent to setsockopt (MCAST_JOIN_GROUP)
              udpSocket->MulticastJoinGroup (0, m_local);
            }
          else
            {
              NS_FATAL_ERROR ("Error: joining multicast on a non-UDP socket");
            }
            }*/
    }

  // Use the address of the first non-loopback device on the node for our address
  //TODO: properly bootstrap from the server
  if (m_address.Get () == 0)
    {
      m_address = GetNode ()->GetObject<Ipv4> ()->GetAddress (1,0).GetLocal ();
    }

  m_socket->SetRecvCallback (MakeCallback (&RonClient::HandleRead, this));

  if (m_sent < m_count)
    ScheduleTransmit (Seconds (0.));
}

void 
RonClient::StopApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();

  if (m_socket != 0) 
    {
      m_socket->Close ();
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
      m_socket = 0;
    }

  CancelEvents ();
  Reset ();
}

void
RonClient::CancelEvents ()
{
  NS_LOG_FUNCTION_NOARGS ();

  for (std::list<EventId>::iterator itr = m_events.begin ();
       itr != m_events.end (); itr++)
    Simulator::Cancel (*itr);

  m_events.clear ();
}

void 
RonClient::SetFill (std::string fill)
{
  NS_LOG_FUNCTION (fill);

  uint32_t dataSize = fill.size () + 1;

  if (dataSize != m_dataSize)
    {
      if (m_data)
        delete [] m_data;
      m_data = new uint8_t [dataSize];
      m_dataSize = dataSize;
    }

  memcpy (m_data, fill.c_str (), dataSize);

  //
  // Overwrite packet size attribute.
  //
  m_size = dataSize;
}

void 
RonClient::SetFill (uint8_t fill, uint32_t dataSize)
{
  if (dataSize != m_dataSize)
    {
      if (m_data)
        delete [] m_data;
      m_data = new uint8_t [dataSize];
      m_dataSize = dataSize;
    }

  memset (m_data, fill, dataSize);

  //
  // Overwrite packet size attribute.
  //
  m_size = dataSize;
}

void 
RonClient::SetFill (uint8_t *fill, uint32_t fillSize, uint32_t dataSize)
{
  if (dataSize != m_dataSize)
    {
      if (m_data)
        delete [] m_data;
      m_data = new uint8_t [dataSize];
      m_dataSize = dataSize;
    }

  if (fillSize >= dataSize)
    {
      memcpy (m_data, fill, dataSize);
      return;
    }

  //
  // Do all but the final fill.
  //
  uint32_t filled = 0;
  while (filled + fillSize < dataSize)
    {
      memcpy (&m_data[filled], fill, fillSize);
      filled += fillSize;
    }

  //
  // Last fill may be partial
  //
  memcpy (&m_data[filled], fill, dataSize - filled);

  //
  // Overwrite packet size attribute.
  //
  m_size = dataSize;
}

void 
RonClient::SetDataSize (uint32_t dataSize)
{
  NS_LOG_FUNCTION (dataSize);

  //
  // If the client is setting the echo packet data size this way, we infer
  // that she doesn't care about the contents of the packet at all, so 
  // neither will we.
  //
  if (m_data)
    delete [] m_data;
  m_data = 0;
  m_dataSize = 0;
  m_size = dataSize;
}

uint32_t 
RonClient::GetDataSize (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_size;
}

void 
RonClient::ScheduleTransmit (Time dt, bool viaOverlay /*= false*/)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_events.push_front (Simulator::Schedule (dt, &RonClient::Send, this, viaOverlay));
}

void 
RonClient::Send (bool viaOverlay)
{
  NS_LOG_FUNCTION_NOARGS ();

  NS_ASSERT_MSG (m_heuristic, "m_heuristic is NULL! Can't run a RonClient without a heuristic!");

  NS_LOG_LOGIC ("Sending " << (viaOverlay ? "indirect" : "direct") << " packet.");

  Ptr<Packet> p1;
  if (m_dataSize)
    {
      //
      // If m_dataSize is non-zero, we have a data buffer of the same size that we
      // are expected to copy and send.  This state of affairs is created if one of
      // the Fill functions is called.  In this case, m_size must have been set
      // to agree with m_dataSize
      //
      NS_ASSERT_MSG (m_dataSize == m_size, "RonClient::Send(): m_size and m_dataSize inconsistent");
      NS_ASSERT_MSG (m_data, "RonClient::Send(): m_dataSize but no m_data");
      p1 = Create<Packet> (m_data, m_dataSize);
    }
  else
    {
      //
      // If m_dataSize is zero, the client has indicated that she doesn't care 
      // about the data itself either by specifying the data size by setting
      // the corresponding atribute or by not calling a SetFill function.  In 
      // this case, we don't worry about it either.  But we do allow m_size
      // to have a value different from the (zero) m_dataSize.
      //
      p1 = Create<Packet> (m_size);
    }

  // send a message to each of the available servers
  for (RonPeerTable::Iterator serverItr = m_serverPeers->Begin ();
      serverItr != m_serverPeers->End (); serverItr++)
  {
    Ptr<RonPeerEntry> serverPeer = *serverItr;

    // Here we'll gather all the overlay peers we will use in multipath messaging
    RonPathHeuristic::RonPathContainer overlayPeerChoices;

    // If forwarding thru overlay, use heuristic to pick a peer from those available
    if (viaOverlay)
    {
      try
      {
        overlayPeerChoices = m_heuristic->GetBestMultiPath (Create<PeerDestination> (serverPeer), m_multipathFanout);
        //TODO: FIX THIS BUG!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        //for whatever reason, it doesn't work in the CheckTimeout function... something wrong with the way we reverse the header and send it back?  looks like it's because the server pops the header, reverses it, and puts it in the packet for the way back.  this will erase a lot of information about the peers so we should really just get around to actually to storing the entire peer entry (at least const parts of them) in the header path...
        //basically I think we need to build a RonPath that contains the actual peer entries so that they'll compare to be the same; could probably test this by adding the header, popping it off, and comparing the extracted path with the original
        //for now, only use the timeout mechanism when we're allowing
        //multiple contact attempts as we don't currently support
        //that along with multipath
        if (m_count > 1)
          m_heuristic->NotifyTimeout (overlayPeerChoices.front (), Simulator::Now ());
      }
      catch (RonPathHeuristic::NoValidPeerException& e)
      {
        NS_LOG_LOGIC ("Node " << GetNode ()->GetId () << " has no more overlay peers to choose.");
        //NS_LOG_DEBUG (e.what ());
        CancelEvents ();
        return;
      }
    }

    // We need to send a packet for each overlay path as well as along
    // the regular path to the destination, but the latter only if we
    // have not sent such a message before
    uint32_t totalPacketsToSend = overlayPeerChoices.size ();
    if (!m_sent)
      totalPacketsToSend++;

    for (uint32_t i = 0; i < totalPacketsToSend; i++)
    {
      Ptr<RonHeader> head = Create<RonHeader> ();
      head->SetOrigin (m_address);
      Ptr<Packet> p = p1->Copy ();

      // Do the multipath overlay peers first, then the regular path on last iteration
      if (i < overlayPeerChoices.size ())
        head->SetPath (overlayPeerChoices[i]);
      else
        head->SetDestination (serverPeer->address);

      head->SetSeq (m_sent);
      p->AddHeader (*head);

      // call to the trace sinks before the packet is actually sent,
      // so that tags added to the packet can be sent as well
      m_sendTrace (p, GetNode ()->GetId ());
      m_socket->SendTo (p, 0, InetSocketAddress(head->GetNextDest (), m_port));
      ScheduleTimeout (head);
      NS_LOG_DEBUG ("Sent " << m_sent << "th packet out of " << m_count << " max");
      m_sent++;
    }
  }
}

void 
RonClient::HandleRead (Ptr<Socket> socket)
{
  Ptr<Packet> packet;
  Address from;

  while (packet = socket->RecvFrom (from))
    {
      NS_LOG_LOGIC ("Reading packet from socket.");

      if (InetSocketAddress::IsMatchingType (from))
        {
          Ipv4Address source = InetSocketAddress::ConvertFrom (from).GetIpv4 ();

          RonHeader head;
          packet->PeekHeader (head);

          // If the packet is for us, process the ACK
          if (head.GetFinalDest () == head.GetNextDest ())
            {
              ProcessAck (packet, source);
            }

          // Else forward it
          else
            {
              ForwardPacket (packet, source);
              //TODO: ACK the partial path
              //TODO: update last-contacted info
              //TODO: possibly look at other branches
            }
        }
    }
}

void
RonClient::ForwardPacket (Ptr<Packet> packet, Ipv4Address source)
{
  NS_LOG_LOGIC ("Forwarding packet");

  packet->RemoveAllPacketTags ();
  packet->RemoveAllByteTags ();

  // Edit the packet's current RON header and get next hop
  RonHeader head;
  packet->RemoveHeader (head);

  // If this is the first hop on the route, we need to set the source
  // since NS3 doesn't give us a convenient way to access which interface
  // the original packet was sent out on.
  if (head.GetHop () == 0)
    head.SetOrigin (source);

  head.IncrHops ();
  Ipv4Address destination = head.GetNextDest ();
  packet->AddHeader (head);

  m_forwardTrace (packet, GetNode ()-> GetId ());
  m_socket->SendTo (packet, 0, InetSocketAddress(destination, m_port));
  //TODO: setup timeouts/ACKs for forwards
}

void
RonClient::ProcessAck (Ptr<Packet> packet, Ipv4Address source)
{
  NS_LOG_LOGIC ("ACK received");

  RonHeader head;
  packet->PeekHeader (head);
  uint32_t seq = head.GetSeq ();
  
  m_ackTrace (packet, GetNode ()-> GetId ());
  m_outstandingSeqs.erase (seq);
  //TODO: handle an ack from an old seq number

  Time time = Simulator::Now ();
  head.ReversePath ();//currently in order from dest
  Ptr<RonPath> path = head.GetPath ();

  NS_ASSERT_MSG (path->GetN () > 0, "got 0 length path from Header in ProcessAck");

  //need to set destination's address explicitly as we currently don't have everything in the master table
  //TODO: fix this
  (*path->GetDestination ()->Begin ())->address = source;

  m_heuristic->NotifyAck (path, time);

  CancelEvents ();
  //TODO: store path? send more data?
}

void
RonClient::ScheduleTimeout (Ptr<RonHeader> head)
{
  m_events.push_front (Simulator::Schedule (m_timeout, &RonClient::CheckTimeout, this, head));
  m_outstandingSeqs.insert (head->GetSeq ());
}

void
RonClient::AddPeer (Ptr<Node> node)
{
  //if (addr != m_address)
  m_peers->AddPeer (node);
}



void
RonClient::SetPeerTable (Ptr<RonPeerTable> peers)
{
  m_peers = peers;
}


void
RonClient::SetServerPeerTable (Ptr<RonPeerTable> peers)
{
  m_serverPeers = peers;
}


void
RonClient::AddServerPeer (Ptr<RonPeerEntry> peer)
{
  m_serverPeers->AddPeer (peer);
  //m_servAddress = peer->address;
}


void
RonClient::SetHeuristic (Ptr<RonPathHeuristic> heuristic)
{
  m_heuristic = heuristic;
  heuristic->SetSourcePeer (Create<RonPeerEntry> (GetNode ()));
}


Ipv4Address
RonClient::GetAddress () const
{
  return m_address;
}

void
RonClient::CheckTimeout (Ptr<RonHeader> head)
{
  uint32_t seq = head->GetSeq ();
  std::set<uint32_t>::iterator itr = m_outstandingSeqs.find (seq);

  // If it's timed out, we should try a different path to the server
  if (itr != m_outstandingSeqs.end ())
    {
      NS_LOG_LOGIC ("Packet with seq# " << seq << " timed out.");
      m_outstandingSeqs.erase (itr);

      Time time = Simulator::Now ();
      Ptr<RonPath> path = head->GetPath ();

      //need to set destination's address explicitly as we currently don't have everything in the master table
      //TODO: fix this
      (*path->GetDestination ()->Begin ())->address = m_serverPeers->GetPeerByAddress (head->GetFinalDest ())->address;

      m_heuristic->NotifyTimeout (path, time);
      
      // try again?
      //TODO: how to handle multi-path messages when m_count > 1??
      //maybe a CheckMultipathTimeout that looks to see if ANY of the
      //RonHeaders in a list of them have been ACKed?
      if (m_sent < m_count)
        ScheduleTransmit (Seconds (0.0), true);
    }
}


void
RonClient::ConnectTraces (Ptr<OutputStreamWrapper> traceOutputStream)
{
  this->TraceDisconnectWithoutContext ("Ack", m_ackcb);
  this->TraceDisconnectWithoutContext ("Send", m_sendcb);
  this->TraceDisconnectWithoutContext ("Forward", m_forwardcb);
  
  m_ackcb = MakeBoundCallback (&AckReceived, traceOutputStream);
  m_sendcb = MakeBoundCallback (&PacketSent, traceOutputStream);
  m_forwardcb = MakeBoundCallback (&PacketForwarded, traceOutputStream);

  this->TraceConnectWithoutContext ("Ack", m_ackcb);
  this->TraceConnectWithoutContext ("Forward", m_forwardcb);
  this->TraceConnectWithoutContext ("Send", m_sendcb);
}
  

} // Namespace ns3
