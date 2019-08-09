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

// custom-app.hpp

#ifndef NDN_CFNProactiveOld_H
#define NDN_CFNProactiveOld_H

#include "ns3/ndnSIM/apps/ndn-app.hpp"
#include "ns3/random-variable-stream.h"

#include "ns3/uinteger.h"
#include "ns3/integer.h"


#include <iostream>
#include <utility>
#include <algorithm>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/adjacency_matrix.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/property_map/transform_value_property_map.hpp>

#include <nlohmann/json.hpp>

#include "computation-graph.hpp"

// for convenience
using json = nlohmann::json;

namespace ns3 {
namespace ndn {

enum Type {stateless, stateful, data};
typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS, std::string> Graph;
enum rootEnum { root };

/**
 * @brief A simple custom application
 *
 * This applications demonstrates how to send Interests and respond with Datas to incoming interests
 *
 * When application starts it "sets interest filter" (install FIB entry) for /prefix/sub, as well as
 * sends Interest for this prefix
 *
 * When an Interest is received, it is replied with a Data with 1024-byte fake payload
 */
class CFNProactiveOld : public App {
public:
  // register NS-3 type "CFNProactiveOld"
  static TypeId
  GetTypeId();

  CFNProactiveOld();

  // (overridden from ndn::App) Processing upon start of the application
  virtual void
  StartApplication();

  // (overridden from ndn::App) Processing when application is stopped
  virtual void
  StopApplication();

  // (overridden from ndn::App) Callback that will be called when Interest arrives
  virtual void
  OnInterest(std::shared_ptr<const ndn::Interest> interest);

  // (overridden from ndn::App) Callback that will be called when Data arrives
  virtual void
  OnData(std::shared_ptr<const ndn::Data> contentObject);

  void
  GenerateUpdate(int val);

  boost::graph_traits<Graph>::vertex_iterator
  findVertex(std::string vertexId);

  void
  handleUpdate(std::shared_ptr<const ndn::Interest> interest);

  void
  requestMissingUpdate(std::string updateHash);

  void
  sendBackUpdate(std::shared_ptr<const ndn::Interest> interest);

  void
  DispatchRequest(ns3::IntegerValue val, bool pickNodeToSend, bool re_execution = false);

  void
  sendBackThunk(Name interestName, double delay);

  void
  SendThunkInterest(std::shared_ptr<const ndn::Data> data);

  void
  sendUtilInterest();

  void
  handleUtilInterest(std::shared_ptr<const ndn::Interest> interest);

  void
  SendInitialInterest();

  void
  sendNackData(Name name);

  void
  sendRetryData(Name name);

  void
  executeLocally(ns3::IntegerValue val);

  void
  decrementUtil();

  void
  RequestInputs(Name taskName);

  void
  SendInputInterest(Name inputName);

  void
  NodeFailed(Name name);

private:
  Name m_prefix;
  Name m_hint;
  Name m_funcName;

  Graph m_graph;

  std::string m_sourceNode;

  Ptr<UniformRandomVariable> m_updateGen; // random update generator
  Ptr<UniformRandomVariable> m_rand; // random num generator

  // store CRDT updates here
  // hash, function, source node in graph, type, data size, hint
  std::vector<std::tuple<std::string, Name, std::string, Type, uint32_t, Name>> m_updates;

  std::vector<std::tuple<Name, double>> m_util_vector;

  json m_j;

  ns3::Time m_period;

  uint32_t m_cores;
  uint32_t m_util;

  uint32_t m_hopLimit;

  ComputationGraph m_computegraph;

  ns3::IntegerValue m_val;

  // vector of name, hops
  std::vector<std::pair<Name, int>> m_nodes;

  Name m_nodeToSend;

  uint32_t m_nodes_num;

  uint32_t m_node_num;

  bool is_node_stopped;

  // <task name, received inputs, total inputs>
  std::vector<std::tuple<Name, std::vector<std::tuple<Name, int, int>>>> m_inputState;

  int k;

  int m_caller_enabled;

  std::vector<std::string> m_called_functions;

  // event id, task name, hint name,
  std::vector<std::tuple<ns3::EventId, Name, Name>> m_outstandingInterests;

  std::vector<Name> m_deadNodes;

  int m_numTasks;
};

}
} // namespace ns3

#endif // CUSTOM_APP_H_
