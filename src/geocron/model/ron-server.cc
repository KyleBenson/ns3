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
#include "ns3/uinteger.h"

#include "ron-trace-functions.h"
#include "ron-header.h"
#include "ron-server.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("RonServerApplication");
NS_OBJECT_ENSURE_REGISTERED (RonServer);

TypeId
RonServer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RonServer")
    .SetParent<Application> ()
    .AddConstructor<RonServer> ()
    .AddAttribute ("Port", "Port on which we listen for incoming packets.",
                   UintegerValue (9),
                   MakeUintegerAccessor (&RonServer::m_port),
                   MakeUintegerChecker<uint16_t> ())
    .AddTraceSource ("Receive", "A new packet is received by the server",
                     MakeTraceSourceAccessor (&RonServer::m_recvTrace))
  ;
  return tid;
}

RonServer::RonServer ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

RonServer::~RonServer()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_socket = 0;
}

void
RonServer::DoDispose (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  StopApplication ();
  Application::DoDispose ();
}

void 
RonServer::StartApplication (void)
{
  NS_LOG_FUNCTION_NOARGS ();

  if (m_socket == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tid);
      InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), m_port);
      m_socket->Bind (local);
      if (addressUtils::IsMulticast (m_local))
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
        }
    }

  m_socket->SetRecvCallback (MakeCallback (&RonServer::HandleRead, this));
}

void 
RonServer::StopApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();

  if (m_socket != 0) 
    {
      m_socket->Close ();
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    }
}

void 
RonServer::HandleRead (Ptr<Socket> socket)
{
  Ptr<Packet> packet;
  Address from;
  while (packet = socket->RecvFrom (from))
    {
      if (InetSocketAddress::IsMatchingType (from))
        {
          RonHeader head;
          packet->RemoveHeader (head);

          // If this is the first hop on the route, we need to set the source
          // since NS3 doesn't give us a convenient way to access which interface
          // the original packet was sent out on.
          // NOTE: this has been temporarily resolved by calling head->SetOrigin (m_address) in RonClient.cc
          //if (head.GetHop () == 0 and not head.IsForward ())
            //head.SetOrigin (InetSocketAddress::ConvertFrom (from).GetIpv4 ());

          NS_LOG_INFO (head);

          Ptr<Packet> packetWithHeader = packet->Copy ();
          packetWithHeader->AddHeader (head);
          m_recvTrace (packetWithHeader, GetNode ()->GetId ());

          head.ReversePath ();

          packet->AddHeader (head);

          packet->RemoveAllPacketTags ();
          packet->RemoveAllByteTags ();

          NS_LOG_INFO ("ACKing " << (head.IsForward () ? "indirect" : "direct") << " packet from " <<
                       InetSocketAddress::ConvertFrom (from).GetIpv4 () <<
                       " on behalf of " << head.GetFinalDest ());

          // NOTE: make sure we use the destination from the RonHeader as otherwise
          // we may not have seen this IP address before and we'll cause a crash
          // when trying to extract the RonPath on the other side
          socket->SendTo (packet, 0, head.GetFinalDest ());
        }
    }
}

void
RonServer::ConnectTraces (Ptr<OutputStreamWrapper> traceOutputStream)
{
  this->TraceDisconnectWithoutContext ("Receive", m_recvcb);
  m_recvcb = MakeBoundCallback (&PacketReceivedAtServer, traceOutputStream);
  this->TraceConnectWithoutContext ("Receive", m_recvcb);
}

} // Namespace ns3
