/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
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
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "ron-helper.h"
#include "ron-server.h"
#include "ron-client.h"

#include "ns3/uinteger.h"
#include "ns3/names.h"
#include "ns3/log.h"

#include <iterator>

NS_LOG_COMPONENT_DEFINE ("RonClientServerHelper");

namespace ns3 {

RonServerHelper::RonServerHelper (uint16_t port)
{
  m_factory.SetTypeId (RonServer::GetTypeId ());
  SetAttribute ("Port", UintegerValue (port));
}

void 
RonServerHelper::SetAttribute (
  std::string name, 
  const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
RonServerHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
RonServerHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
RonServerHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
RonServerHelper::InstallPriv (Ptr<Node> node) const
{
  NS_LOG_FUNCTION_NOARGS ();

  Ptr<Application> app = m_factory.Create<RonServer> ();
  node->AddApplication (app);
  app->SetNode (node);

  return app;
}

  //////********************************************//////////////
  //////********************************************//////////////
  //////******************  CLIENT  ****************//////////////
  //////********************************************//////////////
  //////********************************************//////////////

RonClientHelper::RonClientHelper (uint16_t port)
{
  m_factory.SetTypeId (RonClient::GetTypeId ());
  SetAttribute ("Port", UintegerValue (port));
}

void 
RonClientHelper::SetAttribute (
  std::string name, 
  const AttributeValue &value)
{
  m_factory.Set (name, value);
}

void
RonClientHelper::SetFill (Ptr<Application> app, std::string fill)
{
  app->GetObject<RonClient>()->SetFill (fill);
}

void
RonClientHelper::SetFill (Ptr<Application> app, uint8_t fill, uint32_t dataLength)
{
  app->GetObject<RonClient>()->SetFill (fill, dataLength);
}

void
RonClientHelper::SetFill (Ptr<Application> app, uint8_t *fill, uint32_t fillLength, uint32_t dataLength)
{
  app->GetObject<RonClient>()->SetFill (fill, fillLength, dataLength);
}

ApplicationContainer
RonClientHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
RonClientHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
RonClientHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
RonClientHelper::InstallPriv (Ptr<Node> node) const
{
  NS_LOG_FUNCTION_NOARGS ();

  Ptr<Application> app = m_factory.Create<RonClient> ();

  node->AddApplication (app);
  app->SetNode (node);

  return app;
}

template<class ForwardIter>
void
RonClientHelper::AddPeers (ForwardIter peersBegin, ForwardIter peersEnd, ApplicationContainer apps)
{
  while (peersBegin != peersEnd)
    {
      for (ApplicationContainer::Iterator app = apps.Begin (); app != apps.End (); app++)
        {
          Ptr<RonClient> rca = DynamicCast<RonClient> (*app);
          rca->AddPeer (*peersBegin);
        }
      peersBegin++;
    }
}

} // namespace ns3

/* working with packet headers
int main (int argc, char *argv[])
{
  // instantiate a header.
  RonHeader sourceHeader;
  sourceHeader.SetData (2);

  // instantiate a packet
  Ptr<Packet> p = Create<Packet> ();

  // and store my header into the packet.
  p->AddHeader (sourceHeader);

  // print the content of my packet on the standard output.
  p->Print (std::cout);
  std::cout << std::endl;

  // you can now remove the header from the packet:
  RonHeader destinationHeader;
  p->RemoveHeader (destinationHeader);

  // and check that the destination and source
  // headers contain the same values.
  NS_ASSERT (sourceHeader.GetData () == destinationHeader.GetData ());

  return 0;
}
*/
