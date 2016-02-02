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
 * Author: Valerio Sartini (valesar@gmail.com)
 */

#ifndef GEOCRON_INET_TOPOLOGY_READER_H
#define GEOCRON_INET_TOPOLOGY_READER_H

#include "ns3/nstime.h"
#include "topology-reader.h"

namespace ns3 {


// ------------------------------------------------------------
// --------------------------------------------
/**
 * \ingroup topology
 *
 * \brief Topology file reader (Inet-format type).
 *
 * This class takes an input file in Inet format and extracts all
 * the informations needed to build the topology
 * (i.e.number of nodes, links and links structure).
 * It have been tested with Inet 3.0
 * http://topology.eecs.umich.edu/inet/
 *
 * It might set a link attribute named "Weight", corresponding to
 * the euclidean distance between two nodes, the nodes being randomly positioned.
 *
 * This extension for the UCI GeoCRON project actually reads the node info
 * in order to set their geographic location using a StaticPosition MobilityModel.
 * It also adds functionality for storing different node types in several different
 * containers representing the following network components: seismic sensors,
 * water sensors, wireless basestations, gateway routers, backbone routers,
 * and servers.  Each of these types is determined by the node name (ID),
 * which includes an encoding in the first two digits to represent the type.
 */
class GeocronInetTopologyReader : public TopologyReader
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  GeocronInetTopologyReader ();
  virtual ~GeocronInetTopologyReader ();

  /**
   * \brief Main topology reading function.
   *
   * This method opens an input stream and reads the Inet-format file.
   * From the first line it takes the total number of nodes and links.
   * Then it reads a number of rows equals to the total nodes and sets
   * their Location using the geographical information provided in the line.
   * Then reads until the end of the file (total links number rows) and saves
   * the structure of every single link in the topology.
   *
   * \return the container of the nodes created (or empty container if there was an error)
   */
  virtual NodeContainer Read (void);

  // These NodeContainers store special types of nodes into appropriate bins
  NodeContainer waterSensors;
  NodeContainer seismicSensors;
  NodeContainer basestations;
  NodeContainer gatewayRouters;
  NodeContainer backboneRouters;
  NodeContainer servers;

private:
  /**
   * \brief Copy constructor
   *
   * Defined and unimplemented to avoid misuse
   */
  GeocronInetTopologyReader (const GeocronInetTopologyReader&);
  /**
   * \brief Copy constructor
   *
   * Defined and unimplemented to avoid misuse
   * \returns
   */
  GeocronInetTopologyReader& operator= (const GeocronInetTopologyReader&);

  // end class GeocronInetTopologyReader
};

// end namespace ns3
};


#endif /* GEOCRON_INET_TOPOLOGY_READER_H */
