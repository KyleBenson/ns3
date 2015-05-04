/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

// An essential include is test.h
#include "ns3/test.h"
#include "ns3/geocron-module.h"
#include "ns3/core-module.h"

#include "ns3/point-to-point-layout-module.h"

#include <vector>
#include <iostream>
#include <ctime>
#include <map>

// Do not put your test classes in namespace ns3.  You may find it useful
// to use the using directive to access the ns3 namespace directly
using namespace ns3;

typedef std::vector<Ptr<RonPeerEntry> > PeerContainer;


void PrintPeer (Ptr<RonPeerEntry> peer)
{
  std::cout << peer->region << " at " << peer->location  << ", ID: " << peer->id << std::endl;  
}

void PrintPeer (Ptr<PeerDestination> dest)
{
  Ptr<RonPeerEntry> peer = (*dest->Begin ());
  PrintPeer (peer);
}

void PrintPeer (Ptr<RonPath> path)
{
  Ptr<PeerDestination> dest = (*path->Begin ());
  PrintPeer (dest);
}

bool ArePeersEqual (Ptr<RonPeerEntry> peer1, Ptr<RonPeerEntry> peer2)
{
  bool equality = *peer1 == *peer2;
  return equality;
}

bool ArePeersEqual (Ptr<PeerDestination> dest1, Ptr<PeerDestination> dest2)
{
  Ptr<RonPeerEntry> peer1 = *dest1->Begin (), peer2 = *dest2->Begin ();
  bool equality = ArePeersEqual (peer1, peer2);
  return equality;
}

bool ArePeersEqual (Ptr<RonPath> path1, Ptr<RonPath> path2)
{
  Ptr<PeerDestination> dest1 = *path1->Begin (), dest2 = *path2->Begin ();
  bool equality = ArePeersEqual (dest1, dest2);
  return equality;
}

class GridGenerator
{
public:
  Ipv4AddressHelper rowHelper;
  Ipv4AddressHelper colHelper;

  PeerContainer cachedPeers;
  NodeContainer cachedNodes;

  Ptr<RonPeerTable> allPeers;

  PointToPointGridHelper * grid;

  // 2D array of peer entries
  // NOTE: how the four quadrant regions are defined, the higher numbered regions will have more peers.
  // i.e. a 5x5 grid will have TopLeft be (0,0) -> (1,1), BottomLeft will be (4,0) -> (2,1)
  std::vector<std::vector<Ptr<RonPeerEntry> > > gridPeers; 

  static GridGenerator * GetInstance ()
  {
    static GridGenerator * instance = new GridGenerator ();
    return instance;
  }
  
  static NodeContainer GetNodes ()
  {
    return GetInstance ()->cachedNodes;
  }

  static PeerContainer GetPeers ()
  {
    return GetInstance ()->cachedPeers;
  }

  static Ptr<Node> GetNode (uint32_t row, uint32_t col)
  {
    return GetInstance ()->grid->GetNode (row, col);
  }

  // really only exists for sanity checks in constructor
  Ptr<RonPeerEntry> DoGetPeer (uint32_t row, uint32_t col)
  {
    return gridPeers[row][col];
  }

  static Ptr<RonPeerEntry> GetPeer (uint32_t row, uint32_t col)
  {
    return GetInstance ()->DoGetPeer (row, col);
  }

  static Ptr<RonPeerTable> GetAllPeers ()
  {
    return GetInstance ()->allPeers;
  }

  GridGenerator ()
  {
    //for RandomRonPathHeuristic
    SeedManager::SetSeed(std::time (NULL));
    SeedManager::SetRun(123);

    rowHelper = Ipv4AddressHelper ("10.1.0.0", "255.255.0.0");
    colHelper = Ipv4AddressHelper ("10.128.0.0", "255.255.0.0");

   //first, build a topology for us to use
    uint32_t nrows = 5, ncols = 5;
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
    pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

    grid = new PointToPointGridHelper (nrows, ncols, pointToPoint);
    grid->BoundingBox (0, 0, nrows, ncols);

    // this routing configuration is taken straight from GeocronExperiment
    Ipv4NixVectorHelper nixRouting;
    nixRouting.SetAttribute("FollowDownEdges", BooleanValue (true));
    Ipv4StaticRoutingHelper staticRouting;
    Ipv4ListRoutingHelper routingList;
    routingList.Add (staticRouting, 0);
    routingList.Add (nixRouting, 10);

    InternetStackHelper internet;
    internet.SetRoutingHelper (routingList);
    grid->InstallStack (internet);
    grid->AssignIpv4Addresses (rowHelper, colHelper);
    rowHelper.NewNetwork ();
    colHelper.NewNetwork ();

    cachedNodes.Add (grid->GetNode (0,0));
    cachedNodes.Add (grid->GetNode (1,1));
    cachedNodes.Add (grid->GetNode (4,1)); //2nd choice for this V
    cachedNodes.Add (grid->GetNode (0,4)); //should be best ortho path for 0,0 --> 4,4
    cachedNodes.Add (grid->GetNode (3,3));
    cachedNodes.Add (grid->GetNode (4,0)); //should be best ortho path for 0,0 --> 4,4
    cachedNodes.Add (grid->GetNode (4,4));
    //cachedNodes.Add (grid->GetNode (3,0));

    NS_ASSERT_MSG (cachedNodes.Get (0)->GetId () != cachedNodes.Get (2)->GetId (), "nodes shouldn't ever have same ID!");

    for (NodeContainer::Iterator itr = cachedNodes.Begin (); itr != cachedNodes.End (); itr++)
    {
      Ptr<RonPeerEntry> newPeer = Create<RonPeerEntry> (*itr);

      //newPeer->id = id++;
      cachedPeers.push_back (newPeer);
      }

    //set regions for testing those types of heuristics
    cachedPeers[0]->region = "TL";
    cachedPeers[1]->region = "TL";
    cachedPeers[2]->region = "BL";
    cachedPeers[3]->region = "TR";
    cachedPeers[4]->region = "BR";
    cachedPeers[5]->region = "BL";
    cachedPeers[6]->region = "BR";
    //cachedPeers[7]->region = "BL";

    //add all cached peers to master peer table
    Ptr<RonPeerTable> master = RonPeerTable::GetMaster ();

    for (PeerContainer::iterator itr = cachedPeers.begin ();
         itr != cachedPeers.end (); itr++)
      {
        master->AddPeer (*itr);
      }

    ////////////////////////////////////////////////////////////
    ////////////////////  NEW STYLE PEER ACCESS  ///////////////
    ////////////////////////////////////////////////////////////

    allPeers = Create<RonPeerTable> ();

    // // save info about ALL the peers
    for (uint32_t i = 0; i < nrows; i++)
      {
        std::vector<Ptr<RonPeerEntry> > vec;
        gridPeers.push_back (vec);

        for (uint32_t j = 0; j < nrows; j++)
          {
            Ptr<RonPeerEntry> thisPeer = Create<RonPeerEntry> (grid->GetNode (i, j));

            // build up the region based on the location of the current node being considered
            std::string regionLabel;
            regionLabel += (i < nrows/2 ? "T" : "B");
            regionLabel += (j < ncols/2 ? "L" : "R");

            Location newRegion = regionLabel;
            thisPeer->region = newRegion;

            gridPeers[i].push_back (thisPeer);

            allPeers->AddPeer (thisPeer);
          }
      }

    // some sanity checks that this craziness ^^^ worked
    NS_ASSERT_MSG (DoGetPeer (0,0)->region == cachedPeers[0]->region,
                   "cachedPeers' region doesn't match that of peers from grid!");
    NS_ASSERT_MSG (DoGetPeer (1,1)->region == cachedPeers[1]->region,
                   "cachedPeers' region doesn't match that of peers from grid!");
    NS_ASSERT_MSG (DoGetPeer (4,1)->region == cachedPeers[2]->region,
                   "cachedPeers' region doesn't match that of peers from grid!");
    NS_ASSERT_MSG (DoGetPeer (0,4)->region == cachedPeers[3]->region,
                   "cachedPeers' region doesn't match that of peers from grid!");
    NS_ASSERT_MSG (DoGetPeer (3,3)->region == cachedPeers[4]->region,
                   "cachedPeers' region doesn't match that of peers from grid!");
    NS_ASSERT_MSG (DoGetPeer (4,0)->region == cachedPeers[5]->region,
                   "cachedPeers' region doesn't match that of peers from grid!");
    NS_ASSERT_MSG (DoGetPeer (4,4)->region == cachedPeers[6]->region,
                   "cachedPeers' region doesn't match that of peers from grid!");
  }

  ~GridGenerator ()
  {
    //delete grid; // causes seg faults since we dont always use object pointers...
  }
};

////////////////////////////////////////////////////////////////////////////////
class TestPeerEntry : public TestCase
{
public:
  TestPeerEntry ();
  virtual ~TestPeerEntry ();
  NodeContainer nodes;
  PeerContainer peers;

private:
  virtual void DoRun (void);
};

TestPeerEntry::TestPeerEntry ()
  : TestCase ("Tests basic operations of RonPeerEntry objects")
{
  nodes = GridGenerator::GetNodes ();
  peers = GridGenerator::GetPeers ();
}

TestPeerEntry::~TestPeerEntry ()
{
}

void
TestPeerEntry::DoRun (void)
{
  //NS_TEST_ASSERT_MSG_EQ (false, true, "got here");

  //Test equality operator
  bool equality = (*peers[0] == *peers[0]);
  NS_TEST_ASSERT_MSG_EQ (equality, true, "equality test same ptr failed");
  
  equality = (*peers[3] == *peers[3]);
  NS_TEST_ASSERT_MSG_EQ (equality, true, "equality test same ptr failed");

  equality = (*peers[2] == *(Create<RonPeerEntry> (nodes.Get (2))));
  NS_TEST_ASSERT_MSG_EQ (equality, true, "equality test new ptr failed");
  
  equality = (*peers[3] == *peers[1]);
  NS_TEST_ASSERT_MSG_NE (equality, true, "equality test false positive");

  equality = (peers[3]->address == peers[3]->address);
  NS_TEST_ASSERT_MSG_EQ (equality, true, "equality test address");

  equality = (peers[1]->address == (Ipv4Address)(uint32_t)0);
  NS_TEST_ASSERT_MSG_EQ (equality, false, "equality test address not 0");

  //Test < operator
  equality = *peers[0] < *peers[4];
  NS_TEST_ASSERT_MSG_EQ (equality, true, "< operator test");

  equality = *peers[0] < *peers[0];
  NS_TEST_ASSERT_MSG_EQ (equality, false, "< operator test with self");

  equality = *peers[4] < *peers[1];
  NS_TEST_ASSERT_MSG_EQ (equality, false, "< operator test");
}


