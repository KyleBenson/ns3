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
#include <cmath>
#include <unistd.h>
#include <boost/functional/hash.hpp>

// Enables using a gaussian model for failure probability
// based on distance from center of disaster
#define GAUSSIAN_FAILURE_MODEL

// Sets peer tables for each client to be the table of all overlay peers.
// Disabling chooses a random subset of the peers totalling c*log(n).
#define USE_FULL_PEER_TABLE

// If enabled, only nodes in the disaster region will report disasters
// and potentially fail
//#define DISASTER_NODES_ONLY_IN_REGION

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
                   StringValue ("inet"),
                   MakeStringAccessor (&GeocronExperiment::topologyType),
                   MakeStringChecker ())
    .AddAttribute ("TraceFolderName",
                   "Name of the folder that will contain trace outputs.",
                   StringValue ("ron_output"),
                   MakeStringAccessor (&GeocronExperiment::traceFolderName),
                   MakeStringChecker ())
    .AddAttribute ("Random",
            "The random variable for random decisions.",
            StringValue ("ns3::UniformRandomVariable"),
            MakePointerAccessor (&GeocronExperiment::random),
            MakePointerChecker<RandomVariableStream> ())
    .AddAttribute ("GaussianRandom",
            "The gaussian random variable for random decisions, in particular failure probability.",
            StringValue ("ns3::NormalRandomVariable"),
            MakePointerAccessor (&GeocronExperiment::gaussianRandom),
            MakePointerChecker<RandomVariableStream> ())
    .AddAttribute ("NumberServersChoices",
            "Number of servers to index based on their being external to disaster regions."
            "This number should of course be bigger than --nservers",
            IntegerValue (10),
            MakeIntegerAccessor (&GeocronExperiment::nServerChoices),
            MakeIntegerChecker<uint32_t> ())
    .AddAttribute ("PeerTableSizeScalingFactor",
            "When generating peer tables, each will be of size PeerTableSizeScalingFactor*log(n), where n is the number of peers",
            IntegerValue (10),
            MakeIntegerAccessor (&GeocronExperiment::peerTableSizeScalingFactor),
            MakeIntegerChecker<uint32_t> ())
  ;
  return tid;
}


GeocronExperiment::GeocronExperiment ()
{
  appStopTime = Time (Seconds (30.0));
  simulationLength = Seconds (10.0);
  overlayPeers = Create<RonPeerTable> ();
  // This should be set to a different table in ApplyExperimentalTreatment()
  activeOverlayPeers = NULL;
  allPeers = RonPeerTable::GetMaster ();

  maxNDevs = 2;
  //cmd.AddValue ("install_stubs", "If not 0, install RON client only on stub nodes (have <= specified links - 1 (for loopback dev))", maxNDevs);

  //these defaults should all be overwritten by the command line parser
  currLocation = "";
  currFprob = 0.0;
  currRun = 0;
  currNPaths = 1;
  currNServers = 1;
  currExperimentalTreatment = "default";

  contactAttempts = 10;
  traceFile = "";
  nruns = 1;
  seed = 0;
  start_run_number = 0;

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
      // BriteRegionHelper just divides into a grid for regions
      else if (topologyType == "inet") {
        regionHelper = CreateObject<BriteRegionHelper> ();
      }
      else {
        NS_ASSERT_MSG(false, "Invalid topology type: " << topologyType);
      }
      NS_LOG_INFO ("Using topology generator " << topologyType);
    }
  return regionHelper;
}

void
GeocronExperiment::SetTimeout (Time newTimeout)
{
  timeout = newTimeout;
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
  // This was for randomly choosing nodes to be in the overlay when using
  // topologies like BRITE, Rocketfuel.  Now, we explicitly place nodes of
  // different types in the topology and so can directly check this...

  return IsSeismicSensor (node) or IsBasestation (node);

  // Old implementation: first part handles if we didn't specify a
  // max degree for overlay nodes, in which case ALL nodes are overlays...
  //return !maxNDevs or GetNodeDegree (node) <= maxNDevs;
}


/////////////////////////////////////////////////////////////////////
////////////////////    Geocron (Inet-based) Topology ///////////////
/////////////////////////////////////////////////////////////////////


