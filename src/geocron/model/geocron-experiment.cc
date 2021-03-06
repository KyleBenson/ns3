/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

/** An experiment of network failure scenarios during disasters.  Randomly fails the given links/nodes
    within the given region, running a RonClient on each node, who contacts the chosen RonServer
    within the disaster region.  Can run this class multiple times, once for each set of parameters.  **/

#include "geocron-experiment.h"
#include "ron-trace-functions.h"
#include "failure-helper-functions.h"
#include "random-ron-path-heuristic.h"

#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/heap/priority_queue.hpp>
 
#include <sstream>
#include <iomanip>
#include <fstream>
#include <ctime>
#include <unistd.h>
#include <boost/functional/hash.hpp>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("GeocronExperiment");
NS_OBJECT_ENSURE_REGISTERED (GeocronExperiment);

TypeId
GeocronExperiment::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::GeocronExperiment")
    .SetParent<Object> ()
    .AddConstructor<GeocronExperiment> ()
    .AddAttribute ("TopologyType",
                   "Name of the topology generator to be used by this experiment.  Supports: rocketfuel, brite.",
                   //StringValue ("rocketfuel"),
                   StringValue ("brite"),
                   MakeStringAccessor (&GeocronExperiment::topologyType),
                   MakeStringChecker ())
  ;
  return tid;
}


GeocronExperiment::GeocronExperiment ()
{
  appStopTime = Time (Seconds (30.0));
  simulationLength = Seconds (10.0);
  overlayPeers = RonPeerTable::GetMaster ();

  maxNDevs = 1;
  //cmd.AddValue ("install_stubs", "If not 0, install RON client only on stub nodes (have <= specified links - 1 (for loopback dev))", maxNDevs);

  currLocation = "";
  currFprob = 0.0;
  currRun = 0;
  contactAttempts = 10;
  traceFile = "";
  nruns = 1;
  nServerChoices = 10;

  regionHelper = NULL;
}


Ptr<RegionHelper> GeocronExperiment::GetRegionHelper ()
{
  if (regionHelper == NULL)
    {
      // set regionHelper by the type specified
      if (topologyType == "rocketfuel")
        regionHelper = CreateObject<RocketfuelRegionHelper> ();
      else if (topologyType == "brite") {
        regionHelper = CreateObject<BriteRegionHelper> ();
      }
      else {
        NS_ASSERT_MSG(false, "Invalid topology type: " << topologyType);
      }
    }
  return regionHelper;
}

void
GeocronExperiment::SetTimeout (Time newTimeout)
{
  timeout = newTimeout;
}


void
GeocronExperiment::NextHeuristic ()
{
  //heuristicIdx++;
}

void
GeocronExperiment::NextDisasterLocation ()
{
  return;
  //locationIdx++;
}


void
GeocronExperiment::NextFailureProbability ()
{
  return;
  //fpropIdx++;
}


////////////////////////////////////////////////////////////////////////////////
///////////////////   File/Topology Helper Functions     ///////////////////////
////////////////////////////////////////////////////////////////////////////////


bool
GeocronExperiment::IsDisasterNode (Ptr<Node> node)
{
  return disasterNodes[currLocation].count ((node)->GetId ());
}


bool
GeocronExperiment::IsOverlayNode (Ptr<Node> node)
{
  // first part handles if we didn't specify a max degree for overlay nodes, in which case ALL nodes are overlays...
  return !maxNDevs or GetNodeDegree (node) <= maxNDevs;
}


/////////////////////////////////////////////////////////////////////
////////////////////    BRITE Topology //////////////////////////////
/////////////////////////////////////////////////////////////////////