////////////////////////////////////////////////////////////////////////////////
class TestPeerTable : public TestCase
{
public:
  TestPeerTable ();
  virtual ~TestPeerTable ();
  NodeContainer nodes;
  PeerContainer peers;

private:
  virtual void DoRun (void);
};


TestPeerTable::TestPeerTable ()
  : TestCase ("Test basic operations of PeerTable objects")
{
  nodes = GridGenerator::GetNodes ();
  peers = GridGenerator::GetPeers ();
}

TestPeerTable::~TestPeerTable ()
{
}

void
TestPeerTable::DoRun (void)
{
  //Ptr<RonPeerTable> table = RonPeerTable::GetMaster ();
  Ptr<RonPeerTable> table = Create<RonPeerTable> ();
  Ptr<RonPeerTable> table0 = Create<RonPeerTable> ();

  Ptr<RonPeerEntry> peer0 = peers[0];
  table->AddPeer (peer0);
  table->AddPeer (peers[1]);
  table->AddPeer (nodes.Get (3));

  bool equality = *peers[3] == *table->GetPeer (nodes.Get (3)->GetId ());
  NS_TEST_ASSERT_MSG_EQ (equality, true, "getting peer (added as node) by id");

  equality = 3 == table->GetN ();
  NS_TEST_ASSERT_MSG_EQ (equality, true, "checking table size after adding");

  NS_TEST_ASSERT_MSG_EQ (table->IsInTable (peers[3]->id), true, "testing IsInTable");
  NS_TEST_ASSERT_MSG_EQ (table->IsInTable (Create<RonPeerEntry> (nodes.Get (3))->id), true, "testing IsInTable fresh copy by node");
  NS_TEST_ASSERT_MSG_EQ (table->IsInTable (peers[2]->id), false, "testing IsInTable false positive");
  //TODO: test by peer entry ptr ref
  //NS_TEST_ASSERT_MSG_EQ (table->IsInTable (peers[2]), false, "testing IsInTable false positive");

  equality = *peers[3] == *table->GetPeer (nodes.Get (3)->GetId ());
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing GetPeer");

  equality = *table->GetPeerByAddress (peer0->address) == *peer0;
  NS_TEST_ASSERT_MSG_EQ (equality, true, "getting peer by address");

  equality = *table->GetPeerByAddress (peer0->address) == *peers[1];
  NS_TEST_ASSERT_MSG_EQ (equality, false, "getting peer by address, testing false positive");

  equality = NULL == table->GetPeer (nodes.Get (4)->GetId ());
  NS_TEST_ASSERT_MSG_EQ (equality, true, "table should return NULL when peer not present");

  //test multiple tables with fresh peer entries
  table0->AddPeer (nodes.Get (0));
  table0->AddPeer (nodes.Get (1));

  equality = *table == *table;
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing self-equality");

  //add all entries to new table and make sure its == master
  Ptr<RonPeerTable> newTable = Create<RonPeerTable> ();
  Ptr<RonPeerTable> master = RonPeerTable::GetMaster ();
  for (RonPeerTable::Iterator itr = master->Begin ();
       itr != master->End (); itr++)
    {
      newTable->AddPeer (*itr);
    }

  equality = *newTable == *master;
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing self-equality GetMaster");

  equality = *table0 == *table;
  NS_TEST_ASSERT_MSG_EQ (equality, false, "testing RonPeerTable equality false positive");

  table0->AddPeer (nodes.Get (3));
  equality = *table0 == *table;
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing RonPeerTable equality");

  table->RemovePeer (nodes.Get (3)->GetId ());
  equality = *table0 == *table;
  NS_TEST_ASSERT_MSG_EQ (equality, false, "testing RonPeerTable false positive equality after removal");

  table0->RemovePeer (nodes.Get (3)->GetId ());
  equality = *table0 == *table;
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing RonPeerTable false positive equality after removal");

  //test iterators
  NS_TEST_ASSERT_MSG_EQ (table->IsInTable (table0->Begin ()), true, "testing IsInTable with Iterator");
  
  table->RemovePeer ((*table->Begin ())->id);
  NS_TEST_ASSERT_MSG_EQ (table->IsInTable (table0->Begin ()), false, "testing IsInTable with Iterator false positive after removal");


  /*TODO: test removal
  equality = table->RemovePeer (nodes.Get (4)->GetId ());
  NS_TEST_ASSERT_MSG_NE (equality, true, "remove should only return true if peer existed");

  equality = table->RemovePeer (nodes.Get (3)->GetId ());
  NS_TEST_ASSERT_MSG_EQ (equality, true, "remove should only return true if peer existed");*/
}


////////////////////////////////////////////////////////////////////////////////
class TestRonPath : public TestCase
{
public:
  TestRonPath ();
  virtual ~TestRonPath ();
  NodeContainer nodes;
  PeerContainer peers;

private:
  virtual void DoRun (void);
};


TestRonPath::TestRonPath ()
  : TestCase ("Test basic operations of RonPath objects")
{
  nodes = GridGenerator::GetNodes ();
  peers = GridGenerator::GetPeers ();
}

TestRonPath::~TestRonPath ()
{
}

void
TestRonPath::DoRun (void)
{
  Ptr<PeerDestination> start = Create<PeerDestination> (peers[0]);
  Ptr<PeerDestination> end = Create<PeerDestination> (peers.back ());

  ////////////////////////////// TEST PeerDestination  ////////////////////

  bool equality = *end == *end;
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing PeerDestination equality same obj");

  equality = *start == *end;
  NS_TEST_ASSERT_MSG_EQ (equality, false, "testing PeerDestination equality false positive");
  
  equality = *start == *Create<PeerDestination> (peers[3]);
  NS_TEST_ASSERT_MSG_EQ (equality, false, "testing PeerDestination equality false positive new copy");
  
  equality = *start == *Create<PeerDestination> (peers.front());
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing PeerDestination equality new copy");
  
  equality = *start == *Create<PeerDestination> (Create<RonPeerEntry> (nodes.Get (0)));
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing PeerDestination equality new peer");

  equality = *(*start->Begin ()) == (*(*Create<PeerDestination> (Create<RonPeerEntry> (nodes.Get (0)))->Begin ()));
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing PeerDestination Iterator equality new copy");

  equality = *(*end->Begin ()) == (*(*Create<PeerDestination> (peers[0])->Begin ()));
  NS_TEST_ASSERT_MSG_EQ (equality, false, "testing PeerDestination Iterator false positive equality new copy");

  equality = *(*start->Begin ()) == (*(*Create<PeerDestination> (peers[0])->Begin ()));
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing PeerDestination Iterator equality new copy");

  NS_TEST_ASSERT_MSG_EQ (start->GetN (), 1, "testing PeerDestination.GetN");

  //TODO: multi-peer destinations, which requires some ordering or comparing


  ////////////////////////////// TEST PATH  ////////////////////

  Ptr<RonPath> path0 = Create<RonPath> (start);
  Ptr<RonPath> path1 = Create<RonPath> (end);
  Ptr<RonPath> path2 = Create<RonPath> (peers[3]);
  Ptr<RonPath> path3 = Create<RonPath> (Create<RonPeerEntry> (nodes.Get (0)));
  Ptr<RonPath> path4 = Create<RonPath> (peers.back());

  equality = *path0 == *path0;
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing RonPath equality with self");

  equality = *path0->GetDestination () == *(*path0->Begin ());
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing RonPath equality GetDestination and Begin with one-hop path");  

  equality = *(Create<RonPath> (start))->GetDestination () == *(*path0->Begin ());
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing RonPath equality GetDestination and Begin with one-hop path, fresh copy");  

  equality = *path0 == *Create<RonPath> (start);
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing RonPath equality with fresh path, same init");

  equality = *path0 == *Create<RonPath> (peers[0]);
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing RonPath equality with fresh path, same init re-retrieved");

  equality = *path4 == *path1;
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing RonPath equality with fresh path, one made by peer");

  equality = *path0 == *path3;
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing RonPath equality with fresh path, also fresh PeerDestination and RonPeerEntry");

  //try accessing elements and testing for equality

  equality = *path0->GetDestination () == *path0->GetDestination ();
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing for consistent self equality of destination");

  equality = *path0->GetDestination () == *path1->GetDestination ();
  NS_TEST_ASSERT_MSG_EQ (equality, false, "testing for destination false positive");

  equality = *start == *(*path0->Begin ());
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing equality with Begin and PeerDestination");

  equality = *(Create<PeerDestination> (peers.back ())) == *(*path1->Begin ());
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing equality with Begin and fresh PeerDestination");

  equality = *path0 == *path1;
  NS_TEST_ASSERT_MSG_EQ (equality, false, "testing false positive equality on separately built paths (start,end)");

  //now add a hop
  path0->AddHop (end);

  Ptr<PeerDestination> oldBegin = (*(path0->Begin ()));
  Ptr<PeerDestination> newBegin, newEnd, oldEnd;

  equality = *oldBegin == *(*path0->Begin ());
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing for consistent path start after adding a PeerDestination");

  equality = *path0->GetDestination () == *path0->GetDestination ();
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing for consistent self equality of destination after adding");

  equality = *path0->GetDestination () == *path1->GetDestination ();
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing for GetDestination equality of two paths after adding to one");

  equality = *(path1->GetDestination ()) == *(Create<RonPath> (end)->GetDestination ());
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing same dest fresh copy before adding");

  equality = *(*path0->Begin ()) == *(*Create<RonPath> (start)->Begin ());
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing same start fresh copy after adding");

  Ptr<RonPath> tempPath = Create<RonPath> (start);
  tempPath->AddHop (end);
  equality = *(path0->GetDestination ()) == *(tempPath->GetDestination ());
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing same dest fresh copy after adding to both");

  equality = 2 == path0->GetN ();
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing for consistent path size after adding a PeerDestination");

  equality = 1 == path1->GetN ();
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing for consistent path size of other path");

  oldBegin = *(path1->Begin ());
  newBegin = (path0->GetDestination ());

  equality = (*newBegin == *oldBegin);
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing for PeerDestination in one path after added from another using GetDestination");

  equality = (*newBegin == *path1->GetDestination ());
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing for PeerDestination in one path after added from another using GetDestination");

  equality = (*path0 == *Create<RonPath> (start));
  NS_TEST_ASSERT_MSG_EQ (equality, false, "testing for PeerDestination false positive equality after adding PeerDestination");

  equality = (*path0 == *Create<RonPath> (Create<PeerDestination> (Create<RonPeerEntry> (nodes.Get (0)))));
  NS_TEST_ASSERT_MSG_EQ (equality, false, "testing for PeerDestination false positive equality after adding PeerDestination fresh PeerDestination");

  equality = (*path0 == *Create<RonPath> (Create<PeerDestination> (peers.back ())));
  NS_TEST_ASSERT_MSG_EQ (equality, false, "testing for PeerDestination false positive equality after adding PeerDestination fresh PeerDestination, checking back");

  oldBegin = *(path1->Begin ());
  RonPath::Iterator pathItr1 = path0->Begin ();
  pathItr1++;
  newBegin = *(pathItr1);

  equality = (*newBegin == *oldBegin);
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing for PeerDestination in one path after added from another using Iterator");

  //paths 0 and 1 should be equal now
  oldEnd = path1->GetDestination ();
  path1->AddHop (start, path1->Begin ());

  equality = *start == *(*path1->Begin ());
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing expected start after inserting to front of path");

  equality = *oldEnd == *path1->GetDestination ();
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing destination after inserting to front of path");

  equality = *path0 == *path0;
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing same path now that it's same as another");

  equality = *(path1->GetDestination ()) == *(Create<RonPath> (end)->GetDestination ());
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing same dest fresh copy");

  equality = *path1->GetDestination () == *Create<RonPath> (peers.back ())->GetDestination ();
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing same dest deeper fresh copy");

  pathItr1 = path0->Begin ();
  RonPath::Iterator pathItr2 = path1->Begin ();
  equality = *(*(pathItr1)) == *(*(pathItr2));
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing begin iterator equality separately built paths (start,end)");

  equality = *path1->GetDestination () == *path0->GetDestination ();
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing same destination now that paths are same");

  equality = *(*(pathItr1)) == *(*(path2->Begin ()));
  NS_TEST_ASSERT_MSG_EQ (equality, false, "testing iterator equality false positive separately built paths (start,end)");

  pathItr1++;
  pathItr2++;
  equality = *(*(pathItr1)) == *(*(pathItr2));
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing iterator equality after incrementing separately built paths (start,end)");

  equality = *path0 == *path1;
  NS_TEST_ASSERT_MSG_EQ (equality, true, "separately built paths (start,end)");

  //check larger paths
  path1->AddHop (peers[3]);
  
  equality = *path0 == *path1;
  NS_TEST_ASSERT_MSG_EQ (equality, false, "testing proper equality for separately built up path subsets");

  //time to test reverse
  path2->AddHop (end);
  path2->AddHop (start);
  path2->Reverse ();

  equality = *path1 == *path2;
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing proper equality for reverse on separately built paths");

  //TODO: check reversing with complex paths
  //TODO: catting paths together
}