void GeocronExperiment::ReadGeocronInetTopology (std::string topologyFile)
{
  GeocronInetTopologyReader topo;
  topo.SetFileName (topologyFile);
  nodes = topo.Read ();

  // need a list routing setup
  Ipv4NixVectorHelper nixRouting;
  nixRouting.SetAttribute("FollowDownEdges", BooleanValue (true));
  Ipv4StaticRoutingHelper staticRouting;
  Ipv4ListRoutingHelper routingList;
  routingList.Add (staticRouting, 0);
  routingList.Add (nixRouting, 10);

  // setup network stack and install it on all nodes
  InternetStackHelper stack;
  stack.SetRoutingHelper (routingList); // has effect on the next Install ()
  stack.Install (nodes);

  // create physical links and setup networking on them
  PointToPointHelper p2p;
  Ipv4AddressHelper address;
  address.SetBase ("10.1.0.0", "255.255.255.252");
  TopologyReader::ConstLinksIterator iter;
  for ( iter = topo.LinksBegin (); iter != topo.LinksEnd (); iter++)
  {
    NodeContainer theseNodes (iter->GetFromNode (), iter->GetToNode ());
    // TODO: set delay based on distance? this is tricky as it is really
    // the speed of light, which results in essentially negligible delays
    // (sub ms) for any distance under 200km
    // TODO: account for different types of links (e.g. p2p vs. UNB vs. WIMAX)
    // between different node types
    p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));
    // Below was used in Rocketfuel files, which have associated latency files, I think...
    // p2p.SetChannelAttribute ("Delay", TimeValue(MilliSeconds(weight[i])));
    // Set link bandwidth based on the "weight" parameter included in
    // Inet topology, which is assumed to be in Mbps
    std::string bw = "100";
    iter->GetAttributeFailSafe (std::string ("Weight"), bw);
    p2p.SetDeviceAttribute ("DataRate", StringValue (bw + "Mbps"));
    NetDeviceContainer devs = p2p.Install (theseNodes);
    address.Assign (devs);
    address.NewNetwork ();

    //NS_LOG_LOGIC ("Link from " << fromLocation << "[" << fromPosition << "] to " << toLocation<< "[" << toPosition << "]");
  }

  // Lastly, set the topology boundary for the RegionHelper
  //
  //TODO: make these part of the object so we can change it
  //OR we could just determine it by the max/min x/y coordinates?
  double boundaryLength = 50;

  Ptr<BriteRegionHelper> regionHelper = DynamicCast<BriteRegionHelper> (GetRegionHelper ());
  NS_ASSERT_MSG (regionHelper, "Need BriteRegionHelper for ReadGeocronInetTopology!");

  regionHelper->SetTopologySize (boundaryLength);

  // HACK: the simulator was set up when using BRITE to pick a set of candidate servers
  // outside each disaster region according to high node degrees.  With this topology,
  // we explicitly placed a server node so to keep the previous functionality intact
  // we simply add each of the server nodes to the candidate list for each region.
  //
  for (NodeContainer::Iterator serverCandidate = topo.servers.Begin ();
      serverCandidate != topo.servers.End (); serverCandidate++)
  {
    for (std::vector<Location>::iterator disasterLocation = disasterLocations->begin ();
        disasterLocation != disasterLocations->end (); disasterLocation++)
    {
      serverNodeCandidates[*disasterLocation].Add (*serverCandidate);
    }
  }

}


/////////////////////////////////////////////////////////////////////
////////////////////    BRITE Topology //////////////////////////////
/////////////////////////////////////////////////////////////////////


