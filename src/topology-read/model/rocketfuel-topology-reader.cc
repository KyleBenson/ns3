/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 Hajime Tazaki
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
 * Author: Hajime Tazaki (tazaki@sfc.wide.ad.jp)
 */

#include <fstream>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <regex.h>
#include <algorithm>

#include "ns3/log.h"
#include "rocketfuel-topology-reader.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("RocketfuelTopologyReader");

NS_OBJECT_ENSURE_REGISTERED (RocketfuelTopologyReader);

TypeId RocketfuelTopologyReader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RocketfuelTopologyReader")
    .SetParent<Object> ()
  ;
  return tid;
}

RocketfuelTopologyReader::RocketfuelTopologyReader ()
{
  NS_LOG_FUNCTION (this);
}

RocketfuelTopologyReader::~RocketfuelTopologyReader ()
{
  NS_LOG_FUNCTION (this);
}

/* uid @loc [+] [bb] (num_neigh) [&ext] -> <nuid-1> <nuid-2> ... {-euid} ... =name[!] rn */

#define REGMATCH_MAX 16

#define START "^"
#define END "$"
#define SPACE "[ \t]+"
#define MAYSPACE "[ \t]*"

#define ROCKETFUEL_MAPS_LINE \
  START "(-*[0-9]+)" SPACE "(@[?A-Za-z0-9,+.]+)" SPACE \
  "(\\+)*" MAYSPACE "(bb)*" MAYSPACE \
  "\\(([0-9]+)\\)" SPACE "(&[0-9]+)*" MAYSPACE \
  "->" MAYSPACE "(<[0-9 \t<>]+>)*" MAYSPACE \
  "(\\{-[0-9\\{\\} \t-]+\\})*" SPACE \
  "=([A-Za-z0-9.!-]+)" SPACE "r([0-9])" \
  MAYSPACE END

#define ROCKETFUEL_WEIGHTS_LINE \
  START "([A-Za-z,+.]+)([0-9]+)" SPACE "([A-Za-z,+.]+)([0-9]+)" SPACE "([0-9.]+)" MAYSPACE END
  //START "([A-Za-z,+.]+)" SPACE "([0-9]+)" SPACE "([A-Za-z,+.]+)" SPACE "([0-9]+)" SPACE "([0-9.]+)" MAYSPACE END
  // C++ regex library can't seem to support capturing adjacent groups so I had to modify rocketfuel traces for above pattern
// Actually, it looks like the problem is them using destructive modifications to the line by inserting null characters to terminate c-strings

int linksNumber = 0;
int nodesNumber = 0;
// nodeMap indexed by "uid"
std::map<std::string, Ptr<Node> > nodeMap;
// linkMap indexed by "fromUid>toUid"
std::map<std::string, TopologyReader::Link *> linkMap;
// mapping of UIDs to IP addresses when alias files are available
std::map<std::string, std::string> nodeAddresses;
// whether the file contains latencies or weights
bool isLatencies;

RocketfuelTopologyReader::LatenciesMap
RocketfuelTopologyReader::ReadLatencies (std::string filename)
{
  LatenciesMap weights;

  std::ifstream weightFile;
  weightFile.open (filename.c_str ());

  std::istringstream lineBuffer;
  std::string line;
  int lineNumber = 0;
  char errbuf[512];
  char *buf;

  if (!weightFile.is_open ())
    {
      NS_LOG_WARN ("Couldn't open the file " << filename);
      return weights;
    }

  while (!weightFile.eof ())
    {
      int ret;
      char *argv[REGMATCH_MAX];

      lineNumber++;
      line.clear ();
      lineBuffer.clear ();

      getline (weightFile, line);
      if (weightFile.eof ())
        break;
      buf = (char *)line.c_str ();

      regmatch_t regmatch[REGMATCH_MAX];
      regex_t regex;

      ret = regcomp (&regex, ROCKETFUEL_WEIGHTS_LINE, REG_EXTENDED | REG_NEWLINE);
      if (ret != 0)
        {
          regerror (ret, &regex, errbuf, sizeof (errbuf));
          regfree (&regex);
          break;
        }

      ret = regexec (&regex, buf, REGMATCH_MAX, regmatch, 0);
      if (ret == REG_NOMATCH)
        {
          NS_LOG_WARN ("match failed (maps file): %s" << buf);
          regfree (&regex);
          break;
        }
      
      line = buf;
      
      /* regmatch[0] is the entire strings that matched */
      for (int i = 1; i < REGMATCH_MAX; i++)
        {
          if (regmatch[i].rm_so == -1)
            {
              argv[i - 1] = NULL;
            }
          else
            {
              line[regmatch[i].rm_eo] = '\0';
              argv[i - 1] = &line[regmatch[i].rm_so];
            }
        }

      std::string loc1 = argv[0];
      std::string loc2 = argv[2];
      std::string weight = argv[4];
      
      std::replace(loc1.begin(), loc1.end(), '+', ' ');
      std::replace(loc2.begin(), loc2.end(), '+', ' ');

      std::string key = loc1 + " -> " + loc2;
      
      if (weights.find(key) == weights.end())
        {
          NS_LOG_INFO (key << ": " << weight << "ms added");
          weights[key] = weight;
        }
      
      regfree (&regex);
    }

  weightFile.close ();
  return weights;
}