////////////////////////////////////////////////////////////////////////////////
class TestRonPathHeuristic : public TestCase
{
public:
  TestRonPathHeuristic ();
  virtual ~TestRonPathHeuristic ();
  NodeContainer nodes;
  PeerContainer peers;

private:
  virtual void DoRun (void);
};


TestRonPathHeuristic::TestRonPathHeuristic ()
  : TestCase ("Test basic operations of RonPathHeuristic objects, except for aggregation")
{
  nodes = GridGenerator::GetNodes ();
  peers = GridGenerator::GetPeers ();
}

TestRonPathHeuristic::~TestRonPathHeuristic ()
{
}

void
TestRonPathHeuristic::DoRun (void)
{
  //RonPathHeuristic has friended us so we can test internals.
  Ptr<RonPathHeuristic> h0 = CreateObject<RonPathHeuristic> ();
  Ptr<PeerDestination> dest = Create<PeerDestination> (peers.back());
  Ptr<RonPath> path = Create<RonPath> (dest);
  path->AddHop (peers[2], path->Begin ());

  h0->SetPeerTable (RonPeerTable::GetMaster ());
  h0->SetSourcePeer (peers.front());

  bool equality = *peers.front () == *h0->GetSourcePeer ();
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing GetSourcePeer after setting it");

  //it must be top-level to build data structures properly
  h0->MakeTopLevel ();

  //we need to call GetBestPath before anything else as it will build all the available paths
  h0->BuildPaths (dest);
  NS_TEST_ASSERT_MSG_NE (h0->m_masterLikelihoods->at (dest).size (), 0,
                         "inner master likelihood table empty after BuildPaths");

  NS_TEST_ASSERT_MSG_EQ (h0->m_masterLikelihoods->at (dest).size (), peers.size () - 2,
                         "testing size of inner master likelihood table after BuildPaths");

  NS_TEST_ASSERT_MSG_EQ (h0->m_likelihoods.at (dest).size (), peers.size () - 2,
                         "testing size of inner likelihood table after BuildPaths");

  NS_TEST_ASSERT_MSG_EQ ((*h0->m_masterLikelihoods)[dest][path]->GetN (), 1,
                         "testing size of Likelihood object in m_masterLikelihoods");

  NS_TEST_ASSERT_MSG_EQ (h0->PathAttempted (path), false, "testing PathAttempted before GetBestPath");

  //update LHs so that they won't be updated again to overwrite this change in GetBestPath
  h0->UpdateLikelihoods (dest);

  //set this path's LH to 2 and we should get it back
  h0->SetLikelihood (path, 2.0);

  NS_TEST_ASSERT_MSG_EQ_TOL ((*h0->m_masterLikelihoods)[dest][path]->GetLh (), 2.0, 0.001,
                             "m_masterLikelihoods was not set properly by SetLikelihood");
  NS_TEST_ASSERT_MSG_EQ_TOL (h0->m_likelihoods[dest][path]->GetLh (), 2.0, 0.001,
                             "m_likelihoods was not set properly by SetLikelihood");

  Ptr<RonPath> path2 = h0->GetBestPath (dest);

  NS_TEST_ASSERT_MSG_EQ_TOL (h0->m_likelihoods[dest][path]->GetLh (), 2.0, 0.001,
                             "m_likelihoods should not be changed by GetBestPath");
  NS_TEST_ASSERT_MSG_EQ_TOL ((*h0->m_masterLikelihoods)[dest][path]->GetLh (), 2.0, 0.001,
                             "m_masterLikelihoods should not be changed by GetBestPath");

  equality = *path == *path2;
  NS_TEST_ASSERT_MSG_EQ (equality, true, "should have gotten back highest likelihood path we just set, but instead got LH=" << h0->m_likelihoods[dest][path2]->GetLh ());

  NS_TEST_ASSERT_MSG_EQ (h0->PathAttempted (path), true, "testing PathAttempted after GetBestPath");

  equality = *path->GetDestination () == *dest;
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing GetBestPath with basic path");

  path = Create<RonPath> (dest);
  NS_TEST_ASSERT_MSG_EQ_TOL (h0->GetLikelihood (path), 1.0, 0.01, "testing GetLikelihood before any feedback");

  h0->NotifyTimeout (path, Simulator::Now ());
  NS_TEST_ASSERT_MSG_EQ_TOL (h0->m_likelihoods[dest][path]->GetLh (), 0.0, 0.01, "testing m_likelihoods after timeout feedback");

  NS_TEST_ASSERT_MSG_EQ_TOL ((*h0->m_masterLikelihoods)[dest][path]->GetLh (), 0.0, 0.01, "testing m_masterLikelihoods after timeout feedback");

  h0->NotifyAck (path, Simulator::Now ());
  NS_TEST_ASSERT_MSG_EQ_TOL (h0->m_likelihoods[dest][path]->GetLh (), 1.0, 0.01, "testing m_likelihoods after ACK feedback");

  ////////////////////////////////////////////////////////////////////////////////
  //we check to make sure that we actually get some peers from the heuristic
  //h0 = CreateObject<RandomRonPathHeuristic> ();
  h0 = CreateObject<RonPathHeuristic> ();
  h0->SetPeerTable (RonPeerTable::GetMaster ());
  h0->SetSourcePeer (peers.front());
  h0->MakeTopLevel ();
  h0->BuildPaths (dest);
  h0->UpdateLikelihoods (dest);

  uint32_t npaths = 0, nExpectedPeers = GridGenerator::GetNodes ().GetN ()- 2;
  Ptr<RonPath> lastPath = NULL;
  double totalLh = 0;
  try
    {
      while (true)
        {
          path = h0->GetBestPath (dest);

          //assert that none of the paths are exactly LH 1 since we're also using h0
          totalLh = 0;
          totalLh += (*h0->m_masterLikelihoods)[dest][path]->GetLh ();

          //totalLh += h0->m_likelihoods[dest][path]->GetLh ();

          NS_TEST_ASSERT_MSG_GT (totalLh, 0.99, /*0.01,*/ "likelihood should be 1");

          //we also want to assert that we get a different path
          equality = (lastPath != NULL) and (*path == *lastPath);
          NS_TEST_ASSERT_MSG_EQ (equality, false, "got same path as last time");
          lastPath = path;
          h0->NotifyTimeout (lastPath, Simulator::Now ());

          //as well as that we got the number of expected possible paths
          npaths++;
          //TODO: something like this in TestRonPathHeuristic to make sure src and dest aren't chosen
        }
    }
  catch (RonPathHeuristic::NoValidPeerException e)
    {
      NS_TEST_ASSERT_MSG_EQ (npaths, nExpectedPeers, npaths << " peers were checked for same region rather than " <<nExpectedPeers);
    }

  /////////////  Test multiple destinations ////////////////

  // reset heuristic's likelihoods first
  h0->ForceUpdateLikelihoods (dest);
  RonPathHeuristic::RonPathContainer paths = h0->GetBestMultiPath (dest, 2);
  NS_TEST_ASSERT_MSG_EQ (paths.size (), 2, "should have gotten 2 paths for multipath!");
  equality = *paths[0] == *paths[1];
  NS_TEST_ASSERT_MSG_NE (equality, true, "the two paths from GetBestMultiPath should not be the same!");

  // verify that getting another round of paths doesn't return duplicate either
  RonPathHeuristic::RonPathContainer nextPaths = h0->GetBestMultiPath (dest, 1);
  equality = *paths[0] == *nextPaths[0];
  NS_TEST_ASSERT_MSG_NE (equality, true, "second round of paths from GetBestMultiPath should have the same peers!");
  equality = *paths[1] == *nextPaths[0];
  NS_TEST_ASSERT_MSG_NE (equality, true, "second round of paths from GetBestMultiPath should have the same peers!");

  // verify that requesting too many paths throws an exception
  try
  {
    h0->GetBestMultiPath (dest, 999999999);
    NS_TEST_ASSERT_MSG_EQ (true, false, "requesting too many paths from GetBestMultiPath should throw exception!");
  }
  catch (RonPathHeuristic::NoValidPeerException e)
    {}

  ////////////////////////////////////////////////////////////////////////////////
  ////////////////////  dangerous: testing the inner structs /////////////////////
  ////////////////////////////////////////////////////////////////////////////////
 

  RonPathHeuristic::PathHasher pathHasher;
  path = Create<RonPath> (Create<PeerDestination> (Create<RonPeerEntry> (nodes.Get (nodes.GetN () - 1))));
  NS_TEST_ASSERT_MSG_EQ (pathHasher (path),
                         pathHasher (Create<RonPath> (Create<PeerDestination> (Create<RonPeerEntry> (nodes.Get (nodes.GetN () - 1))))),
                         "testing PathHasher new copy");

  NS_TEST_ASSERT_MSG_NE (pathHasher (path),
                         pathHasher (Create<RonPath> (Create<PeerDestination> (Create<RonPeerEntry> (nodes.Get (0))))),
                         "testing PathHasher false positive new copy");

  RonPathHeuristic::MasterPathLikelihoodInnerTable * masterInner = &(*h0->m_masterLikelihoods)[dest];
  
  NS_ASSERT_MSG (masterInner == &(*h0->m_masterLikelihoods)[dest], "why aren't the inner tables in the same location?");

  Ptr<RonPathHeuristic::Likelihood> lh = Create<RonPathHeuristic::Likelihood> ();
  (*masterInner)[path] = lh;
  lh->AddLh (0.5);

  NS_TEST_ASSERT_MSG_EQ_TOL ((*h0->m_masterLikelihoods)[dest][path]->GetLh (), 0.5,
                             0.01, "testing m_masterLikelihoods accessing with same keys");

  NS_TEST_ASSERT_MSG_EQ_TOL ((*h0->m_masterLikelihoods)[Create<PeerDestination> (peers.back())][path]->GetLh (), 0.5,
                             0.01, "testing m_masterLikelihoods accessing with fresh copies of keys");
}


