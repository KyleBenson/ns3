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

#ifndef RON_SERVER_H
#define RON_SERVER_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/address.h"
#include "ns3/traced-callback.h"
#include "ns3/output-stream-wrapper.h"

namespace ns3 {

class Socket;
class Packet;

/**
 * \ingroup applications 
 * \defgroup ron ron
 */

/**
 * \ingroup ron
 * \brief A Udp Echo server
 *
 * Every packet received is sent back.
 */
class RonServer : public Application 
{
public:
  static TypeId GetTypeId (void);
  RonServer ();
  virtual ~RonServer ();

  void ConnectTraces (Ptr<OutputStreamWrapper> traceOutputStream);

protected:
  virtual void DoDispose (void);

private:

  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void HandleRead (Ptr<Socket> socket);

  // callbacks
  TracedCallback<Ptr<const Packet>, uint32_t > m_recvTrace;
  CallbackBase m_recvcb;

  uint16_t m_port;
  Ptr<Socket> m_socket;
  Address m_local;
};

} // namespace ns3

#endif /* RON_SERVER_H */