NodeContainer
RocketfuelTopologyReader::GenerateFromMapsFile (int argc, char *argv[])
{
  std::string uid;
  std::string loc;
  std::string ptr;
  std::string name;
  std::string nuid;
  bool dns = false;
  bool bb = false;
  int num_neigh_s = 0;
  unsigned int num_neigh = 0;
  //int radius = 0;
  std::vector <std::string> neigh_list;
  NodeContainer nodes;

  uid = argv[0];

  // Parse a normal-looking location string
  loc = argv[1];
  std::replace(loc.begin(), loc.end(), '+', ' ');
  loc = loc.substr(1); //cut off '@'

  if (argv[2])
    {
      dns = true;
    }

  if (argv[3])
    {
      bb = true;
    }

  num_neigh_s = ::atoi (argv[4]);
  if (num_neigh_s < 0)
    {
      num_neigh = 0;
      NS_LOG_WARN ("Negative number of neighbors given");
    }
  else
    {
      num_neigh = num_neigh_s;
    }

  /* internal neighbors */
  if (argv[6])
    {
      char *nbr;
      char *stringp = argv[6];
      while ((nbr = strsep (&stringp, " \t")) != NULL)
        {
          //need to cut off < and >
          nbr[strlen (nbr) - 1] = '\0';
          neigh_list.push_back (nbr + 1);
        }
    }
  if (num_neigh != neigh_list.size ())
    {
      NS_LOG_WARN ("Given number of neighbors = " << num_neigh << " != size of neighbors list = " << neigh_list.size ());
    }

  /* external neighbors */
  if (argv[7])
    {
      //      euid = argv[7];
    }

  /* name */
  if (argv[8])
    {
      name = argv[8];
      std::remove(name.begin(), name.end(), '!');
      //cut off '='
      if (name[name.size() - 1] == '!')
        name.erase(name.size() - 1);
    }

  //radius = ::atoi (&argv[9][0]);

  // Why are we skipping all nodes beyond radius 0??  This creates disconnected topologies because the
  // other nodes still get created but will not have any links
  /*if (radius > 0)
    {
      return nodes;
      }*/

  /* uid @loc [+] [bb] (num_neigh) [&ext] -> <nuid-1> <nuid-2> ... {-euid} ... =name[!] rn */
  NS_LOG_INFO ("Load Node[" << uid << "]: location: " << loc << " dns: " << dns
               << " bb: " << bb << " neighbors: " << neigh_list.size ()
               << "(" << "%d" << ") externals: \"%s\"(%d) "
               << "name: " << name
               //<< " radius: " << radius
               );

  //cast bb and dns to void, to suppress variable set but not used compiler warning
  //in optimized builds
  (void) bb;
  (void) dns;

  // Create node and link
  if (!uid.empty ())
    {
      if (nodeMap[uid] == 0)
        {
           Ptr<Node> tmpNode = CreateObject<Node> ();
          nodeMap[uid] = tmpNode;
          nodes.Add (tmpNode);
          nodesNumber++;
        }

      for (uint32_t i = 0; i < neigh_list.size (); ++i)
        {
          nuid = neigh_list[i];

          if (nuid.empty ())
            {
              return nodes;
            }

          if (nodeMap[nuid] == 0)
            {
              Ptr<Node> tmpNode = CreateObject<Node> ();
              nodeMap[nuid] = tmpNode;
              nodes.Add (tmpNode);
              nodesNumber++;
            }

          // Only create link if the neighbor didn't create it already!
          Link *link = linkMap[nuid + ">" + uid];
          if (linkMap[nuid + ">" + uid] == 0)
            {
              NS_LOG_INFO (linksNumber << ":" << nodesNumber << " From: " << uid << " to: " << nuid);
              link = new Link(nodeMap[uid], uid, nodeMap[nuid], nuid);

              link->SetAttribute("From Location", loc);
              //link->SetAttribute("From Address", name);
              link->SetAttribute("From Address", nodeAddresses[name]);

              linkMap[uid + ">" + nuid] = link;
            }
          else
            {
              link->SetAttribute("To Location", loc);
              //link->SetAttribute("To Address", name);
              link->SetAttribute("To Address", nodeAddresses[name]);

              //update links with full information now that we have it
              AddLink (*link);
              linksNumber++;
              linkMap[nuid + ">" + uid] = link;
            }
        }
    }
  return nodes;
}