////////////////////////////////////////////////////////////////////////////////
class TestAggregateRonPathHeuristic : public TestCase
{
public:
  TestAggregateRonPathHeuristic ();
  virtual ~TestAggregateRonPathHeuristic ();
  NodeContainer nodes;
  PeerContainer peers;

private:
  virtual void DoRun (void);
};


TestAggregateRonPathHeuristic::TestAggregateRonPathHeuristic ()
  : TestCase ("Test aggregation feature of RonPathHeuristic objects, using NewRegion and Random")
{
  nodes = GridGenerator::GetNodes ();
  peers = GridGenerator::GetPeers ();
}

TestAggregateRonPathHeuristic::~TestAggregateRonPathHeuristic ()
{
}

void
TestAggregateRonPathHeuristic::DoRun (void)
{
  bool equality;
  Ptr<PeerDestination> dest = Create<PeerDestination> (peers.back()),
    src = Create<PeerDestination> (peers.front()),
    topRight = Create<PeerDestination> (peers[3]),
    botLeft = Create<PeerDestination> (peers[5]);
  Ptr<RonPath> path = Create<RonPath> (dest);
  path->AddHop (botLeft, path->Begin ());

  Ptr<RandomRonPathHeuristic> random = CreateObject<RandomRonPathHeuristic> ();
  Ptr<NewRegionRonPathHeuristic> newreg = CreateObject<NewRegionRonPathHeuristic> ();
  
  random->SetPeerTable (RonPeerTable::GetMaster ());
  random->SetSourcePeer (peers.front());
  random->MakeTopLevel ();
  random->AddHeuristic (newreg);

  //do this so we can check some internal data structs
  random->BuildPaths (dest);
  random->UpdateLikelihoods (dest);

  NS_TEST_ASSERT_MSG_EQ ((*random->m_masterLikelihoods)[dest][path]->GetN (), 2,
                         "aggregate heuristic should have two entries in m_masterLikelihoods Likelihood");
  equality = NULL != random->m_likelihoods[dest][path];
  NS_TEST_ASSERT_MSG_EQ (equality, true,
                         "aggregate heuristic should have non-null entry in m_likelihoods");
  //TODO: look at the m_masterLikelihoods

  //If we tell the heuristics that some region failed to work, we should never see any
  //peers from that region since we only have one there.
  //path goes thru "BL"

  //we want to check with a different path to make sure that it affected both peers

  //as random is top-level, its m_likelihoods Likelihood aggregates all the others
  double oldLh = random->m_likelihoods[dest][path]->GetLh ();
  NS_TEST_ASSERT_MSG_GT (oldLh, 1.0, "checking range of random's assigned LH, though this could theoretically happen once in a blue moon?");
  NS_TEST_ASSERT_MSG_LT (oldLh, 2.0, "checking range of random's assigned LH");
  
  NS_TEST_ASSERT_MSG_EQ_TOL (newreg->m_likelihoods[dest][path]->GetLh (), 1.0, 0.01,
                             "m_likelihoods should be 1.0 for new region");
  
  //get master likelihood and verify before and after timeout
  double totalLh = 0;
  Ptr<RonPathHeuristic::Likelihood> lh = (*random->m_masterLikelihoods)[dest][path];

  NS_TEST_ASSERT_MSG_EQ_TOL (lh->GetLh (), oldLh, 0.01, "testing master LH before NotifyTimeout");

  // TIMEOUT
  random->NotifyTimeout (path, Simulator::Now ());

  NS_TEST_ASSERT_MSG_EQ_TOL (newreg->m_likelihoods[dest][path]->GetLh (), 0.0, 0.01,
                             "m_likelihoods should now be 0.0 for target region");

  NS_TEST_ASSERT_MSG_EQ (random->m_likelihoods[dest][path]->GetLh (), 0,
                             "random's LH, which is whole aggregate's, should be 0 after NotifyTimeout");

  NS_TEST_ASSERT_MSG_EQ (lh->GetLh (), 0, "testing master LH after NotifyTimeout");

  uint32_t npaths = 0, nExpectedPeers = GridGenerator::GetNodes ().GetN () - 3;
  Ptr<RonPath> lastPath = path;
  try
    {
      while (true)
        {
          path = random->GetBestPath (dest);

          //make sure we get valid paths
          equality = *(*path->Begin ()) == *src;
          NS_TEST_ASSERT_MSG_NE (equality, true, "next path contains source peer!");

          equality = *(*path->Begin ()) == *dest;
          NS_TEST_ASSERT_MSG_NE (equality, true, "next path contains destination peer as first hop!");

          //check that NewRegion worked properly
          if ((*(*path->Begin ())->Begin ())->region == "BL")
            {
              NS_TEST_ASSERT_MSG_EQ (newreg->m_likelihoods[dest][path]->GetLh (), 0.0,
                                     "we got a path using the NotifyTimeout region and LH != 0");
              NS_TEST_ASSERT_MSG_LT ((*newreg->m_masterLikelihoods)[dest][path]->GetLh (), 1.0,
                                     "we got a path using the NotifyTimeout region and master LH >= 1.0");
            }

          //assert that none of the paths are exactly LH 1 since we're also using random
          totalLh = (*random->m_masterLikelihoods)[dest][path]->GetLh ();
          double regLh = (newreg->m_likelihoods)[dest][path]->GetLh ();

          NS_TEST_ASSERT_MSG_GT (totalLh, 0.0 + regLh, /*0.01,*/ "likelihood should be an aggregate and features random");
          NS_TEST_ASSERT_MSG_LT (totalLh, 1.0 + regLh, /*0.01,*/ "likelihood should be < 2 as 2 heuristics");

          //we also want to assert that we get a different path
          equality = *path == *lastPath;
          NS_TEST_ASSERT_MSG_EQ (equality, false, "got same path as last time");
          lastPath = path;

          //as well as that we got the number of expected possible paths
          npaths++;

          random->NotifyTimeout (path, Simulator::Now ()); //so we don't get duplicates
          //TODO: something like this in TestRonPathHeuristic to make sure src and dest aren't chosen
        }
    }
  catch (RonPathHeuristic::NoValidPeerException e)
    {
      NS_TEST_ASSERT_MSG_EQ (npaths, nExpectedPeers, npaths << " peers were checked for same region rather than " << nExpectedPeers);
    }
}


////////////////////////////////////////////////////////////////////////////////

class TestOrthogonalRonPathHeuristic : public TestCase
{
public:
  TestOrthogonalRonPathHeuristic ();
  virtual ~TestOrthogonalRonPathHeuristic ();
  NodeContainer nodes;
  PeerContainer peers;

private:
  virtual void DoRun (void);
};


TestOrthogonalRonPathHeuristic::TestOrthogonalRonPathHeuristic ()
  : TestCase ("Test OrthogonalRonPathHeuristic")
{
  nodes = GridGenerator::GetNodes ();
  peers = GridGenerator::GetPeers ();
}

TestOrthogonalRonPathHeuristic::~TestOrthogonalRonPathHeuristic ()
{
}