void GeocronExperiment::ReadBriteTopology (std::string topologyFile)
{
  this->topologyFile = topologyFile;
  
  //TODO: don't hard-code this
  topologyFile = "src/brite/examples/conf_files/TD_ASBarabasi_RTWaxman.conf";

  //TODO: make these part of the object so we can change it
  double boundaryLength = 1000;
  
  Ptr<BriteRegionHelper> regionHelper = DynamicCast<BriteRegionHelper> (GetRegionHelper ());
  NS_ASSERT_MSG (regionHelper, "Need BriteRegionHelper for ReadBriteTopology!");

  regionHelper->SetTopologySize (boundaryLength);

  BriteTopologyHelper bth (topologyFile);
  // need to seed it with something different from any other running procs
  std::size_t seed = 0;
  boost::hash_combine (seed, std::time (NULL));
  boost::hash_combine (seed, getpid());
  bth.AssignStreams (seed);

  //TODO: BGP routing and others

  // need a list routing setup
  Ipv4NixVectorHelper nixRouting;
  nixRouting.SetAttribute("FollowDownEdges", BooleanValue (true));
  Ipv4StaticRoutingHelper staticRouting;
  Ipv4ListRoutingHelper routingList;
  routingList.Add (staticRouting, 0);
  routingList.Add (nixRouting, 10);
  InternetStackHelper stack;
  stack.SetRoutingHelper (routingList); // has effect on the next Install ()

  bth.BuildBriteTopology (stack);

  //TODO: hierarchical address assignment... maybe eventually
  Ipv4AddressHelper address;
  address.SetBase ("10.1.0.0", "255.255.255.252");
  bth.AssignIpv4Addresses (address);

  // BriteTopologyHelper doesn't give easy access to ALL nodes, so get all of the global ones
  nodes = NodeContainer::GetGlobal ();

  // TODO: save information about AS's for that heuristic and other reasons

  NS_LOG_INFO ("Number of AS created " << bth.GetNAs ());
}


/////////////////////////////////////////////////////////////////////
////////////////////    Rocketfuel Topology /////////////////////////
/////////////////////////////////////////////////////////////////////


void
GeocronExperiment::ReadLatencyFile (std::string latencyFile)
{
  // Load latency information if requested
  if (latencyFile != "")
    {
      if (! boost::filesystem::exists(latencyFile))
        {
          NS_LOG_ERROR("File does not exist: " + latencyFile);
          exit(-1);
        }
      latencies = RocketfuelTopologyReader::ReadLatencies (latencyFile);
    }
}


void
GeocronExperiment::ReadLocationFile (std::string locationFile)
{
  if (locationFile != "")
    {
      if (! boost::filesystem::exists (locationFile))
        {
          NS_LOG_ERROR("File does not exist: " + locationFile);
          exit(-1);
        }

      std::ifstream infile (locationFile.c_str ());
      std::string line;
      Location region;
      double lat = 0.0, lon = 0.0;
      std::vector<std::string> parts; //for splitting up line

      while (std::getline (infile, line))
        {
          boost::split (parts, line, boost::is_any_of ("\t"));
          region = parts[0];
          lat = boost::lexical_cast<double> (parts[1]);
          lon = boost::lexical_cast<double> (parts[2]);

          // Add region information to the RegionHelper
          Ptr<RocketfuelRegionHelper> rrh = DynamicCast<RocketfuelRegionHelper> (GetRegionHelper ());
          NS_ASSERT_MSG (rrh != NULL, "Can't read location file without a RocketfuelRegionHelper!"); 

          Vector vector = Vector (lat, lon, 1.0);
          rrh->SetRegion (vector, region);

          //std::cout << "Loc=" << region << ", lat=" << lat << ", lon=" << lon << std::endl;
          //std::cout << "Location read=" << region << ", Location in helper=" << GetRegionHelper ()->GetRegion (vector) << std::endl;
        }
    }
}


