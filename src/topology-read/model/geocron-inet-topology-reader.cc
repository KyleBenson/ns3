/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 Universita' di Firenze, Italy
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
 * Author: Tommaso Pecorella (tommaso.pecorella@unifi.it)
 * Author: Valerio Sartini (Valesar@gmail.com)
 */

#include <fstream>
#include <cstdlib>
#include <sstream>
#include <string>
#include "ns3/mobility-module.h"

#include "ns3/log.h"
#include "ns3/names.h"

#include <boost/lexical_cast.hpp>

#include "geocron-inet-topology-reader.h"


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("GeocronInetTopologyReader");

NS_OBJECT_ENSURE_REGISTERED (GeocronInetTopologyReader);

TypeId GeocronInetTopologyReader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::GeocronInetTopologyReader")
    .SetParent<Object> ()
  ;
  return tid;
}

GeocronInetTopologyReader::GeocronInetTopologyReader ()
{
  NS_LOG_FUNCTION (this);
}

GeocronInetTopologyReader::~GeocronInetTopologyReader ()
{
  NS_LOG_FUNCTION (this);
}

NodeContainer
GeocronInetTopologyReader::Read (void)
{
  std::ifstream topgen;
  topgen.open (GetFileName ().c_str ());
  // Stores nodes, indexed by name, for later access when adding links
  std::map<std::string, Ptr<Node> > nodeMap;
  NodeContainer nodes;

  if ( !topgen.is_open () )
    {
      NS_LOG_WARN ("GeocronInet topology file object is not open, check file name and permissions");
      return nodes;
    }

  std::string id;
  std::string xpos;
  std::string ypos;
  std::string from;
  std::string to;
  std::string linkAttr;

  int linksNumber = 0;
  int nodesNumber = 0;

  int totnode = 0;
  int totlink = 0;

  std::istringstream lineBuffer;
  std::string line;

  getline (topgen,line);
  lineBuffer.str (line);

  lineBuffer >> totnode;
  lineBuffer >> totlink;
  NS_LOG_INFO ("GeocronInet topology should have " << totnode << " nodes and " << totlink << " links");

  for (int i = 0; i < totnode && !topgen.eof (); i++)
    {
      getline (topgen,line);
      lineBuffer.clear ();
      lineBuffer.str (line);

      lineBuffer >> id;
      lineBuffer >> xpos;
      lineBuffer >> ypos;

      Ptr<Node> node = CreateObject<Node> ();
      nodes.Add (node);
      nodeMap[id] = node;

      // add position information via a mobility model
      MobilityHelper mobility;
      // put the location for this node in a list for the helper
      Ptr<ListPositionAllocator> positionAllocator = CreateObject<ListPositionAllocator> ();

      Vector position = Vector (boost::lexical_cast<double> (xpos),
          boost::lexical_cast<double> (ypos), 0);
      positionAllocator->Add (position);

      mobility.SetPositionAllocator (positionAllocator);
      mobility.Install (node);

      // Now we place the node into the appropriate bin according to it
      // type, which is derived from the first 3 digits of the 
      // node's name by the following:
      // 100 - seismic sensor
      // 101 - water sensor
      // 103 - basestation
      // 110 - gateway router
      // 111 - backbone router
      // 200 - server
      //
      // We also use the ns-3 object naming system to name the node
      // according to its type so we can later look up this information
      // easily when setting up the experiment or running the simulation.
      //
      std::string nodeType = id.substr (0, 3);
      // We use the type and name combined, rather than nested,
      // because the naming system expects an actual object for each
      // node in the hierarchy.  Since there is no named object for the buckets,
      // e.g. a waterSensor and a seismicSensor, each node under
      // them with the same "name" appears the same and crashes ns3.
      std::string nodeName = id.substr (3);

      if (nodeType == "100") {
        seismicSensors.Add (node);
        nodeType = "seismicSensor";
      }
      else if (nodeType == "101") {
        waterSensors.Add (node);
        nodeType = "waterSensor";
      }
      else if (nodeType == "103") {
        basestations.Add (node);
        nodeType = "basestation";
      }
      else if (nodeType == "110") {
        gatewayRouters.Add (node);
        nodeType = "gatewayRouter";
      }
      else if (nodeType == "111") {
        backboneRouters.Add (node);
        nodeType = "backboneRouter";
      }
      else if (nodeType == "200") {
        servers.Add (node);
        nodeType = "server";
      }
      else
      {
        NS_LOG_WARN ("Unknown node type " << nodeType);
        nodeType = "unknown";
      }
      Names::Add (nodeType + nodeName, node);

      NS_LOG_INFO ( "Node #" << nodesNumber << " of type " << nodeType << " and name: " << nodeName );
      nodesNumber++;
    }

  for (int i = 0; i < totlink && !topgen.eof (); i++)
    {
      getline (topgen,line);
      lineBuffer.clear ();
      lineBuffer.str (line);

      lineBuffer >> from;
      lineBuffer >> to;
      lineBuffer >> linkAttr;

      if ( (!from.empty ()) && (!to.empty ()) )
        {
          NS_LOG_INFO ( "Link " << linksNumber << " from: " << from << " to: " << to);

          Link link ( nodeMap[from], from, nodeMap[to], to );
          if ( !linkAttr.empty () )
            {
              NS_LOG_INFO ( "Link " << linksNumber << " weight: " << linkAttr);
              link.SetAttribute ("Weight", linkAttr);
            }
          AddLink (link);
          //TODO: set link attributes of BW and latency?

          linksNumber++;
        }
    }

  NS_LOG_INFO ("GeocronInet topology created with " << nodesNumber << " nodes and " << linksNumber << " links");
  topgen.close ();

  return nodes;
}

} /* namespace ns3 */