void
TestOrthogonalRonPathHeuristic::DoRun (void)
{
  bool equality;
  //Ptr<PeerDestination> dest = Create<PeerDestination> (CreateObject<RonPeerEntry> (GridGenerator::GetNode (4, 4))),
  Ptr<PeerDestination> dest = Create<PeerDestination> (GridGenerator::GetPeers ().back()),
    src = Create<PeerDestination> (peers.front()),
    topRight = Create<PeerDestination> (peers[3]),
    botLeft = Create<PeerDestination> (peers[5]);
  Ptr<RonPath> path, path1 = Create<RonPath> (dest), path2 = Create<RonPath> (dest);
  path1->AddHop (botLeft, path1->Begin ());
  path2->AddHop (topRight, path2->Begin ());

  Ptr<OrthogonalRonPathHeuristic> ortho = CreateObject<OrthogonalRonPathHeuristic> ();
  
  ortho->SetPeerTable (RonPeerTable::GetMaster ());
  ortho->SetSourcePeer (peers.front());
  ortho->MakeTopLevel ();

  //do this so we can check some internal data structs
  ortho->BuildPaths (dest);
  ortho->UpdateLikelihoods (dest);

  // TIMEOUT
  //  ortho->NotifyTimeout (path1, Simulator::Now ());

  path = ortho->GetBestPath (dest);

  //make sure we get the right path
  equality = (*(path) == *(path2)) or (*(path) == *(path1));

  Ptr<RonPeerEntry> thisPeer = *(*(path->Begin ()))->Begin ();

  NS_TEST_ASSERT_MSG_EQ (equality, true, "returned path should have top right or bottom left node!");

  // we don't define a rigid tie-breaker, so check that we get both top-right and bottom-left, but not the same one each time
  bool gotTopRight = true;
  if (*path == *path1)
    gotTopRight = false;

  ortho->NotifyTimeout (path, Simulator::Now ());

  path = ortho->GetBestPath (dest);

  // now it should be the other one
  if (gotTopRight) {
    equality = ArePeersEqual (path, path1);
    NS_TEST_ASSERT_MSG_EQ (equality, true, "returned path should have bottom left node now!");
  } else {
    equality = ArePeersEqual (path, path2);
    NS_TEST_ASSERT_MSG_EQ (equality, true, "returned path should have top right node now!");
  }

  //NOTE: we currently ignore obtuse angles in this heuristic, which any node other than the far corners of the square will be
  //now it should be peers[2], a node 'inside' the grid
  // ortho->NotifyTimeout (path, Simulator::Now ());

  // path = ortho->GetBestPath (dest);

  // equality = *(*path->Begin ()) == *Create<PeerDestination> (peers[2]);
  // NS_TEST_ASSERT_MSG_NE (equality, true, "next path should be peers[2], i.e. (4,1)");
}


////////////////////////////////////////////////////////////////////////////////
class TestRonHeader : public TestCase
{
public:
  TestRonHeader ();
  virtual ~TestRonHeader ();
  NodeContainer nodes;
  Ptr<RonPeerTable> peers;

private:
  virtual void DoRun (void);
};


TestRonHeader::TestRonHeader ()
  : TestCase ("Test basic operations of RonHeader objects")
{
  nodes = GridGenerator::GetNodes ();

  //we want to work with a table rather than lists
  //PeerContainer peersList = GridGenerator::GetPeers ();
  peers = Create<RonPeerTable> ();

  //for (PeerContainer::iterator itr = peersList.begin ();
  for (NodeContainer::Iterator itr = nodes.Begin ();
       itr != nodes.End (); itr++)
    {
      NS_ASSERT_MSG (!peers->IsInTable ((*itr)->GetId ()), "this peer (" << (*itr)->GetId () << ") shouldn't be in the table yet!");

      peers->AddPeer (*itr);

      NS_ASSERT_MSG (peers->IsInTable ((*itr)->GetId ()), "peers should be in table now");
    }
}

TestRonHeader::~TestRonHeader ()
{
}

void
TestRonHeader::DoRun (void)
{
  NS_ASSERT_MSG (peers->GetN () == nodes.GetN (), "table size != node container size");

  Ptr<Packet> packet = Create<Packet> ();
  Ipv4Address addr0 = peers->GetPeer (nodes.Get (0)->GetId ())->address;
  uint32_t tmpId = nodes.Get (1)->GetId ();
  NS_TEST_ASSERT_MSG_EQ (peers->IsInTable (tmpId), true, "table should contain this peer");
  Ipv4Address addr1 = peers->GetPeer (tmpId)->address;
  Ipv4Address addr2 = peers->GetPeer (nodes.Get (2)->GetId ())->address;
  Ipv4Address srcAddr = Ipv4Address ("244.244.244.244");

  Ptr<RonHeader> head0 = Create<RonHeader> ();
  head0->SetDestination (addr0);
  head0->SetOrigin (addr2);
  Ptr<RonHeader> head1 = Create<RonHeader> (addr1);
  Ptr<RonHeader> head2 = Create<RonHeader> (addr2, addr1);

  //test destinations based on constructors

  bool equality = head1->GetFinalDest () == head2->GetNextDest ();
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing RonHeader GetFinalDest and GetNextDest with 2 diff constructors.");

  NS_TEST_ASSERT_MSG_EQ (head2->IsForward (), true, "testing IsForward");
  NS_TEST_ASSERT_MSG_EQ (head0->IsForward (), false, "testing IsForward false positive");

  equality = head1->GetFinalDest () == head1->GetNextDest ();
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing RonHeader GetFinalDest and GetNextDest should be same.");

  equality = head2->GetFinalDest () == head2->GetNextDest ();
  NS_TEST_ASSERT_MSG_EQ (equality, false, "testing RonHeader GetFinalDest and GetNextDest shouldn't be same.");

  equality = head0->GetFinalDest () == Create<RonHeader> (addr0)->GetNextDest ();
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing RonHeader GetFinalDest and GetNextDest with 1 constructor and SetDestination, fresh header.");

  equality = head0->GetFinalDest () == Create<RonHeader> (addr1)->GetNextDest ();
  NS_TEST_ASSERT_MSG_EQ (equality, false, "testing RonHeader GetFinalDest and GetNextDest false positive equality with 1 constructor and SetDestination, fresh header.");

  //test seq#
  NS_TEST_ASSERT_MSG_EQ (head0->GetSeq (), 0, "testing default seq#");
  head0->SetSeq (5);
  NS_TEST_ASSERT_MSG_EQ (head0->GetSeq (), 5, "testing seq# after setting it");

  //test origin
  
  head2->SetOrigin (srcAddr);
  NS_TEST_ASSERT_MSG_EQ (head2->GetOrigin (), srcAddr, "testing proper origin after setting it");

  //test operator=
  RonHeader tmpHead = *head2;
  NS_TEST_ASSERT_MSG_EQ (head2->GetFinalDest (), tmpHead.GetFinalDest (), "testing assignment operator");
  NS_TEST_ASSERT_MSG_EQ (head2->GetNextDest (), tmpHead.GetNextDest (), "testing assignment operator");
  NS_TEST_ASSERT_MSG_EQ (head2->GetOrigin (), tmpHead.GetOrigin (), "testing assignment operator");
  NS_TEST_ASSERT_MSG_EQ (head2->GetSeq (), tmpHead.GetSeq (), "testing assignment operator");

  //test reverse
  tmpHead = *head2;
  head2->ReversePath ();
  Ipv4Address prevHop = head2->GetNextDest ();
  NS_TEST_ASSERT_MSG_EQ (head2->GetFinalDest (), srcAddr, "testing proper origin after reversing");
  NS_TEST_ASSERT_MSG_EQ (head2->GetNextDest (), prevHop, "testing proper next hop after reversing");
  NS_TEST_ASSERT_MSG_NE (head2->GetFinalDest (), (Ipv4Address)(uint32_t)0, "final destination is 0 after reverse!");
  NS_TEST_ASSERT_MSG_NE (head2->GetOrigin (), (Ipv4Address)(uint32_t)0, "origin is 0 after reverse!");

  //undo reverse
  head2->ReversePath ();
  equality = tmpHead.GetFinalDest () == head2->GetFinalDest ();
  NS_TEST_ASSERT_MSG_EQ (equality, true, "header should be back to normal");
  equality = tmpHead.GetNextDest () == head2->GetNextDest ();
  NS_TEST_ASSERT_MSG_EQ (equality, true, "header should be back to normal");
  equality = tmpHead.GetOrigin () == head2->GetOrigin ();
  NS_TEST_ASSERT_MSG_EQ (equality, true, "header should be back to normal");

  //test hop incrementing
  prevHop = head2->GetFinalDest ();
  head2->IncrHops ();
  NS_TEST_ASSERT_MSG_EQ (head2->GetNextDest (), prevHop, "testing proper next hop after incrementing hops");

  //test path manipulation/interface

  Ptr<PeerDestination> dest = Create<PeerDestination> (peers->GetPeerByAddress (addr2));
  Ptr<RonPath> path = Create<RonPath> (dest);
  path->AddHop (Create<PeerDestination> (peers->GetPeerByAddress (addr1)), path->Begin ());

  //path should now be the same as returned by head2
  //path: scrAddr -> addr1 -> addr2

  NS_TEST_ASSERT_MSG_EQ (head2->GetPath ()->GetN (), 2, "path pulled from header isn't right length");

  equality = *head2->GetPath () == *path;
  NS_TEST_ASSERT_MSG_EQ (equality, true, "Checking GetPath equality");

  equality = *head0->GetPath () == *head2->GetPath ();
  NS_TEST_ASSERT_MSG_EQ (equality, false, "Checking GetPath inequality");

  head0->SetPath (path);
  equality = *head0->GetPath () == *head2->GetPath ();
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing path equality after SetPath");

  //RonHeader currently doesn't provide access to inserting hops at arbitrary locations,
  //so we reverse, add at end, and reverse again to insert at front (after source)
  head0->ReversePath ();
  head0->AddDest (addr0);
  head0->ReversePath ();
  path->AddHop (peers->GetPeerByAddress (addr0), path->Begin ());
  equality = *path == *head0->GetPath ();
  NS_TEST_ASSERT_MSG_EQ (equality, true, "Checking path equality after modifying both path and header");
  NS_TEST_ASSERT_MSG_NE (head0->GetFinalDest (), (Ipv4Address)(uint32_t)0, "final destination is 0 after double reverse!");
  NS_TEST_ASSERT_MSG_NE (head0->GetOrigin (), (Ipv4Address)(uint32_t)0, "origin is 0 after double reverse!");

  //TODO: test iterators

  //test serializing
  packet->AddHeader (*head0);
  packet->RemoveHeader (tmpHead);

  NS_TEST_ASSERT_MSG_EQ (head0->GetFinalDest (), tmpHead.GetFinalDest (), "testing assignment operator");
  NS_TEST_ASSERT_MSG_EQ (head0->GetNextDest (), tmpHead.GetNextDest (), "testing assignment operator");
  NS_TEST_ASSERT_MSG_EQ (head0->GetOrigin (), tmpHead.GetOrigin (), "testing assignment operator");
  NS_TEST_ASSERT_MSG_EQ (head0->GetSeq (), tmpHead.GetSeq (), "testing assignment operator");
  NS_TEST_ASSERT_MSG_EQ (head0->GetHop (), tmpHead.GetHop (), "testing assignment operator");
  NS_TEST_ASSERT_MSG_EQ (head0->IsForward (), tmpHead.IsForward (), "testing assignment operator");
  equality = tmpHead.GetFinalDest () == (Ipv4Address)(uint32_t)0;
  NS_TEST_ASSERT_MSG_NE (equality, true, "final destination is 0!");

  Ptr<RonPath> path1, path2;
  path1 = head0->GetPath ();
  path2 = tmpHead.GetPath ();
  equality = *path1 == *path2;
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing deserialized packet path");

  head0->ReversePath ();
  NS_TEST_ASSERT_MSG_NE (head0->GetFinalDest (), (Ipv4Address)(uint32_t)0, "final destination is 0 after reverse!");
  NS_TEST_ASSERT_MSG_NE (head0->GetOrigin (), (Ipv4Address)(uint32_t)0, "origin is 0 after reverse!");
  path1 = head0->GetPath ();
  path2 = tmpHead.GetPath ();
  equality = *path1 == *path2;
  NS_TEST_ASSERT_MSG_EQ (equality, false, "testing deserialized packet path false positive after reverse");
}