/** Read network topology, store nodes and devices. */
void
GeocronExperiment::ReadRocketfuelTopology (std::string topologyFile)
{
  this->topologyFile = topologyFile;

  if (! boost::filesystem::exists(topologyFile)) {
    NS_LOG_ERROR("File does not exist: " + topologyFile);
    exit(-1);
  }
  
  RocketfuelTopologyReader topo_reader;
  topo_reader.SetFileName(topologyFile);
  nodes = topo_reader.Read();
  NS_LOG_INFO ("Nodes read from file: " + boost::lexical_cast<std::string> (nodes.GetN()));

  NS_LOG_INFO ("Assigning addresses and installing interfaces...");

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("100Gbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  // NixHelper to install nix-vector routing
  Ipv4NixVectorHelper nixRouting;
  nixRouting.SetAttribute("FollowDownEdges", BooleanValue (true));
  Ipv4StaticRoutingHelper staticRouting;
  Ipv4ListRoutingHelper routingList;
  routingList.Add (staticRouting, 0);
  routingList.Add (nixRouting, 10);
  InternetStackHelper stack;
  stack.SetRoutingHelper (routingList); // has effect on the next Install ()
  stack.Install (nodes);

  //For each link in topology, add a connection between nodes and assign IP
  //addresses to the new network they've created between themselves.
  Ipv4AddressHelper address;
  address.SetBase ("10.1.0.0", "255.255.255.252");
  //NetDeviceContainer router_devices;
  //Ipv4InterfaceContainer router_interfaces;

  NS_LOG_INFO ("Generating links and checking failure model.");

  for (TopologyReader::ConstLinksIterator iter = topo_reader.LinksBegin();
       iter != topo_reader.LinksEnd(); iter++) {
    Location fromLocation = iter->GetAttribute ("From Location");
    Location toLocation = iter->GetAttribute ("To Location");

    // Set latency for this link if we loaded that information
    if (latencies.size())
      {
        RocketfuelTopologyReader::LatenciesMap::iterator latency = latencies.find (fromLocation + " -> " + toLocation);
        //if (iter->GetAttributeFailSafe ("Latency", latency))
        
        if (latency != latencies.end())
          {
            pointToPoint.SetChannelAttribute ("Delay", StringValue (latency->second + "ms"));
          }
        else
          {
            latency = latencies.find (toLocation + " -> " + fromLocation);
            if (latency != latencies.end())
              pointToPoint.SetChannelAttribute ("Delay", StringValue (latency->second + "ms"));
            else
              pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));
          }
      }

    // Nodes
    Ptr<Node> from_node = iter->GetFromNode();
    Ptr<Node> to_node = iter->GetToNode();
    NodeContainer both_nodes (from_node);
    both_nodes.Add (to_node);

    // NetDevices
    NetDeviceContainer new_devs = pointToPoint.Install (both_nodes);
 
    // Interfaces
    Ipv4InterfaceContainer new_interfaces = address.Assign (new_devs);
    //router_interfaces.Add(new_interfaces);
    address.NewNetwork();

    // Mobility model to set positions for geographically-correlated information
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAllocator = CreateObject<ListPositionAllocator> ();
    Vector fromPosition = GetRegionHelper ()->GetLocation (fromLocation);
    Vector toPosition = GetRegionHelper ()->GetLocation (toLocation);
    positionAllocator->Add (fromPosition);
    positionAllocator->Add (toPosition);
    mobility.SetPositionAllocator (positionAllocator);
    mobility.Install (both_nodes);
    
    NS_LOG_LOGIC ("Link from " << fromLocation << "[" << fromPosition << "] to " << toLocation<< "[" << toPosition << "]");
  } //end link iteration
}


void
GeocronExperiment::SetDisasterLocation (Location newDisasterLocation)
{
  currLocation = newDisasterLocation;
}


void
GeocronExperiment::SetFailureProbability (double newFailureProbability)
{
  currFprob = newFailureProbability;
}


/*void
GeocronExperiment::SetHeuristic (newHeuristic)
{

}*/



void
GeocronExperiment::SetTraceFile (std::string newTraceFile)
{
  NS_LOG_INFO ("New trace file is: " + newTraceFile);

  traceFile = newTraceFile;
}


void
GeocronExperiment::AutoSetTraceFile ()
{
  boost::filesystem::path newTraceFile ("ron_output");
  newTraceFile /= boost::filesystem::path(topologyFile).stem ();
  newTraceFile /= boost::algorithm::replace_all_copy (currLocation, " ", "_");

  // round the fprob to 1 decimal place
  std::ostringstream fprob;
  std::setprecision (1);
  fprob << currFprob;
  newTraceFile /= fprob.str ();
  //newTraceFile /= boost::lexical_cast<std::string> (currFprob);

  // extract unique filename from heuristic to summarize parameters, aggregations, etc.
  TypeId::AttributeInformation info;
  NS_ASSERT (currHeuristic->GetTypeId ().LookupAttributeByName ("SummaryName", &info));
  //StringValue summary;
  //TODO: get this working with a checker
  StringValue summary = *(DynamicCast<const StringValue> (info.initialValue));
  //info.checker->Check (summary);
  NS_ASSERT_MSG (summary.Get () != "", "Blank SummaryName!");
  newTraceFile /= summary.Get ();

  // set output number if start_run_number is specified
  uint32_t outnum = currRun;
  if (start_run_number and nruns > 1)
    outnum = currRun + start_run_number;
  
  std::string fname = "run";
  fname += boost::lexical_cast<std::string> (outnum);
  newTraceFile /= (fname);
  newTraceFile.replace_extension(".out");

  // Change name to avoid overwriting
  uint32_t copy = 0;
  while (boost::filesystem::exists (newTraceFile))
    {
      newTraceFile.replace_extension (".out(" + boost::lexical_cast<std::string> (copy++) + ")");
    }

  boost::filesystem::create_directories (newTraceFile.parent_path ());

  SetTraceFile (newTraceFile.string ());
}


