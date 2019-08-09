/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2011-2015  Regents of the University of California.
 *
 * This file is part of ndnSIM. See AUTHORS for complete list of ndnSIM authors and
 * contributors.
 *
 * ndnSIM is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * ndnSIM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndnSIM, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 **/

// ndn-simple.cpp

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"

namespace ns3 {

/**
 * This scenario simulates a very simple network topology:
 *
 *
 *      +----------+     1Mbps      +--------+     1Mbps      +----------+
 *      | consumer | <------------> | router | <------------> | producer |
 *      +----------+         10ms   +--------+          10ms  +----------+
 *
 *
 * Consumer requests data from producer with frequency 10 interests per second
 * (interests contain constantly increasing sequence number).
 *
 * For every received interest, producer replies with a data packet, containing
 * 1024 bytes of virtual payload.
 *
 * To run scenario and see what is happening, use the following command:
 *
 *     NS_LOG=ndn.Consumer:ndn.Producer ./waf --run=ndn-simple
 */

int
main(int argc, char* argv[])
{
  // setting default parameters for PointToPoint links and channels
  Config::SetDefault("ns3::PointToPointNetDevice::DataRate", StringValue("1Gbps"));
  Config::SetDefault("ns3::PointToPointChannel::Delay", StringValue("10ms"));
  Config::SetDefault("ns3::QueueBase::MaxSize", StringValue("20000p"));

  // Read optional command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
  CommandLine cmd;
  cmd.Parse(argc, argv);

  // Creating nodes
  NodeContainer nodes;
  nodes.Create(3);

  // Connecting nodes using two links
  PointToPointHelper p2p;
  p2p.Install(nodes.Get(0), nodes.Get(1));
  p2p.Install(nodes.Get(1), nodes.Get(2));
  // p2p.Install(nodes.Get(2), nodes.Get(3));
  // p2p.Install(nodes.Get(3), nodes.Get(4));

  // Install NDN stack on all nodes
  ndn::StackHelper ndnHelper;
  //ndnHelper.SetDefaultRoutes(true);
  ndnHelper.InstallAll();

  // Installing global routing interface on all nodes
  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll();

  // Choosing forwarding strategy
  ndn::StrategyChoiceHelper::InstallAll("/graph/program", "/localhost/nfd/strategy/multicast");
  ndn::StrategyChoiceHelper::InstallAll("/function", "/localhost/nfd/strategy/best-route");
  ndn::StrategyChoiceHelper::InstallAll("/util", "/localhost/nfd/strategy/multicast");

  // Installing applications

  // Consumer
  ndn::AppHelper consumerHelper("ns3::ndn::SyncRealGraph");
  // Consumer will request /prefix/0, /prefix/1, ...
  consumerHelper.SetPrefix("/graph/program");
  consumerHelper.SetAttribute("Function", StringValue("/function"));
  consumerHelper.SetAttribute("Hint", StringValue("/consumer"));
  auto apps = consumerHelper.Install(nodes.Get(0));                        // first node
  //apps.Stop(Seconds(10.0)); // stop the consumer app at 10 seconds mark

  // Producer
  ndn::AppHelper producerHelper("ns3::ndn::SyncRealGraph");
  // Producer will reply to all requests starting with /prefix
  producerHelper.SetPrefix("/graph/program");
  producerHelper.SetAttribute("Function", StringValue("/function"));
  producerHelper.SetAttribute("Hint", StringValue("/producer"));
  producerHelper.Install(nodes.Get(2)); // last node

  // Producer
  // ndn::AppHelper testHelper("ns3::ndn::SyncRealGraph");
  // // Producer will reply to all requests starting with /prefix
  // testHelper.SetPrefix("/graph/program");
  // testHelper.SetAttribute("Function", StringValue("/function"));
  // testHelper.SetAttribute("Hint", StringValue("/test"));
  // testHelper.Install(nodes.Get(3)); // last node
  //
  // // Producer
  // ndn::AppHelper testHelper2("ns3::ndn::SyncRealGraph");
  // // Producer will reply to all requests starting with /prefix
  // testHelper2.SetPrefix("/graph/program");
  // testHelper2.SetAttribute("Function", StringValue("/function"));
  // testHelper2.SetAttribute("Hint", StringValue("/test2"));
  // testHelper2.Install(nodes.Get(4)); // last node

  ndnGlobalRoutingHelper.AddOrigins("/graph/program", nodes.Get(0));
  ndnGlobalRoutingHelper.AddOrigins("/function", nodes.Get(0));
  ndnGlobalRoutingHelper.AddOrigins("/graph/program", nodes.Get(2));
  ndnGlobalRoutingHelper.AddOrigins("/util", nodes.Get(2));
  ndnGlobalRoutingHelper.AddOrigins("/util", nodes.Get(0));

  ndnGlobalRoutingHelper.AddOrigins("/consumer", nodes.Get(0));
  ndnGlobalRoutingHelper.AddOrigins("/function", nodes.Get(2));
  ndnGlobalRoutingHelper.AddOrigins("/producer", nodes.Get(2));

  // ndnGlobalRoutingHelper.AddOrigins("/test", nodes.Get(3));
  // ndnGlobalRoutingHelper.AddOrigins("/function", nodes.Get(3));
  // ndnGlobalRoutingHelper.AddOrigins("/graph/program", nodes.Get(3));
  // ndnGlobalRoutingHelper.AddOrigins("/util", nodes.Get(3));
  //
  // ndnGlobalRoutingHelper.AddOrigins("/test2", nodes.Get(4));
  // ndnGlobalRoutingHelper.AddOrigins("/function", nodes.Get(4));
  // ndnGlobalRoutingHelper.AddOrigins("/graph/program", nodes.Get(4));
  // ndnGlobalRoutingHelper.AddOrigins("/util", nodes.Get(4));

  // Calculate and install FIBs
  ndn::GlobalRoutingHelper::CalculateAllPossibleRoutes();

  Simulator::Stop(Seconds(1600.0));

  Simulator::Run();
  Simulator::Destroy();

  return 0;
}

} // namespace ns3

int
main(int argc, char* argv[])
{
  return ns3::main(argc, argv);
}