////////////////////////////////////////////////////////////////////////////////

class TestAngleRonPathHeuristic : public TestCase
{
public:
  TestAngleRonPathHeuristic ();
  virtual ~TestAngleRonPathHeuristic ();
  NodeContainer nodes;
  PeerContainer peers;

private:
  virtual void DoRun (void);
};



TestAngleRonPathHeuristic::TestAngleRonPathHeuristic ()
  : TestCase ("Test AngleRonPathHeuristic feature of RonPathHeuristic objects, using NewRegion and Random")
{
  nodes = GridGenerator::GetNodes ();
  peers = GridGenerator::GetPeers ();
}

TestAngleRonPathHeuristic::~TestAngleRonPathHeuristic ()
{
}

void
TestAngleRonPathHeuristic::DoRun (void)
{
  bool equality;
  Ptr<PeerDestination> dest = Create<PeerDestination> (GridGenerator::GetPeers ().back()),
  //Ptr<PeerDestination> dest = Create<PeerDestination> (peers.back()),
    src = Create<PeerDestination> (peers.front()),
    topRight = Create<PeerDestination> (peers[3]),
    botLeft = Create<PeerDestination> (peers[5]),
    nextBest = Create<PeerDestination> (peers[2]); // (4,1)
  Ptr<RonPath> path, path1 = Create<RonPath> (dest), path2 = Create<RonPath> (dest), path3 = Create<RonPath> (dest);
  path1->AddHop (botLeft, path1->Begin ());
  path2->AddHop (topRight, path2->Begin ());
  path3->AddHop (nextBest, path3->Begin ());

  Ptr<AngleRonPathHeuristic> angle = CreateObject<AngleRonPathHeuristic> ();
  
  angle->SetPeerTable (RonPeerTable::GetMaster ());
  angle->SetSourcePeer (peers.front());
  angle->MakeTopLevel ();

  //do this so we can check some internal data structs
  angle->BuildPaths (dest);
  angle->UpdateLikelihoods (dest);

  // TIMEOUT
  //  ortho->NotifyTimeout (path1, Simulator::Now ());

  path = angle->GetBestPath (dest);

  //make sure we get the right path
  equality = (*(path) == *(path2)) or (*(path) == *(path1));
  NS_TEST_ASSERT_MSG_EQ (equality, true, "returned path should have top right or bottom left node!");

  // we don't define a rigid tie-breaker, so check that we get both top-right and bottom-left, but not the same one each time
  bool gotTopRight = true;
  if (*path == *path1)
    gotTopRight = false;

  angle->NotifyTimeout (path, Simulator::Now ());

  path = angle->GetBestPath (dest);

  // now it should be the other one
  if (gotTopRight) {
    equality = (*(path) == *(path1));
    NS_TEST_ASSERT_MSG_EQ (equality, true, "returned path should have bottom left node now!");
  } else {
    equality = (*(path) == *(path2));
    NS_TEST_ASSERT_MSG_EQ (equality, true, "returned path should have top right node now!");
  }

  angle->NotifyTimeout (path, Simulator::Now ());

  path = angle->GetBestPath (dest);
  
  // prints don't work...
  //NS_LOG_DEBUG ( (*((*path->Begin ())->Begin ())) );
  
  // Zhipeng: this one is giving us peers[1], i.e. (1,1) instead of the right one...

  // now it should be the next best angle option, near the bottom left
  equality = ArePeersEqual (path, path3);

  NS_TEST_ASSERT_MSG_EQ (equality, true, "next path should be peers[2], i.e. (4,1)");
}

////////////////////////////////////////////////////////////////////////////////

class TestIdealRonPathHeuristic : public TestCase
{
public:
  TestIdealRonPathHeuristic ();
  virtual ~TestIdealRonPathHeuristic ();
  NodeContainer nodes;
  PeerContainer peers;

private:
  virtual void DoRun (void);
};



TestIdealRonPathHeuristic::TestIdealRonPathHeuristic ()
  : TestCase ("Test IdealRonPathHeuristic")
{
  nodes = GridGenerator::GetNodes ();
  peers = GridGenerator::GetPeers ();
}

TestIdealRonPathHeuristic::~TestIdealRonPathHeuristic ()
{
}