/** Run through all the possible combinations of disaster locations, failure probabilities,
    heuristics, and other parameters for the given number of runs. */
void
GeocronExperiment::RunAllScenarios ()
{
  //set the seed using both clock and pid so that simultaneously running sims don't overlap
  std::size_t seed = 0;
  boost::hash_combine (seed, std::time (NULL));
  boost::hash_combine (seed, getpid());
  SeedManager::SetSeed(seed);
  int runSeed = 0;
  for (std::vector<Location>::iterator disasterLocation = disasterLocations->begin ();
       disasterLocation != disasterLocations->end (); disasterLocation++)
    {
      SetDisasterLocation (*disasterLocation); 
      for (std::vector<double>::iterator fprob = failureProbabilities->begin ();
           fprob != failureProbabilities->end (); fprob++)
        {
          SetFailureProbability (*fprob);
          for (currRun = 0; currRun < nruns; currRun++)
            {
              // We want to compare each heuristic to each other for each configuration of failures
              ApplyFailureModel ();
              SetNextServers ();
              for (uint32_t h = 0; h < heuristics->size (); h++)
                {
                  currHeuristic = heuristics->at (h);
                  SeedManager::SetRun(runSeed++);
                  AutoSetTraceFile ();
                  Run ();
                }
              UnapplyFailureModel ();
            }
        }
    }
}


/** Connect the defined traces to the specified client applications. */
void
GeocronExperiment::ConnectAppTraces ()
{
  //need to prevent stupid waf from throwing errors for unused definitions...
  (void) PacketForwarded;
  (void) PacketSent;
  (void) AckReceived;

  if (traceFile != "")
    {
      Ptr<OutputStreamWrapper> traceOutputStream;
      AsciiTraceHelper asciiTraceHelper;
      traceOutputStream = asciiTraceHelper.CreateFileStream (traceFile);
  
      for (ApplicationContainer::Iterator itr = clientApps.Begin ();
           itr != clientApps.End (); itr++)
        {
          Ptr<RonClient> app = DynamicCast<RonClient> (*itr);
          app->ConnectTraces (traceOutputStream);
        }
    }
}


//////////////////////////////////////////////////////////////////////
/////************************************************************/////
////////////////             FAILURE MODEL          //////////////////
/////************************************************************/////
//////////////////////////////////////////////////////////////////////