void GeocronExperiment::ReadBriteTopology (std::string topologyFile)
{
  //TODO: make these part of the object so we can change it
  double boundaryLength = 1000;

  Ptr<BriteRegionHelper> regionHelper = DynamicCast<BriteRegionHelper> (GetRegionHelper ());
  NS_ASSERT_MSG (regionHelper, "Need BriteRegionHelper for ReadBriteTopology!");

  regionHelper->SetTopologySize (boundaryLength);

  BriteTopologyHelper bth (topologyFile);
  // need to seed it with something different from any other running procs
  if (seed == 0)
  {
    boost::hash_combine (seed, std::time (NULL));
    boost::hash_combine (seed, getpid());
  }
  bth.AssignStreams (seed);

  // the topologyFile attribute is used to determine the name of the directory hierarchy
  // the trace outputs will be saved to
  this->topologyFile = boost::lexical_cast<std::string> (seed);

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

  uint32_t n_leaves = 0;
  for (uint32_t i = 0; i < bth.GetNAs (); i++)
    n_leaves += bth.GetNLeafNodesForAs (i);

  NS_LOG_INFO ("Number of leaves " << n_leaves);
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
GeocronExperiment::ApplyExperimentalTreatment (std::string treatment)
{
  currExperimentalTreatment = treatment;

  if (treatment == "sw" or treatment == "ws")
    activeOverlayPeers = overlayPeers;
  else if (treatment == "s")
    activeOverlayPeers = peersByType["seismicSensor"];
  else if (treatment == "w")
    activeOverlayPeers = peersByType["basestation"];
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
  // The trace files should look like:
  // ron_output/topology/disaster_location/fprob/nservers/heuristic/run#.out

  boost::filesystem::path newTraceFile (traceFolderName);
  newTraceFile /= boost::filesystem::path(topologyFile).stem ();
  newTraceFile /= boost::algorithm::replace_all_copy (currLocation, " ", "_");

  // round the fprob to 1 decimal place
  std::ostringstream fprob;
  std::setprecision (1);
  fprob << currFprob;
  newTraceFile /= fprob.str ();

  newTraceFile /= boost::lexical_cast<std::string> (currNServers);
  newTraceFile /= boost::lexical_cast<std::string> (currNPaths);
  newTraceFile /= currExperimentalTreatment;

  // extract unique filename from heuristic to summarize parameters, aggregations, etc.
  // Do this by creating an instance of the current heuristic so that
  // we can call GetAttribute on it (ObjectFactory has no Get())
  StringValue info;
  Ptr<Object> heur = currHeuristic->Create ();
  heur->GetAttribute ("SummaryName", info);
  NS_ASSERT_MSG (info.Get () != "", "Blank SummaryName!");
  newTraceFile /= info.Get ();

  // set output number if start_run_number is specified
  uint32_t outnum = currRun;
  if (start_run_number)
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

  NS_LOG_UNCOND ("========================================================================");
  NS_LOG_INFO ("Running all scenarios...");

  // TODO: move this to a class member and set it via args rather than hard-coded
  // Each treatment represents a custom experimental setup not accounted
  // for in the rest of the simulation parameters.
  std::vector<std::string> experimentalTreatments;
  experimentalTreatments.push_back ("s");
  experimentalTreatments.push_back ("w");
  experimentalTreatments.push_back ("sw");

  // Loop over all the possible scenarios, running the simulation and resetting between each
  for (std::vector<std::string>::iterator expTreatment = experimentalTreatments.begin ();
      expTreatment != experimentalTreatments.end (); expTreatment++)
  {
    ApplyExperimentalTreatment (*expTreatment);
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
          // We want to compare each heuristic configuration (which heuristic, multipath fanout, etc.)
          // to each other for each configuration of failures
          //
          ApplyFailureModel ();
          //
          // TODO: if we go back to randomly picking servers,
          // shouldn't we ensure that some portion of the same
          // servers are chosen between runs so the randomness
          // isn't due so much to the server choices?
          for (uint32_t s = 0; s < nservers->size (); s++)
          {
            currNServers = nservers->at (s);
            SetNextServers ();

            for (uint32_t p = 0; p < npaths->size (); p++)
            {
              for (uint32_t h = 0; h < heuristics->size (); h++)
              {
                currHeuristic = heuristics->at (h);
                currNPaths = npaths->at (p);
                SeedManager::SetRun(runSeed++);
                AutoSetTraceFile ();
                Run ();
              }
            }
          }
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
  (void) PacketReceivedAtServer;

  if (traceFile != "")
    {
      Ptr<OutputStreamWrapper> traceOutputStream;
      AsciiTraceHelper asciiTraceHelper;
      traceOutputStream = asciiTraceHelper.CreateFileStream (traceFile);

      // connect client apps
      for (ApplicationContainer::Iterator itr = clientApps.Begin ();
           itr != clientApps.End (); itr++)
        {
          Ptr<RonClient> app = DynamicCast<RonClient> (*itr);
          app->ConnectTraces (traceOutputStream);
        }

      // connect the server apps
      for (ApplicationContainer::Iterator itr = serverApps.Begin ();
           itr != serverApps.End (); itr++)
        {
          Ptr<RonServer> app = DynamicCast<RonServer> (*itr);
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
  // This function iterates over all the nodes and builds various
  // structures that let us choose the following types of nodes:
  // servers, disaster nodes, nodes indexed by type, and all overlay nodes.
  // It also adds RonPeerEntry's to Nodes, sets region information,
  // and installs RonClientApplications, all of which is crucial.
  //
  // NOTE: some of this only needs to be done when using certain
  // topologies (e.g. BRITE) as the new GeocronInet topology style
  // explicitly categorizes each node type in the input file.
  //
  NS_LOG_INFO ("Topology finished.  Choosing & installing clients.");
  bool choosingServers = topologyType != "inet";

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
      std::string nodeType = GetNodeType (*node);

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
      // If the node is in a disaster region and the simulator is
      // configured for only those nodes to be active in the
      // disaster model, add it to the corresponding list
      for (std::vector<Location>::iterator disasterLocation = disasterLocations->begin ();
           // Only bother if the region is defined
           disasterLocation != disasterLocations->end () and nodeRegion != NULL_REGION;
           disasterLocation++)
        {
#ifdef DISASTER_NODES_ONLY_IN_REGION
          if (nodeRegion == *disasterLocation)
#endif
              disasterNodes[*disasterLocation].insert(std::pair<uint32_t, Ptr<Node> > ((*node)->GetId (), *node));

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
      // We may only install the overlay application on certain clients.
      // These may be explicitly defined (GeocronInet topology model) or
      // they may be determined by checking if they are attached to stub networks
      // (their degree is less than a certain value).
      if (IsOverlayNode (*node))
        {
          overlayNodes.Add (*node);
          Ptr<RonPeerEntry> newEntry = overlayPeers->AddPeer (*node);
          allPeers->AddPeer (newEntry);

          if (peersByType.count (nodeType) == 0)
            peersByType[nodeType] = Create<RonPeerTable> ();
          peersByType[nodeType]->AddPeer (newEntry);
        }

      // ROUTING NODES (choose servers)
      // must ignore if no location information, since clients expect server location info!
      else if (choosingServers and !IsOverlayNode (*node) and HasLocation (*node))
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
      // NOTE: we will set the peer table later since we will randomly assign a new one for each run
    }
  
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (appStopTime);

  // Now cache the actual choices of server nodes for each disaster region
  if (choosingServers)
  {
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
}

/** Determine the probability that the given node will fail due to a disaster at the given location. */
bool
GeocronExperiment::ShouldFailNode(Ptr<Node> node, Vector disasterLocation)
{
  // Failure model: Gaussian-like model chooses failure probability of
  // a node as fprob/2^(D*S/B) where:
  // fprob = base failure probability
  // S = split factor of region (SxS grid cells)
  // B = boundary length of whole region
  // This was derived from an approximation model where each cell away
  // from the disaster location halved the fprob.  This is the continuous,
  // rather than step-wise, version of that.
  //
  // TODO: implement actual Gaussian model below that normalizes the
  // probabilities based on the size of the disaster (region size)
  //
  // Failure model: get a random value normally distributed with a 0 mean and R/3 stdev,
  // ensure it's positive since we're dealing with distance, and return true if this value
  // is greater than the node's distance from the center of the circle.
  // In this manner, nodes farther away are much less likely to fail.
  // Note that we ensure 99.7% of the cumulative distribution is within the disaster
  // circle by setting the standard deviation to be 1/3rd the radius of the circle.
#ifdef GAUSSIAN_FAILURE_MODEL
  Ptr<BriteRegionHelper> regionHelper = DynamicCast<BriteRegionHelper> (GetRegionHelper ());

  double distance = CalculateDistance (GetLocation (node), disasterLocation);

  // This is equal to B/S
  double disasterRadius = regionHelper->GetRegionSize ();
  double thisFprob = currFprob / std::pow (2, distance / disasterRadius);

  //NS_LOG_UNCOND ("Fprob for Node " << GetNodeName (node) <<
      //" at distance " << distance << " from disaster at " <<
      //disasterLocation << " is " << thisFprob);

  return (random->GetValue () < thisFprob);

  //NS_LOG_UNCOND ("Distance: " << distance << ", radius: " << disasterRadius);

  //double randValue = std::abs(gaussianRandom->GetValue(0, std::pow(disasterRadius/3, 2), disasterRadius));
  // scale this value using the current failure probability so we can tune the number of failures
  // TODO: fine-tune this better and normalize so that the number failed
  // is the same between gaussian and uniform
  //randValue *= 2 * currFprob;
  //NS_LOG_UNCOND ("Rand is: " << randValue);
  //return (randValue > distance);
#else
  return (random->GetValue () < currFprob);
#endif
}

/** Determine the probability that the given WIRED link will fail
 * due to a disaster at the given location.
 * NOTE: currently it simply returns the logical or of
 * the two nodes' call to ShouldFail.
 *
 * TODO: It takes into
 * account the fact that this link may pass closer to the
 * center of a disaster and so should be based on the actual
 * line representing the link, rather than just the endpoints.
 */
bool
GeocronExperiment::ShouldFailLink(Ptr<Node> node1, Ptr<Node> node2, Vector disasterLocation)
{
  return ShouldFailNode(node1, disasterLocation) or ShouldFailNode(node2, disasterLocation);
}

/**   Fail the links that were chosen with given probability */
void
GeocronExperiment::ApplyFailureModel () {
  NS_LOG_INFO ("Applying failure model.");

  // Keep track of these and unfail them later
  failNodes = NodeContainer ();
  ifacesToKill = Ipv4InterfaceContainer ();

  Vector disasterLocation = GetRegionHelper ()->GetLocation (currLocation);

  for (std::map<uint32_t, Ptr <Node> >::iterator nodeItr = disasterNodes[currLocation].begin ();
       nodeItr != disasterNodes[currLocation].end (); nodeItr++)
    {
      Ptr<Node> node = nodeItr->second;

      // Fail nodes within the disaster region with some probability
      // NOTE: we never fail the server as it is assumed to be far away
      // from the disaster region.
      if (!IsServer (node) and ShouldFailNode(node, disasterLocation))
        {
          failNodes.Add (node);
          NS_LOG_LOGIC ("Node " << GetNodeName (node) << " failed.");
          FailNode (node);
        }

      else if (!IsServer (node)) {
        // Check all links on this node if it wasn't failed (which would fail all links anyway)
        // NOTE: iface 0 is the loopback
        Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
        NS_ASSERT_MSG(ipv4 != NULL, "Node has no Ipv4 object!");

        for (uint32_t i = 1; i <= GetNodeDegree(node); i++)
          {
            Ptr<NetDevice> thisNetDevice = node->GetDevice (i);

            // HACK: We don't fail wireless links, which exist between the
            // water sensors and basestations as well as between basestations.
            // NOTE: this check needs to come before we try to get the
            // other node as our method of doing so assumes p2p links.
            if (IsWaterSensor (node) or
                (IsBasestation (node) and !thisNetDevice->IsPointToPoint ()))
              continue;

            // get the necessary info for the other end of the link
            Ptr<NetDevice> otherNetDevice = GetOtherNetDevice (thisNetDevice);
            Ptr<Node> otherNode = otherNetDevice->GetNode ();

            // HACK: NOTE: for simplicity, we opted to connect the BS's to
            // each other using P2P links rather than WiMAX, so here we
            // need to explicitly check if this link is between two BS's
            // and not fail it if that's the case.
            // TODO: remove this if we ever use wired links between BS's
            if (IsBasestation (node) and IsBasestation (otherNode))
              continue;

            if (ShouldFailLink(node, otherNode, disasterLocation))
              {
                // fail both this end of the link and the other side!
                ifacesToKill.Add(ipv4, i);
                FailIpv4 (ipv4, i);

                // get the necessary info for the other end of the link
                Ptr<Ipv4> otherIpv4 = otherNetDevice->GetNode ()->GetObject<Ipv4> ();
                uint32_t otherIndex = otherIpv4->GetInterfaceForDevice (otherNetDevice);

                ifacesToKill.Add(otherIpv4, otherIndex);
                FailIpv4 (otherIpv4, otherIndex);
              }
          }
      }
    }
}


void
GeocronExperiment::UnapplyFailureModel () {
  NS_LOG_INFO ("Unapplying failure model");

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

  // If we've previously installed a RonServer Application on some node(s),
  // we should remove all instances so they won't conflict with new ones if
  // we happen to choose the same serverNodes twice in a simulation.
  if (serverNodes.GetN ())
  {
    for (NodeContainer::Iterator nodeItr = serverNodes.Begin ();
        nodeItr != serverNodes.End (); nodeItr++)
    {
      Ptr<Node> serverNode = *nodeItr;
      for (uint32_t i = 0; i < serverNode->GetNApplications (); i++)
      {
        Ptr<Application> app = serverNode->GetApplication (i);
        if (DynamicCast<RonServer> (app))
        {
          NS_LOG_LOGIC ("Removing server app from node " << serverNode->GetId ());
          Ptr<Application> removedApp = serverNode->RemoveApplication (i);
          NS_ASSERT_MSG (DynamicCast<RonServer> (removedApp), "Removed app was not a RonServer!");
          break;
        }
      }
    }
  }

  // Clear all of the previous round's server nodes since we pick new ones each time
  serverNodes = NodeContainer ();

  // Choose the server nodes at random from those available.
  // To do this, we'll put all the candidates into a vector, shuffle them up,
  // then pick up currNServers from them (or randomly from all the nodes if necessary).
  // If we try to just continually pull server nodes at random, we end up with repeats,
  // which results in the actual number of servers being less than it should have.
  std::vector<Ptr<Node> > theseServerChoices (serverNodeCandidates[currLocation].Begin (),
      serverNodeCandidates[currLocation].End ());
  std::random_shuffle (theseServerChoices.begin (), theseServerChoices.end ());

  Ptr<Node> nextServerNode;
  for (uint32_t i = 0; i < currNServers; i++)
  {
    if (theseServerChoices.size () > i)
    {
      nextServerNode = theseServerChoices[i];
    }
    else
    {
      uint32_t nodeIndex = random->GetInteger (0, nodes.GetN ());
      NS_LOG_DEBUG ("No servers in this region, so using node at random: " << nodeIndex);
      nextServerNode = nodes.Get (nodeIndex);
    }

    serverNodes.Add (nextServerNode);

    NS_LOG_INFO ("Server is Node " << nextServerNode->GetId () <<
          " at location " << nextServerNode->GetObject<RonPeerEntry> ()->region);
  }

  //Application
  RonServerHelper ronServer (9);

// Do some error checking when not optimized
#ifdef NS3_LOG_ENABLE
  for (NodeContainer::Iterator nodeItr = serverNodes.Begin ();
      nodeItr != serverNodes.End (); nodeItr++)
  {
    Ptr<Node> serverNode = *nodeItr;
    if (serverNode->GetNApplications ())
    {
      Ptr<RonClient> clientApp = DynamicCast<RonClient> (serverNode->GetApplication (0));
      if (clientApp)
        NS_ASSERT_MSG (true, "RonClient and RonServer on same node.  This is unsupported behavior!");
    }
  }
#endif

  serverApps = ronServer.Install (serverNodes);
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (appStopTime);

  // save all the servers so we can tell the clients where they are
  serverPeers = Create<RonPeerTable> ();
  serverPeers->AddPeers (serverNodes);
  NS_ASSERT_MSG (serverPeers->GetN () == currNServers,
      "Should have built a table with " << currNServers
      << " server peers but only have " << serverPeers->GetN ());

  allPeers->AddPeers (serverPeers);
#ifdef NS3_LOG_ENABLE
  for (RonPeerTable::Iterator itr = serverPeers->Begin ();
      itr != serverPeers->End (); itr++)
  {
    NS_LOG_DEBUG ("Caching server peer with ID " << (*itr)->id << " and IP address " << (*itr)->address);
  }
#endif
}


Ptr<RonPeerTable>
GeocronExperiment::GetPeerTableForPeer (Ptr<RonPeerEntry> peer)
{
#ifdef USE_FULL_PEER_TABLE
  return activeOverlayPeers;
#else
  // We want to return the same peer table for a given peer so that each
  // heuristic will have an identical table.  We regenerate the tables
  // each time we are back to the first number of servers as that indicates
  // a new run and new application of the failure model, hence a new round
  // of all the parameters that should be compared with the same table
  static std::vector<Ptr<RonPeerEntry> > peerVector (overlayPeers->Begin (), overlayPeers->End ());
  static std::map<uint32_t, Ptr<RonPeerTable> > cachedPeerTables;

  // Check if we should generate a new table
  // TODO: do this check in a less-hacky manner
  if (currNServers == nservers->at (0) or cachedPeerTables.count (peer->id) == 0)
  {
    // randomly shuffle the peerVector and choose c*log(n) peers to add to a new RonPeerTable
    std::random_shuffle (peerVector.begin (), peerVector.end ());
    Ptr<RonPeerTable> newTable = Create<RonPeerTable> ();

    uint32_t peersAdded = 0, maxPeers = peerTableSizeScalingFactor * std::log(overlayPeers->GetN ());
    NS_LOG_LOGIC ("Generating table of " << maxPeers << " peers for peer " << peer->id);

    for (; peersAdded < maxPeers; peersAdded++)
    {
      newTable->AddPeer (peerVector[peersAdded]);
    }

    // add one more if the random ones included the peer in question
    // TODO: remove the other peer? doesn't seem to hurt anything for now...
    if (newTable->IsInTable (peer))
      newTable->AddPeer (peerVector[peersAdded]);

    cachedPeerTables[peer->id] = newTable;
    return newTable;
  }
  return cachedPeerTables[peer->id];
#endif
}


void
GeocronExperiment::Run ()
{
  int numDisasterPeers = 0;

  // Set up the proper heuristics, peer tables, and calculate the
  // number of active overlay nodes that will report data.
  // NOTE: we used to only count peers in the disaster region as
  // reporting data, so we may need to TODO: fix this at a later date
  // if we go back to simulation wider regions than are affected by
  // the disaster.
  for (RonPeerTable::Iterator peerItr = activeOverlayPeers->Begin ();
      peerItr != activeOverlayPeers->End (); peerItr++)
    {
      Ptr<Node> node = (*peerItr)->node;
      // iterate over all applications just in case we add other apps later on...
      for (uint32_t i = 0; i < node->GetNApplications (); i++) {
        Ptr<RonClient> ronClient = DynamicCast<RonClient> (node->GetApplication (i));
        if (ronClient == NULL) {
          continue;
        }

        //add the heuristic and aggregate a weak random one to it to break ties
        //TODO: make a helper?
        //TODO: have the random be part of the object factory/attributes?
        //TODO: just define it as the input to the sim?
        Ptr<RonPathHeuristic> heuristic = currHeuristic->Create<RonPathHeuristic> ();
        NS_ASSERT_MSG (heuristic, "Created heuristic is NULL! Bad heuristic name?");
        // Must set heuristic first so that source will be set and heuristic can make its heap
        heuristic->MakeTopLevel (); //must always do this! TODO: not need to?
        heuristic->SetSourcePeer (node->GetObject<RonPeerEntry> ());

        ronClient->SetHeuristic (heuristic);
        ronClient->SetServerPeerTable (serverPeers);

        // determine the peers this client, and likewise the heuristic, will know about
        Ptr<RonPeerTable> table = GetPeerTableForPeer (node->GetObject<RonPeerEntry> ());
        ronClient->SetPeerTable (table);
        heuristic->SetPeerTable (table);

        // add the random heuristic
        //TODO: do this by TypeId and remove #include from beggining
        //Ptr<RonPathHeuristic> randHeuristic = CreateObject<RandomRonPathHeuristic> ();
        //TODO: get this working??? that's concerning maybe we should just turn this off...
        //randHeuristic->SetAttribute ("Weight", DoubleValue (0.01));
        //heuristic->AddHeuristic (randHeuristic);

        ronClient->SetAttribute ("MaxPackets", UintegerValue (contactAttempts));
        ronClient->SetAttribute ("MultipathFanout", UintegerValue (currNPaths));
        numDisasterPeers++;
      }
    }

  ConnectAppTraces ();

  // pointToPoint.EnablePcap("rocketfuel-example",router_devices.Get(0),true);

  NS_LOG_INFO ("------------------------------------------------------------------------");
  NS_LOG_UNCOND ("Starting simulation on map file " << topologyFile << ": " << std::endl
                 << nodes.GetN () << " total nodes" << std::endl
                 << activeOverlayPeers->GetN () << " total active overlay nodes" << std::endl
                 << serverPeers->GetN () << " total server(s)" << std::endl
                 << disasterNodes[currLocation].size () << " nodes in " << currLocation << " total" << std::endl
                 << numDisasterPeers << " active overlay nodes reporting data in " << currLocation << std::endl //TODO: get size from table
                 << std::endl << "Failure probability: " << currFprob << std::endl
                 << failNodes.GetN () << " nodes failed" << std::endl
                 << ifacesToKill.GetN () / 2 << " links failed\n");

  Simulator::Stop (simulationLength);
  Simulator::Run ();
  Simulator::Destroy ();

  NS_LOG_INFO ("Next simulation run...");
  NS_LOG_UNCOND ("========================================================================");

  // reset apps
  for (ApplicationContainer::Iterator app = clientApps.Begin ();
       app != clientApps.End (); app++)
    {
      Ptr<RonClient> ronClient = DynamicCast<RonClient> (*app);
      ronClient->Reset ();
    }
}