void
TestIdealRonPathHeuristic::DoRun (void)
{
  bool equality;
  Ptr<Node> nodeToFail;
  Ptr<RonPeerEntry> srcPeer = GridGenerator::GetPeer (0, 0);
  Ptr<PeerDestination> dest = Create<PeerDestination> (GridGenerator::GetPeer (4, 4)),
    src = Create<PeerDestination> (srcPeer),
    topRight = Create<PeerDestination> (GridGenerator::GetPeer (4, 0)),
    botLeft = Create<PeerDestination> (GridGenerator::GetPeer (0, 4));
  Ptr<RonPath> path, path1 = Create<RonPath> (dest), path2 = Create<RonPath> (dest);
  path1->AddHop (botLeft, path1->Begin ());
  path2->AddHop (topRight, path2->Begin ());
  equality = *path1 == *path2;
  NS_TEST_ASSERT_MSG_NE (equality, true, "path1 should not equal path2!");

  Ptr<IdealRonPathHeuristic> ideal = CreateObject<IdealRonPathHeuristic> ();
  Ptr<RonPeerTable> peers = Create<RonPeerTable> ();
  peers->AddPeer (GridGenerator::GetPeer (4, 0));
  peers->AddPeer (GridGenerator::GetPeer (0, 4));
  ideal->SetPeerTable (peers);
  ideal->SetSourcePeer (srcPeer);
  ideal->MakeTopLevel ();

  //do this so we can check some internal data structs
  //ideal->BuildPaths (dest);
  int size = (*(ideal->m_masterLikelihoods))[dest].size ();
  NS_TEST_ASSERT_MSG_EQ (size, 0, "PathLikelihoodInnerTable should have 0 paths, but got " << size);
  ideal->AddPath (path1);
  size = (*(ideal->m_masterLikelihoods))[dest].size ();
  NS_TEST_ASSERT_MSG_EQ (size, 1, "PathLikelihoodInnerTable should have 1 path, but got " << size);
  ideal->AddPath (path2);
  size = (*(ideal->m_masterLikelihoods))[dest].size ();
  NS_TEST_ASSERT_MSG_EQ (size, 2, "PathLikelihoodInnerTable should have 2 paths, but got " << size);

  ideal->UpdateLikelihoods (dest);
  size = (*(ideal->m_masterLikelihoods))[dest].size ();
  NS_TEST_ASSERT_MSG_EQ (size, 2, "PathLikelihoodInnerTable should only have 2 paths, but got " << size);

  path = ideal->GetBestPath (dest);

  //make sure we get the right path
  equality = (*(path) == *(path2)) or (*(path) == *(path1));
  NS_TEST_ASSERT_MSG_EQ (equality, true, "returned path should have top right or bottom left node!"); 
  // we don't define a rigid tie-breaker, so check that we get both top-right and bottom-left, but not the same one each time
  bool gotTopRight = true;
  if (*path == *path1)
    gotTopRight = false;

  // now, we want to fail the overlay node it chose before, force an update, and verify that the other node is returned next
  Ptr<RonPath> lastPath = path;
  nodeToFail = (*((*lastPath->Begin ())->Begin ()))->node;
  FailNode (nodeToFail);
  ideal->ForceUpdateLikelihoods (dest);

  double lh = ((ideal->m_likelihoods))[dest][path]->GetLh ();
  NS_TEST_ASSERT_MSG_EQ (lh, 0.0, "likelihood of failed path should be 0, but got " << lh);
  lh = ideal->GetLikelihood (path);
  NS_TEST_ASSERT_MSG_EQ (lh, 0.0, "likelihood of failed path should be 0, but got " << lh);

  // now we should be getting the other path
  path = ideal->GetBestPath (dest);

  equality = *path == *lastPath;
  NS_TEST_ASSERT_MSG_NE (equality, true, "should not have gotten the same path after failing one!");

  // now it should be the other one
  if (gotTopRight) {
    equality = (*(path) == *(path1));
    NS_TEST_ASSERT_MSG_EQ (equality, true, "returned path should have bottom left node now!");
  } else {
    equality = (*(path) == *(path2));
    NS_TEST_ASSERT_MSG_EQ (equality, true, "returned path should have top right node now!");
  }

  // Fail this other node and then we should get an Exception since no Paths will have LH > 0!
  nodeToFail = (*((*path->Begin ())->Begin ()))->node;
  FailNode (nodeToFail);
  ideal->ForceUpdateLikelihoods (dest);

  try {
    lastPath = path;
    path = ideal->GetBestPath (dest);
    equality = *path == *lastPath;
    lh = (*(ideal->m_masterLikelihoods))[dest][path]->GetLh ();
    NS_TEST_ASSERT_MSG_EQ (lh, 0.0, "likelihood of failed path should be 0, but got " << lh);
    NS_TEST_ASSERT_MSG_EQ (equality, false, "should not have gotten the same path since we failed that node!");
    NS_TEST_ASSERT_MSG_EQ (true, false, "should not have gotten any valid peers now that all are failed!");
  }
  catch (RonPathHeuristic::NoValidPeerException& e) {}

  // let's unfail this node and verify that we get the last path again
  UnfailNode (nodeToFail);
  ideal->ForceUpdateLikelihoods (dest);
  lastPath = path;
  path = ideal->GetBestPath (dest);
  equality = (*path) == (*lastPath);
  NS_TEST_ASSERT_MSG_EQ (equality, true, "Should have gotten the last path again");
  lh = (*(ideal->m_masterLikelihoods))[dest][path]->GetLh ();
  NS_TEST_ASSERT_MSG_EQ (lh, 1.0, "likelihood of unfailed path should be 1, but got " << lh);

  // now, we should fail some non-overlay nodes to check some
  // edge conditions by verifying that failing some of the other nodes
  // results in no paths being available as well

  // check the first and last nodes on each leg of the path
  // note that gotTopRight refers to the FIRST path,
  // but we're now working with the second
  NodeContainer nodesToFail;
  if (!gotTopRight)
  {
    nodesToFail.Add (GridGenerator::GetNode (4, 0));
    nodesToFail.Add (GridGenerator::GetNode (1, 0));
    nodesToFail.Add (GridGenerator::GetNode (3, 0));
    nodesToFail.Add (GridGenerator::GetNode (4, 3));
    nodesToFail.Add (GridGenerator::GetNode (4, 1));
  }
  else
  {
    nodesToFail.Add (GridGenerator::GetNode (0, 4));
    nodesToFail.Add (GridGenerator::GetNode (0, 1));
    nodesToFail.Add (GridGenerator::GetNode (0, 3));
    nodesToFail.Add (GridGenerator::GetNode (1, 4));
    nodesToFail.Add (GridGenerator::GetNode (3, 4));
  }

  // Now iterate over each of these node edge cases and verify that failing
  // only that one node will cause the heuristic to abandon that path entirely
  for (NodeContainer::Iterator nodeItr = nodesToFail.Begin ();
      nodeItr != nodesToFail.End (); nodeItr++)
  {
    nodeToFail = *nodeItr;
    FailNode (nodeToFail);
    ideal->ForceUpdateLikelihoods (dest);
    try {
      lastPath = path;
      path = ideal->GetBestPath (dest);
      equality = *path == *lastPath;
      Ptr<MobilityModel> mm = nodeToFail->GetObject<MobilityModel> ();
      NS_TEST_ASSERT_MSG_EQ (true, false, "should not have gotten any valid peers now that all are failed! peer at (" << mm->GetPosition () << ")");
    }
    catch (RonPathHeuristic::NoValidPeerException& e) {}

    // unfail that node and try the one on the end of the physical path first leg
    UnfailNode (nodeToFail);
  }

  // Next, let's fail some links (Ipv4Interfaces) and verify that the
  // heuristic behaves as expected.  Let's make sure to also fail the
  // first physical link on either path, hence adding topLeft to nodesToFail
  nodesToFail.Add (GridGenerator::GetNode (0, 0));
  for (NodeContainer::Iterator nodeItr = nodesToFail.Begin ();
      nodeItr != nodesToFail.End (); nodeItr++)
  {
    // Fail all the links incident with this Node
    Ptr<Ipv4> ipv4 = (*nodeItr)->GetObject<Ipv4> ();
    for (uint32_t nDevs = 0; nDevs < ipv4->GetNInterfaces (); nDevs++)
    {
      FailIpv4 (ipv4, nDevs);
    }
    ideal->ForceUpdateLikelihoods (dest);
    try {
      lastPath = path;
      path = ideal->GetBestPath (dest);
      equality = *path == *lastPath;
      Ptr<MobilityModel> mm = (*nodeItr)->GetObject<MobilityModel> ();
      NS_TEST_ASSERT_MSG_EQ (true, false, "should not have gotten any valid peers now that all are failed! peer at (" << mm->GetPosition () << ")");
    }
    catch (RonPathHeuristic::NoValidPeerException& e) {}

    for (uint32_t nDevs = 0; nDevs < ipv4->GetNInterfaces (); nDevs++)
    {
      UnfailIpv4 (ipv4, nDevs);
    }
  }

  // Need to unfail nodes!
  UnfailNode ((*(botLeft->Begin ()))->node, Seconds (60));
  UnfailNode ((*(topRight->Begin ()))->node, Seconds (60));
}

////////////////////////////////////////////////////////////////////////////////

class TestFarthestFirstRonPathHeuristic : public TestCase
{
public:
  TestFarthestFirstRonPathHeuristic ();
  virtual ~TestFarthestFirstRonPathHeuristic ();
  NodeContainer nodes;
  PeerContainer peers;

private:
  virtual void DoRun (void);
};


TestFarthestFirstRonPathHeuristic::TestFarthestFirstRonPathHeuristic ()
  : TestCase ("Test FarthestFirstRonPathHeuristic feature of RonPathHeuristic objects")
{
  nodes = GridGenerator::GetNodes ();
  peers = GridGenerator::GetPeers ();
}

TestFarthestFirstRonPathHeuristic::~TestFarthestFirstRonPathHeuristic ()
{
}

void
TestFarthestFirstRonPathHeuristic::DoRun (void)
{
  bool equality;
  Ptr<PeerDestination> dest = Create<PeerDestination> (GridGenerator::GetPeer (0, 4)),
    src = Create<PeerDestination> (peers.front()),
    botRight = Create<PeerDestination> (GridGenerator::GetPeer (4, 4)),
    botLeft = Create<PeerDestination> (peers[5]);
  Ptr<RonPath> path,path1 = Create<RonPath> (dest), path2 = Create<RonPath> (dest), path3 = Create<RonPath> (dest);
  path1->AddHop (botLeft, path1->Begin ());
  path2->AddHop (botRight, path2->Begin ());

  Ptr<FarthestFirstRonPathHeuristic> far = CreateObject<FarthestFirstRonPathHeuristic> ();
  
  far->SetPeerTable (RonPeerTable::GetMaster ());
  far->SetSourcePeer (peers.front());
  far->MakeTopLevel ();

  //do this so we can check some internal data structs
  far->BuildPaths (dest);
  far->UpdateLikelihoods (dest);

  Ptr<PeerDestination> expectedDestination;

  // TIMEOUT

  path = far->GetBestPath (dest);

  expectedDestination = botRight;

  //make sure we get the right path
  //equality = *(*path->Begin ()) == *Create<PeerDestination> (peers[4]);
  //equality = (*(*path->Begin ()) == *expectedDestination);
  equality = *path == *path2;
  NS_TEST_ASSERT_MSG_EQ (equality, true, "returned path should have bottom right node!");

  far->NotifyTimeout (path, Simulator::Now ());

  path = far->GetBestPath (dest);

  expectedDestination = Create<PeerDestination> (GridGenerator::GetPeer (3, 3));

//make sure we get the right path
  equality = (*(*(*path->Begin ())->Begin ()) == *(*expectedDestination->Begin ()));
  NS_TEST_ASSERT_MSG_EQ (equality, true, "returned path should have been next from bottom right, i.e. 3,3!");
}

//////////////////////////////////////////////////////////////////

class TestDistRonPathHeuristic : public TestCase
{
public:
  TestDistRonPathHeuristic ();
  virtual ~TestDistRonPathHeuristic ();
  NodeContainer nodes;
  PeerContainer peers;

private:
  virtual void DoRun (void);
};


TestDistRonPathHeuristic::TestDistRonPathHeuristic ()
  : TestCase ("Test DistRonPathHeuristic feature of RonPathHeuristic objects, using NewRegion and Random")
{
  nodes = GridGenerator::GetNodes ();
  peers = GridGenerator::GetPeers ();
}

TestDistRonPathHeuristic::~TestDistRonPathHeuristic ()
{
}