void
GeocronExperiment::IndexNodes () {
  NS_LOG_INFO ("Topology finished.  Choosing & installing clients.");

  // We want to narrow down the number of server choices by picking about nServerChoices of the highest-degree
  // nodes.  We want at least nServerChoices of them, but will allow more so as not to include some of degree d
  // but exclude others of degree d.
  //
  // So we keep an ordered list of nodes and keep track of the sums
  //
  // invariant: if the group of smallest degree nodes were removed from the list,
  // its size would be < nServerChoices
  typedef std::pair<uint32_t/*degree*/, Ptr<Node> > DegreeType;
  typedef boost::heap::priority_queue<DegreeType, boost::heap::compare<std::greater<DegreeType> > > DegreePriorityQueue;
  DegreePriorityQueue potentialServerNodeCandidates;
  uint32_t nLowestDegreeNodes = 0, lowDegree = 0;

  // For finding all the overlay nodes, which must be done before installing applications since we need to specify the peers
  NodeContainer overlayNodes;

  for (NodeContainer::Iterator node = nodes.Begin ();
       node != nodes.End (); node++)
    {
      uint32_t degree = GetNodeDegree (*node);

      // Sanity check that a node has some actual links, otherwise remove it from the simulation
      // this happened with some disconnected Rocketfuel models and made null pointers
      if (degree <= 0)
        {
          NS_LOG_LOGIC ("Node " << (*node)->GetId () << " has no links!");
          //node.erase ();
          disasterNodes[currLocation].erase ((*node)->GetId ());
          continue;
        }

      // Aggregate RonPeerEntry objects for getting useful peering information later
      Ptr<RonPeerEntry> thisPeer = (*node)->GetObject<RonPeerEntry> ();

      // sanity check that this is the only place we set this info
#ifdef NS3_LOG_ENABLE
      if (thisPeer)
        NS_ASSERT_MSG (false, "Where did this RonPeerEntry come from???");
#endif

      Location nodeRegion = GetRegionHelper ()->GetRegion(GetLocation(*node));
      // note that this swanky RonPeerEntry constructor handles the other attributes
      thisPeer = CreateObject<RonPeerEntry> (*node);
      thisPeer->region = nodeRegion;
      (*node)->AggregateObject (thisPeer);

      // Build disaster node indexes
      // If the node is in a disaster region, add it to the corresponding list

      for (std::vector<Location>::iterator disasterLocation = disasterLocations->begin ();
           // Only bother if the region is defined
           disasterLocation != disasterLocations->end () and nodeRegion != NULL_REGION;
           disasterLocation++)
        {
          if (nodeRegion == *disasterLocation)
            {
              disasterNodes[*disasterLocation].insert(std::pair<uint32_t, Ptr<Node> > ((*node)->GetId (), *node));
            }
 
          // Used for debugging to make sure locations are being found properly
#ifdef NS3_LOG_ENABLE
          if (!HasLocation (*node))
            {
              NS_LOG_DEBUG ("Node " << (*node)->GetId () << " has no position!");
            }
#endif
        } // end disaster location iteration
      
      // OVERLAY NODES
      //
      // We may only install the overlay application on clients attached to stub networks,
      // so we just choose the stub network nodes here
      // (note that all nodes have a loopback device)
      if (IsOverlayNode (*node)) 
        {
          overlayNodes.Add (*node);
          overlayPeers->AddPeer (*node);
        }

      // ROUTING NODES
      // must ignore if no location information, since clients expect server location info!
      else if (!IsOverlayNode (*node) and HasLocation (*node))
        {
          //add potential server
          if (degree >= lowDegree or potentialServerNodeCandidates.size () < nServerChoices)
            potentialServerNodeCandidates.push (std::pair<uint32_t, Ptr<Node> > (degree, *node));
          else
            continue;

          //base condition to set lowDegree the 1st time
          if (!lowDegree)
            lowDegree = degree;

          if (degree == lowDegree)
            nLowestDegreeNodes++;
          else if (degree < lowDegree)
            {
              lowDegree = degree;
              nLowestDegreeNodes = 1;
            }

          //remove group of lowest degree nodes if it's okay to do so
          if (potentialServerNodeCandidates.size () - nLowestDegreeNodes >= nServerChoices)
            {
              #ifdef NS3_LOG_ENABLE
              uint32_t nThrownAway = 0;
              #endif
              while (potentialServerNodeCandidates.top ().first == lowDegree)
                {
                  potentialServerNodeCandidates.pop ();
                  #ifdef NS3_LOG_ENABLE
                  nThrownAway++;
                  #endif
                }
              NS_LOG_DEBUG ("Throwing away " << nThrownAway << " vertices of degree " << lowDegree);

              // recalculate nLowestDegreeNodes
              lowDegree = GetNodeDegree (potentialServerNodeCandidates.top ().second);
              nLowestDegreeNodes = 0;
              for (DegreePriorityQueue::iterator itr = potentialServerNodeCandidates.begin ();
                   itr != potentialServerNodeCandidates.end () and (itr)->first == lowDegree; itr++)
                nLowestDegreeNodes++;
              NS_LOG_DEBUG (nLowestDegreeNodes << " nodes have new lowest degree = " << lowDegree);
            }
        }
    } // end node iteration

  // Install client applications
  // NOTE: this must be done after finding ALL the overlay nodes so that we can assign peers to each application
  RonClientHelper ronClient (9);
  ronClient.SetAttribute ("Interval", TimeValue (Seconds (1.)));
  ronClient.SetAttribute ("PacketSize", UintegerValue (1024));
  ronClient.SetAttribute ("Timeout", TimeValue (timeout));
  
  // Install client app and set number of server contacts they make based on whether all nodes
  // should report the disaster or only the ones in the disaster region.
  for (NodeContainer::Iterator node = overlayNodes.Begin ();
       node != overlayNodes.End (); node++)
    {
      ApplicationContainer newApp = ronClient.Install (*node);
      clientApps.Add (newApp);
      Ptr<RonClient> newClient = DynamicCast<RonClient> (newApp.Get (0));
      newClient->SetAttribute ("MaxPackets", UintegerValue (0)); //explicitly enable sensor reporting when disaster area set
      newClient->SetPeerTable (overlayPeers); //TODO: make this part of the helper?? or some p2p overlay algorithm? or the table itself? give out a callback function with that nodes peers?
    }
  
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (appStopTime);

  // Now cache the actual choices of server nodes for each disaster region
  NS_LOG_DEBUG (potentialServerNodeCandidates.size () << " total potentialServerNodeCandidates.");
  for (DegreePriorityQueue::iterator itr = potentialServerNodeCandidates.begin ();
       itr != potentialServerNodeCandidates.end (); itr++)
    {
      NS_LOG_DEBUG ("Node " << itr->second->GetId () << " has degree " << GetNodeDegree (itr->second));
      Ptr<Node> serverCandidate = itr->second;
      Ptr<RonPeerEntry> candidatePeer = serverCandidate->GetObject<RonPeerEntry> ();
      NS_ASSERT_MSG (candidatePeer != NULL, "Server doesn't have a RonPeerEntry!");
      Location loc = candidatePeer->region;

      for (std::vector<Location>::iterator disasterLocation = disasterLocations->begin ();
           disasterLocation != disasterLocations->end (); disasterLocation++)
        {
          if (*disasterLocation != loc)
            serverNodeCandidates[*disasterLocation].Add (serverCandidate);
        }
    }
}