NodeContainer
RocketfuelTopologyReader::GenerateFromWeightsFile (int argc, char *argv[])
{
  // Locations and uids are squished together unfortunately
  /* location1uid1 location2uid2 latency */
  std::string loc1 = argv[0];
  std::string uid1 = argv[1];
  std::string loc2 = argv[2];
  std::string uid2 = argv[3];
  std::string weight = argv[4];
  NodeContainer nodes;

  for (int i =0; i < argc; i++)
    NS_LOG_INFO(argv[i]);

  NS_LOG_INFO (linksNumber << ":" << nodesNumber << " From: " << uid1 << " (" << loc1 << ") to: " 
               << uid2 << " (" << loc2 << ") with weight: " << weight);

  std::replace(loc1.begin(), loc1.end(), '+', ' ');
  std::replace(loc2.begin(), loc2.end(), '+', ' ');

  // Create node and link
  if (!uid1.empty () && !uid2.empty ())
    {
      if (nodeMap[uid1] == 0)
        {
          Ptr<Node> tmpNode = CreateObject<Node> ();
          nodeMap[uid1] = tmpNode;
          nodes.Add (tmpNode);
          nodesNumber++;
        }

      if (nodeMap[uid2] == 0)
        {
          Ptr<Node> tmpNode = CreateObject<Node> ();
          nodeMap[uid2] = tmpNode;
          nodes.Add (tmpNode);
          nodesNumber++;
        }

      NS_LOG_INFO (linksNumber << ":" << nodesNumber << " From: " << uid1 << " (" << loc1 << ") to: " 
                   << uid2 << " (" << loc2 << ") with weight: " << weight);

      // Only create link if the neighbor didn't create it already!
      if (linkMap[uid2 + ">" + uid1] == 0)
        {
          NS_LOG_INFO (linksNumber << ":" << nodesNumber << " From: " << uid1 << " (" << loc1 << ") to: " 
                       << uid2 << " (" << loc2 << ") with weight: " << weight);

          Link *link = new Link(nodeMap[uid1], uid1, nodeMap[uid2], uid2);
          
          link->SetAttribute("From Location", loc1);
          link->SetAttribute("To Location", loc2);
          link->SetAttribute( (isLatencies ? "Latency" : "Weight"), weight);
          
          linkMap[uid1 + ">" + uid2] = link;
          AddLink (*link);
          linksNumber++;
        }
    }
  return nodes;
}

enum RocketfuelTopologyReader::RF_FileType
RocketfuelTopologyReader::GetFileType (const char *line)
{
  int ret;
  regmatch_t regmatch[REGMATCH_MAX];
  regex_t regex;
  char errbuf[512];

  // Check whether MAPS file or not
  ret = regcomp (&regex, ROCKETFUEL_MAPS_LINE, REG_EXTENDED | REG_NEWLINE);
  if (ret != 0)
    {
      regerror (ret, &regex, errbuf, sizeof (errbuf));
      return RF_UNKNOWN;
    }
  ret = regexec (&regex, line, REGMATCH_MAX, regmatch, 0);
  if (ret != REG_NOMATCH)
    {
      regfree (&regex);
      return RF_MAPS;
    }
  regfree (&regex);

  // Check whether Weights file or not
  ret = regcomp (&regex, ROCKETFUEL_WEIGHTS_LINE, REG_EXTENDED | REG_NEWLINE);
  if (ret != 0)
    {
      regerror (ret, &regex, errbuf, sizeof (errbuf));
      return RF_UNKNOWN;
    }
  ret = regexec (&regex, line, REGMATCH_MAX, regmatch, 0);
  if (ret != REG_NOMATCH)
    {
      regfree (&regex);
      return RF_WEIGHTS;
    }
  regfree (&regex);

  return RF_UNKNOWN;
}

