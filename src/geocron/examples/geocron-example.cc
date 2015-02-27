/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

/** A simulator of network failure scenarios during disasters.  Builds a mesh topology of
    point-to-point connected nodes generated using Rocketfuel and fails random nodes/links
    within the specified city region. **/

//#include "ns3/ron-header.h"

/* REMOVE THESE INCLUDES WHEN MOVED OUT OF SCRATCH DIRECTORY!!!!!!!!!!! */
/*#include "ron-header.h"
#include "ron-helper.h"
#include "ron-client.h"
#include "ron-server.h"
#include "geocron-experiment.h"*/

#include <boost/tokenizer.hpp>
//#include <boost/regex.hpp>
#include <boost/algorithm/string/replace.hpp>
/*#include <boost/algorithm/string/find_format.hpp>
#include <boost/algorithm/string/finder.hpp>
#include <boost/algorithm/string/formatter.hpp>*/
//#include "boost/system"
#include "ns3/geocron-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("RonDisasterSimulation");

int 
main (int argc, char *argv[])
{
  Ptr<GeocronExperiment> exp = CreateObject<GeocronExperiment> ();

  ////////////////////////////////////////////////////////////////////////////////
  ////////////////////     Arguments    //////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////
  int verbose = 0;
  bool trace_acks = true;
  bool trace_forwards = true;
  bool trace_sends = true;
  // Max number of devices a node can have to be in the overlay (to eliminate backbone routers)
  std::string fail_prob = "0.3";
  bool report_disaster = true;
  std::string heuristic = "0-1-2";
  std::string filename = "rocketfuel/maps/3356.cch";
  std::string latencyFile = "";
  std::string locationFile = "";
  std::string disaster_location = "Los Angeles, CA";
  bool tracing = false;
  double timeout = 1.0;

  CommandLine cmd;
  // file /topology generator parameters
  cmd.AddValue ("file", "File to read network topology, or configuration in the case of topology generators, from", filename);

  // cmd.AddValue ("event_radius", "Radius of affect of the events", exp->eventRadius);
  // cmd.AddValue ("boundary", "Length of (one side) of the square bounding box for the geographic region under study (in meters)", exp->boundaryLength);

  cmd.AddValue ("latencies", "File to read latencies from in Rocketfuel weights file format", latencyFile);
  cmd.AddValue ("locations", "File to read city locations from (used with Rocketfuel)", locationFile);

  // tracing/output
  cmd.AddValue ("tracing", "Whether to write traces to file", tracing);
  cmd.AddValue ("trace_acks", "Whether to print traces when a client receives an ACK from the server", trace_acks);
  cmd.AddValue ("trace_forwards", "Whether to print traces when a client forwards a packet", trace_forwards);
  cmd.AddValue ("trace_sends", "Whether to print traces when a client sends a packet initially", trace_sends);
  cmd.AddValue ("verbose", "Whether to print verbose log info (1=INFO, 2=LOGIC, 3=FUNCTION)", verbose);

  // scenario parameters
  cmd.AddValue ("disaster", "Where the disaster(s) (and subsequent failures) is(are) to occur "
                "(use underscores for spaces so the command line parser will actually work)", disaster_location);
  cmd.AddValue ("fail_prob", "Probability(s) that a link in the disaster region will fail", fail_prob);
  cmd.AddValue ("report_disaster", "Only RON clients in the disaster region will report to the server", report_disaster);
  cmd.AddValue ("heuristic", "Which heuristic(s) to use when choosing intermediate overlay nodes.", heuristic);
  cmd.AddValue ("runs", "Number of times to run simulation on given inputs.", exp->nruns);
  cmd.AddValue ("start_run", "Starting number to use for multiple runs when outputting files.", exp->start_run_number);
  cmd.AddValue ("timeout", "Seconds to wait for server reply before attempting contact through the overlay.", timeout);
  cmd.AddValue ("contact_attempts", "Number of times a reporting node will attempt to contact the server "
                "(it will use the overlay after the first attempt).  Default is 1 (no overlay).", exp->contactAttempts);

  cmd.Parse (argc,argv);

  // Parse string args for possible multiple arguments
  typedef boost::tokenizer<boost::char_separator<char> > 
    tokenizer;
  boost::char_separator<char> sep("-");
  
  std::vector<double> * failureProbabilities = new std::vector<double> ();
  tokenizer tokens(fail_prob, sep);
  for (tokenizer::iterator tokIter = tokens.begin();
       tokIter != tokens.end(); ++tokIter)
    {
      failureProbabilities->push_back (boost::lexical_cast<double> (*tokIter));
    }

  std::vector<std::string> * disasterLocations = new std::vector<std::string> ();
  tokens = tokenizer(disaster_location, sep);
  for (tokenizer::iterator tokIter = tokens.begin();
       tokIter != tokens.end(); ++tokIter)
    {
      // Need to allow users to input '_'s instead of spaces so the parser won't truncate locations...
      //std::string correctedToken = boost::replace_all_copy(*tokIter, '_', ' ');
      //correctedToken = *(new std::string(*tokIter));
      std::string correctedToken = boost::algorithm::replace_all_copy ((std::string)*tokIter, "_", " ");
      disasterLocations->push_back (correctedToken);
    }

  // keep a map of nicknames to TypeIds
  std::map<std::string, TypeId> heuristicMap;

  /* Here we need to find all of the available RonPathHeuristic derived classes.
     ns-3's TypeId system makes this easier to do. */
  for (uint32_t i = 0; i < TypeId::GetRegisteredN (); i++)
    {
      TypeId tid = TypeId::GetRegistered (i);
      if (!tid.GetGroupName ().compare ("RonPathHeuristics"))
      //if (tid.IsChildOf (RonPathHeuristic::GetTypeId ()))
        {
          //get short name from typeid
          TypeId::AttributeInformation info;
          NS_ASSERT (tid.LookupAttributeByName ("ShortName", &info));
          StringValue name = *(DynamicCast<const StringValue> (info.initialValue));
          //info.checker->Check (name);
          heuristicMap[(std::string)name.Get ()] = tid;

          NS_LOG_INFO ("Found heuristic " << name.Get ());
        }
    }

  // create factories for the heuristics requested by args
  std::vector<ObjectFactory*> * heuristics = new std::vector<ObjectFactory*> ();
  tokens = tokenizer(heuristic, sep);
  for (tokenizer::iterator tokIter = tokens.begin();
       tokIter != tokens.end(); ++tokIter)
    {
      //std::cout << *tokIter <<std::endl; 
      if (heuristicMap.count ((std::string)(*tokIter)))
        {
          ObjectFactory * fact = new ObjectFactory ();
          fact->SetTypeId (heuristicMap[*tokIter]);
          heuristics->push_back (fact);
        }
    }

  ////////////////////////////////////////////////////////////////////////////////
  ////////////////////    Logging    /////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////

  if (verbose == 1)
    {
      LogComponentEnable ("GeocronExperiment", LOG_LEVEL_INFO);
    }
  else if (verbose == 2)
    {
      LogComponentEnable ("GeocronExperiment", LOG_LEVEL_LOGIC);
    }

  ////////////////////////////////////////////////////////////////////////////////
  //////////       Create experiment and set parameters   ////////////////////////
  ////////////////////////////////////////////////////////////////////////////////

  exp->heuristics = heuristics;
  exp->disasterLocations = disasterLocations;
  exp->failureProbabilities = failureProbabilities;
  exp->SetTimeout (Seconds (timeout));

  /*exp->ReadLatencyFile (latencyFile);
  exp->ReadLocationFile (locationFile);
  exp->ReadRocketfuelTopology (filename);
  */
  
  exp->ReadBriteTopology (filename);

  exp->IndexNodes ();
  //exp->SetTraceFile (traceFile);
  exp->RunAllScenarios ();

  return 0;
}