/**   Fail the links that were chosen with given probability */
void
GeocronExperiment::ApplyFailureModel () {
  NS_LOG_INFO ("Applying failure model.");

  // Keep track of these and unfail them later
  failNodes = NodeContainer ();
  ifacesToKill = Ipv4InterfaceContainer ();

  for (std::map<uint32_t, Ptr <Node> >::iterator nodeItr = disasterNodes[currLocation].begin ();
       nodeItr != disasterNodes[currLocation].end (); nodeItr++)
    {
      Ptr<Node> node = nodeItr->second;
      // Fail nodes within the disaster region with some probability
      if (random.GetValue () < currFprob)
        {
          failNodes.Add (node);
          NS_LOG_LOGIC ("Node " << (node)->GetId () << " failed.");
          FailNode (node);
        }

      else {
        // Check all links on this node if it wasn't failed (which would fail all links anyway)
        // NOTE: iface 0 is the loopback
        Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
        NS_ASSERT_MSG(ipv4 != NULL, "Node has no Ipv4 object!");

        for (uint32_t i = 1; i <= GetNodeDegree(node); i++)
          {
            if (random.GetValue () < currFprob)
              {
                ifacesToKill.Add(ipv4, i);
                FailIpv4 (ipv4, i);
              }
          }
      }
    }
}


void
GeocronExperiment::UnapplyFailureModel () {
  // Unfail the links that were chosen
  for (Ipv4InterfaceContainer::Iterator iface = ifacesToKill.Begin ();
       iface != ifacesToKill.End (); iface++)
    {
      UnfailIpv4 (iface->first, iface->second);
    }

  // Unfail the nodes that were chosen
  for (NodeContainer::Iterator node = failNodes.Begin ();
       node != failNodes.End (); node++)
    {
      UnfailNode (*node, appStopTime);
    }


  clientApps.Start (Seconds (2.0));
  clientApps.Stop (appStopTime);
}