void
RocketfuelTopologyReader::TryBuildAliases (void)
{
  std::ifstream aliasFile;
  std::string aliasFileName = GetFileName();
  std::string uid;
  std::string address;
  std::string name;
  size_t extIndex = aliasFileName.rfind (".cch");
  std::string line;
  size_t firstSpace;
  size_t secondSpace;

  if (extIndex == std::string::npos)
    {
      NS_LOG_WARN ("Could not find alias file for: " + aliasFileName);
      return;
    }

  aliasFileName.replace (extIndex, 4, ".al");
  aliasFile.open (aliasFileName.c_str ());
  
  if (!aliasFile.is_open())
    {
      NS_LOG_WARN ("Couldn't open the file " << aliasFileName);
    }

  NS_LOG_INFO ("Parsing alias file: " << aliasFileName);
  while (!aliasFile.eof ())
    {
      getline (aliasFile, line);
      
      if (line.empty ())
        break;

      firstSpace = line.find(" ");
      secondSpace = line.rfind(" ");
      uid = line.substr (0, firstSpace);
      address = line.substr (firstSpace + 1, secondSpace - firstSpace - 1);
      name = line.substr (secondSpace + 1);
      
      nodeAddresses[name] = address;
      NS_LOG_INFO ("Uid: " << uid << ", IP: " << address << ", Name: " << name);
    }

  aliasFile.close();
}

NodeContainer
RocketfuelTopologyReader::Read (void)
{
  std::ifstream topgen;
  topgen.open (GetFileName ().c_str ());
  NodeContainer nodes;

  std::istringstream lineBuffer;
  std::string line;
  int lineNumber = 0;
  enum RF_FileType ftype = RF_UNKNOWN;
  char errbuf[512];
  //char buf[2048];
  char *buf;

  if (!topgen.is_open ())
    {
      NS_LOG_WARN ("Couldn't open the file " << GetFileName ());
      return nodes;
    }

  while (!topgen.eof ())
    {
      int ret;
      int argc;
      char *argv[REGMATCH_MAX];

      lineNumber++;
      line.clear ();
      lineBuffer.clear ();

      getline (topgen, line);
      if (topgen.eof ())
        break;
      buf = (char *)line.c_str ();
      //strncpy(buf, (char *)line.c_str(), sizeof(buf));

      if (lineNumber == 1)
        {
          ftype = GetFileType (buf);
          if (ftype == RF_UNKNOWN)
            {
              NS_LOG_INFO ("Unknown File Format (" << GetFileName () << ")");
              break;
            }
          if (ftype == RF_MAPS)
            TryBuildAliases();

          // Determine whether it's weights or latencies
          else if (ftype == RF_WEIGHTS)
            {
              isLatencies = (GetFileName ().find("latencies") != std::string::npos);
            }
        }

      regmatch_t regmatch[REGMATCH_MAX];
      regex_t regex;

      if (ftype == RF_MAPS)
        {
          ret = regcomp (&regex, ROCKETFUEL_MAPS_LINE, REG_EXTENDED | REG_NEWLINE);
          if (ret != 0)
            {
              regerror (ret, &regex, errbuf, sizeof (errbuf));
              regfree (&regex);
              break;
            }

          ret = regexec (&regex, buf, REGMATCH_MAX, regmatch, 0);
          if (ret == REG_NOMATCH)
            {
              NS_LOG_WARN ("match failed (maps file): %s" << buf);
              regfree (&regex);
              break;
            }
        }
      else if (ftype == RF_WEIGHTS)
        {
          ret = regcomp (&regex, ROCKETFUEL_WEIGHTS_LINE, REG_EXTENDED | REG_NEWLINE);
          if (ret != 0)
            {
              regerror (ret, &regex, errbuf, sizeof (errbuf));
              regfree (&regex);
              break;
            }

          ret = regexec (&regex, buf, REGMATCH_MAX, regmatch, 0);
          if (ret == REG_NOMATCH)
            {
              NS_LOG_WARN ("match failed (weights file): %s" << buf);
              regfree (&regex);
              break;
            }
        }

      line = buf;
      argc = 0;

      /* regmatch[0] is the entire strings that matched */
      for (int i = 1; i < REGMATCH_MAX; i++)
        {
          if (regmatch[i].rm_so == -1)
            {
              argv[i - 1] = NULL;
            }
          else
            {
              line[regmatch[i].rm_eo] = '\0';
              argv[i - 1] = &line[regmatch[i].rm_so];
              argc = i;
            }
        }

      if (ftype == RF_MAPS)
        {
          nodes.Add (GenerateFromMapsFile (argc, argv));
        }
      else if (ftype == RF_WEIGHTS)
        {
          nodes.Add (GenerateFromWeightsFile (argc, argv));
        }
      else
        {
          NS_LOG_WARN ("Unsupported file format (only Maps/Weights are supported)");
        }

      regfree (&regex);
    }


  topgen.close ();

  NS_LOG_INFO ("Rocketfuel topology created with " << nodesNumber << " nodes and " << linksNumber << " links");
  return nodes;
}

} /* namespace ns3 */