void
TestDistRonPathHeuristic::DoRun (void)
{
  bool equality;
  Ptr<PeerDestination> dest = Create<PeerDestination> (peers.back()),
    src = Create<PeerDestination> (peers.front()),
    topRight = Create<PeerDestination> (peers[3]),
    botLeft = Create<PeerDestination> (peers[5]),
    innerLeft = Create<PeerDestination> (peers[1]),
    innerRight = Create<PeerDestination> (peers[4]),
    nextBest = Create<PeerDestination> (GridGenerator::GetPeer (4,1));
  Ptr<RonPath> path, path1 = Create<RonPath> (dest), path2 = Create<RonPath> (dest), path3 = Create<RonPath> (dest),
    path4 = Create<RonPath> (dest), path5 = Create<RonPath> (dest);
  path1->AddHop (botLeft, path1->Begin ());
  path2->AddHop (topRight, path2->Begin ());
  path3->AddHop (nextBest, path3->Begin ());
  path4->AddHop (innerRight, path4->Begin ());
  path5->AddHop (innerLeft, path5->Begin ());

  Ptr<DistRonPathHeuristic> dist = CreateObject<DistRonPathHeuristic> ();
  
  dist->SetPeerTable (RonPeerTable::GetMaster ());
  dist->SetSourcePeer (peers.front());
  dist->MakeTopLevel ();

  //do this so we can check some internal data structs
  dist->BuildPaths (dest);
  dist->UpdateLikelihoods (dest);

  // TIMEOUT
  
  path = dist->GetBestPath (dest);

  //make sure we get the right path
  equality = (*(path) == *(path2)) or (*(path) == *(path1));
  NS_TEST_ASSERT_MSG_EQ (equality, true, "returned path should have top right or bottom left node!");

  // we don't define a rigid tie-breaker, so check that we get both top-right and bottom-left, but not the same one each time
  bool gotTopRight = true;
  if (*path == *path1)
    gotTopRight = false;

  dist->NotifyTimeout (path, Simulator::Now ());

  path = dist->GetBestPath (dest);

  // now it should be the other one
  if (gotTopRight) {
    equality = (*(path) == *(path1));
    NS_TEST_ASSERT_MSG_EQ (equality, true, "returned path should have bottom left node now!");
  } else {
    equality = (*(path) == *(path2));
    NS_TEST_ASSERT_MSG_EQ (equality, true, "returned path should have top right node now!");
  }

  dist->NotifyTimeout (path, Simulator::Now ());

  path = dist->GetBestPath (dest);
  
  // now it should be the next best angle option, near the bottom left
  equality = *(path) == *path3;
  NS_TEST_ASSERT_MSG_EQ (equality, true, "next path should be peers[2], i.e. (4,1)");
  
  bool gotInnerRight = true;
  if (*path == *path5)
    gotTopRight = false;

  dist->NotifyTimeout (path, Simulator::Now ());

  path = dist->GetBestPath (dest);

  // now it should be the other one
  if (gotInnerRight) {
    equality = (*(path) == *(path4));
    NS_TEST_ASSERT_MSG_EQ (equality, true, "returned path should have inner right node now!");
  } else {
    equality = (*(path) == *(path5));
    NS_TEST_ASSERT_MSG_EQ (equality, true, "returned path should have inner left node now!");
  }
}


class TestGeocronExperiment : public TestCase
{
public:
  TestGeocronExperiment ();
  virtual ~TestGeocronExperiment ();
  NodeContainer nodes;
  Ptr<RonPeerTable> peers;

private:
  virtual void DoRun (void);
};

TestGeocronExperiment::TestGeocronExperiment ()
  : TestCase ("Test various features of GeocronExperiment object")
{
  nodes = GridGenerator::GetNodes ();
}

TestGeocronExperiment::~TestGeocronExperiment ()
{}

void TestGeocronExperiment::DoRun (void)
{
  NS_TEST_ASSERT_MSG_EQ (GetNodeDegree (nodes.Get (0)), 2, "corner node doesn't have degree 2!");
  NS_TEST_ASSERT_MSG_EQ (GetNodeDegree (nodes.Get (1)), 4, "interior node doesn't have degree 4!");
  NS_TEST_ASSERT_MSG_EQ (GetNodeDegree (nodes.Get (2)), 3, "edge node doesn't have degree 3!");
}

class TestPathRetrieval : public TestCase
{
public:
  TestPathRetrieval ();
  virtual ~TestPathRetrieval ();
  NodeContainer nodes;
  Ptr<RonPeerTable> peers;

private:
  virtual void DoRun (void);
};

TestPathRetrieval::TestPathRetrieval ()
  : TestCase ("Test the Ipv4NixVectorRouting mechanism for building node & link paths")
{
  nodes = GridGenerator::GetNodes ();
}

TestPathRetrieval::~TestPathRetrieval ()
{}

void TestPathRetrieval::DoRun (void)
{
  Ptr<Node> tl = GridGenerator::GetNode (0, 0), bl = GridGenerator::GetNode (0, 4), br = GridGenerator::GetNode (4, 4);

  NodeContainer nodes1;
  Ipv4InterfaceContainer ifaces1;

  Ptr<Ipv4NixVectorRouting> nvr = tl->GetObject<Ipv4NixVectorRouting> ();
  Ipv4Address addr = GetNodeAddress (bl);

  nvr->GetPathFromIpv4Address (addr, nodes1, ifaces1);

  NS_TEST_ASSERT_MSG_EQ (nodes1.GetN (), 3, "path should only have 3 nodes");
  NS_TEST_ASSERT_MSG_EQ (ifaces1.GetN (), 4, "path should include 4 links");

  // Let's verify the path is correct my iterating over what we know it should be.
  // Nodes are relatively straightforward, but links are a bit tricky.
  // To compare links(Ipv4Interfaces), we need to get the node at the other end of the link
  // corresponding with this iface
  // and compare it with the node we will visit next. Thus, we add the destination node to
  // the node list so that we can properly bookend the last link.
  nodes1.Add (bl);
  NodeContainer::Iterator nodeItr = nodes1.Begin ();
  Ipv4InterfaceContainer::Iterator ifaceItr = ifaces1.Begin ();
  bool atEnd;

  for (int i = 1; i < 5; i++)
  {
    // Links first since there are more and we're starting with the link preceding the first node
    Ptr<NetDevice> devOnOtherSide = GetOtherNetDevice (ifaceItr->first->GetNetDevice (ifaceItr->second));
    Ptr<Node> nodeOnOtherSide = devOnOtherSide->GetNode ();
    NS_TEST_ASSERT_MSG_EQ (nodeOnOtherSide, (*nodeItr), "Node at other end of link not as expected at iteration " << i);

    atEnd = (ifaceItr == ifaces1.End ());
    NS_TEST_ASSERT_MSG_EQ (atEnd, false, "iface iterator reached end earlier than expected!");
    ifaceItr++;

    // nodes 
    atEnd = (nodeItr == nodes1.End ());
    NS_TEST_ASSERT_MSG_EQ (atEnd, false, "node iterator reached end earlier than expected!");

    NS_TEST_ASSERT_MSG_EQ (*nodeItr, GridGenerator::GetNode (0, i), "Node at location 0," << i << " is not correct!");
    nodeItr++;
  }
  atEnd = (nodeItr == nodes1.End ());
  NS_TEST_ASSERT_MSG_EQ (atEnd, true, "node iterator didn't reach end: must be too many nodes?");
  atEnd = (ifaceItr == ifaces1.End ());
  NS_TEST_ASSERT_MSG_EQ (atEnd, true, "iface iterator didn't reach end: must be too many interfaces?");

  // now get the next leg of the path
  nvr = bl->GetObject<Ipv4NixVectorRouting> ();
  addr = GetNodeAddress (br);
  NodeContainer nodes2;
  Ipv4InterfaceContainer ifaces2;

  nvr->GetPathFromIpv4Address (addr, nodes2, ifaces2);

  NS_TEST_ASSERT_MSG_EQ (nodes2.GetN (), 3, "path should only have 3 nodes");
  NS_TEST_ASSERT_MSG_EQ (ifaces2.GetN (), 4, "path should include 4 links");

  nodes2.Add (br);
  nodeItr = nodes2.Begin ();
  ifaceItr = ifaces2.Begin ();

  for (int i = 1; i < 5; i++)
  {
    // Links first since there are more and we're starting with the link preceding the first node
    Ptr<NetDevice> devOnOtherSide = GetOtherNetDevice (ifaceItr->first->GetNetDevice (ifaceItr->second));
    Ptr<Node> nodeOnOtherSide = devOnOtherSide->GetNode ();
    NS_TEST_ASSERT_MSG_EQ (nodeOnOtherSide, (*nodeItr), "Node at other end of link not as expected at iteration " << i);

    atEnd = (ifaceItr == ifaces2.End ());
    NS_TEST_ASSERT_MSG_EQ (atEnd, false, "iface iterator reached end earlier than expected!");
    ifaceItr++;

    // nodes 
    atEnd = (nodeItr == nodes2.End ());
    NS_TEST_ASSERT_MSG_EQ (atEnd, false, "node iterator reached end earlier than expected!");

    NS_TEST_ASSERT_MSG_EQ (*nodeItr, GridGenerator::GetNode (i, 4), "Node at location " << i << ",4 is not correct!");
    nodeItr++;
  }
  atEnd = (nodeItr == nodes2.End ());
  NS_TEST_ASSERT_MSG_EQ (atEnd, true, "node iterator didn't reach end: must be too many nodes?");
  atEnd = (ifaceItr == ifaces2.End ());
  NS_TEST_ASSERT_MSG_EQ (atEnd, true, "iface iterator didn't reach end: must be too many interfaces?");
}


////////////////////////////////////////////////////////////////////////////////
////////////////////$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$////////////////////
//////////$$$$$$$$$$   End of test cases - create test suite $$$$$$$$$$/////////
////////////////////$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$////////////////////
////////////////////////////////////////////////////////////////////////////////


// The TestSuite class names the TestSuite, identifies what type of TestSuite,
// and enables the TestCases to be run.  Typically, only the constructor for
// this class must be defined
//
class GeocronTestSuite : public TestSuite
{
public:
  GeocronTestSuite ();
};

GeocronTestSuite::GeocronTestSuite ()
  : TestSuite ("geocron", UNIT)
{
  //basic data structs
  AddTestCase (new TestPeerEntry, TestCase::QUICK);
  AddTestCase (new TestPeerTable, TestCase::QUICK);
  AddTestCase (new TestRonPath, TestCase::QUICK);

  //heuristics
  AddTestCase (new TestRonPathHeuristic, TestCase::QUICK);
  AddTestCase (new TestAggregateRonPathHeuristic, TestCase::QUICK);
  AddTestCase (new TestAngleRonPathHeuristic, TestCase::QUICK);
  AddTestCase (new TestOrthogonalRonPathHeuristic, TestCase::QUICK);
  AddTestCase (new TestDistRonPathHeuristic, TestCase::QUICK);
  AddTestCase (new TestFarthestFirstRonPathHeuristic, TestCase::QUICK);
  AddTestCase (new TestIdealRonPathHeuristic, TestCase::QUICK);

  //network application / experiment stuff
  AddTestCase (new TestRonHeader, TestCase::QUICK);
  AddTestCase (new TestGeocronExperiment, TestCase::QUICK);
  AddTestCase (new TestPathRetrieval, TestCase::QUICK);
}

// Do not forget to allocate an instance of this TestSuite
static GeocronTestSuite geocronTestSuite;