void
GeocronExperiment::SetNextServers () {
  NS_LOG_LOGIC ("Choosing from " << serverNodeCandidates[currLocation].GetN () << " server provider candidates.");

  Ptr<Node> serverNode = (serverNodeCandidates[currLocation].GetN () ?
                          serverNodeCandidates[currLocation].Get (random.GetInteger (0, serverNodeCandidates[currLocation].GetN () - 1)) :
                          nodes.Get (random.GetInteger (0, serverNodeCandidates[currLocation].GetN () - 1)));

  //Application
  RonServerHelper ronServer (9);

  ApplicationContainer serverApps = ronServer.Install (serverNode);
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (appStopTime);

  serverPeers = Create<RonPeerTable> ();
  serverPeers->AddPeer (serverNode);
}


void
GeocronExperiment::Run ()
{
  int numDisasterPeers = 0;

  // Set up the proper heuristics, peer tables, and calculate the number of overlay nodes.
  for (std::map<uint32_t, Ptr <Node> >::iterator nodeItr = disasterNodes[currLocation].begin ();
       nodeItr != disasterNodes[currLocation].end (); nodeItr++)
    /*  for (ApplicationContainer::Iterator app = disasterNodes[currLocation].Begin ();
        app != clientApps.End (); app++)*/
    {
      // skip over routers
      if (!IsOverlayNode (nodeItr->second))
        continue;

      // iterate over all applications just in case we add other apps later on...
      for (uint32_t i = 0; i < nodeItr->second->GetNApplications (); i++) {
        Ptr<RonClient> ronClient = DynamicCast<RonClient> (nodeItr->second->GetApplication (0));
        if (ronClient == NULL) {
          continue;
        }

        //add the heuristic and aggregate a weak random one to it to break ties
        //TODO: make a helper?
        //TODO: have the random be part of the object factory/attributes?
        //TODO: just define it as the input to the sim?
        Ptr<RonPathHeuristic> heuristic = currHeuristic->Create<RonPathHeuristic> ();
        // Must set heuristic first so that source will be set and heuristic can make its heap
        heuristic->MakeTopLevel (); //must always do this! TODO: not need to?
        ronClient->SetHeuristic (heuristic);
        ronClient->SetServerPeerTable (serverPeers);
        heuristic->SetPeerTable (overlayPeers);

        //TODO: do this by TypeId and remove #include from beggining
        Ptr<RonPathHeuristic> randHeuristic = CreateObject<RandomRonPathHeuristic> ();
        //TODO: get this working
        randHeuristic->SetAttribute ("Weight", DoubleValue (0.01));
        //randHeuristic->
        heuristic->AddHeuristic (randHeuristic);

        ronClient->SetAttribute ("MaxPackets", UintegerValue (contactAttempts));
        numDisasterPeers++;
      }
    }

  //NS_LOG_INFO ("Populating routing tables; please be patient it takes a while...");
  //Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  //NS_LOG_INFO ("Done populating routing tables!");
  
  ConnectAppTraces ();

  // pointToPoint.EnablePcap("rocketfuel-example",router_devices.Get(0),true);

  NS_LOG_UNCOND ("Starting simulation on map file " << topologyFile << ": " << std::endl
                 << nodes.GetN () << " total nodes" << std::endl
                 << overlayPeers->GetN () << " total overlay nodes" << std::endl
                 << disasterNodes[currLocation].size () << " nodes in " << currLocation << " total" << std::endl
                 << numDisasterPeers << " overlay nodes in " << currLocation << std::endl //TODO: get size from table
                 << std::endl << "Failure probability: " << currFprob << std::endl
                 << failNodes.GetN () << " nodes failed" << std::endl
                 << ifacesToKill.GetN () / 2 << " links failed");

  Simulator::Stop (simulationLength);
  Simulator::Run ();
  Simulator::Destroy ();

  NS_LOG_INFO ("Next simulation run...");

  //serverNode->GetObject<Ipv4NixVectorRouting> ()->FlushGlobalNixRoutingCache ();
  
  // reset apps
  for (ApplicationContainer::Iterator app = clientApps.Begin ();
       app != clientApps.End (); app++)
    {
      Ptr<RonClient> ronClient = DynamicCast<RonClient> (*app);
      ronClient->Reset ();
    }
}
