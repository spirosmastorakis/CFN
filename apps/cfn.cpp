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

// custom-app.cpp

#include "cfn.hpp"

#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/callback.h"
#include "ns3/string.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ns3/integer.h"
#include "ns3/double.h"
#include "ns3/global-value.h"

#include "ns3/ndnSIM/helper/ndn-stack-helper.hpp"
#include "ns3/ndnSIM/helper/ndn-fib-helper.hpp"

#include "ns3/random-variable-stream.h"

#include <ndn-cxx/encoding/block-helpers.hpp>

#include "utils/ndn-ns3-packet-tag.hpp"
#include "utils/ndn-rtt-mean-deviation.hpp"

#include <ndn-cxx/lp/tags.hpp>
#include <ndn-cxx/lp/fields.hpp>

#include "boost/property_tree/ptree.hpp"
#include "boost/property_tree/json_parser.hpp"

// for convenience
using json = nlohmann::json;
using boost::property_tree::ptree;

static ns3::GlobalValue g_globalCounter =
  ns3::GlobalValue ("myGlobal", "My global value for json counter",
               ns3::IntegerValue (0),
               ns3::MakeIntegerChecker<int32_t>());

NS_LOG_COMPONENT_DEFINE("ndn.CFN");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(CFN);

// register NS-3 type
TypeId
CFN::GetTypeId()
{
  static TypeId tid = TypeId("ns3::ndn::CFN").SetGroupName("Ndn").SetParent<App>().AddConstructor<CFN>()
  .AddAttribute("Prefix", "Name of the Interest", StringValue("/"),
                MakeNameAccessor(&CFN::m_prefix), MakeNameChecker())
  .AddAttribute("Function", "Name of Function", StringValue("/"),
                MakeNameAccessor(&CFN::m_funcName), MakeNameChecker())
  .AddAttribute("Hint", "Forwarding hint of node", StringValue("/"),
                MakeNameAccessor(&CFN::m_hint), MakeNameChecker())
  .AddAttribute("NumNodes", "Number of nodes", UintegerValue(30),
                MakeUintegerAccessor(&CFN::m_nodes_num), MakeUintegerChecker<uint32_t>())
  .AddAttribute("EnableCallerRelation", "Caller relation enabled", UintegerValue(1),
                MakeUintegerAccessor(&CFN::m_caller_enabled), MakeUintegerChecker<uint32_t>())
  .AddAttribute("ParamK", "Value of param k", UintegerValue(10),
                MakeUintegerAccessor(&CFN::k), MakeUintegerChecker<uint32_t>())
  .AddAttribute("NumNode", "Number of this node", UintegerValue(0),
                MakeUintegerAccessor(&CFN::m_node_num), MakeUintegerChecker<uint32_t>());
  return tid;
}

CFN::CFN()
  : m_graph(root)
  , m_updateGen(CreateObject<UniformRandomVariable>())
  , m_rand(CreateObject<UniformRandomVariable>())
{
  // m_sourceNode = "root";
  // auto v = add_vertex(m_sourceNode, m_graph);
  // boost::add_edge(root, v, m_graph);
}

// Processing upon start of the application
void
CFN::StartApplication()
{
  // initialize ndn::App
  ndn::App::StartApplication();

  // Add FIB entries
  ndn::FibHelper::AddRoute(GetNode(), m_prefix, m_face, 0);
  ndn::FibHelper::AddRoute(GetNode(), m_hint, m_face, 0);
  ndn::FibHelper::AddRoute(GetNode(), m_funcName, m_face, 0);
  ndn::FibHelper::AddRoute(GetNode(), "/util", m_face, 0);
  //ndn::FibHelper::AddRoute(GetNode(), "/", m_face, 0);
  //ndn::FibHelper::AddRoute(GetNode(), "/", m_face, 0);

  // First update
  //std::ifstream t("simple-1000.json");
  std::ifstream t("locality2.json");
  std::stringstream buffer;
  buffer << t.rdbuf();
  //std::cout << "Got string:" << buffer.str() << "\n";
  //graph1.updateGraph(buffer.str());
  m_computegraph.updateGraph(buffer.str());
  //std::cout << m_computegraph.dump();

  m_val.Set(0);
  g_globalCounter.GetValue(m_val);

  if (m_caller_enabled == 1) {
    auto it = m_computegraph.findUpdate2(m_val.Get());
    if (it.caller == "") {
      m_called_functions.push_back(it.name);
      g_globalCounter.SetValue(ns3::IntegerValue(m_val.Get() + 1));
      //auto it = m_computegraph.items.begin();
      //std::advance(it, m_val.Get());
      //auto it = m_computegraph.findUpdate2(m_val.Get());
      NS_LOG_DEBUG("Global counter: " << m_val.Get());
      std::cout << "Scheduling Interest after: " << int(it.start) + 1 << "seconds\n";
      std::cout << "Scheduling Interest for: " << it.name << "\n";
      //Simulator::Schedule(Seconds(int(it.start) + 1), &CFN::DispatchRequest, this, m_val, false);
      Simulator::Schedule(Seconds(1), &CFN::DispatchRequest, this, m_val, false);
    }
  }
  else {
    NS_LOG_DEBUG("Global counter: " << m_val.Get());
    //auto it = m_computegraph.items.begin();
    //std::advance(it, m_val.Get());
    auto it = m_computegraph.findUpdate2(m_val.Get());
    g_globalCounter.SetValue(ns3::IntegerValue(m_val.Get() + 1));
    std::cout << "Scheduling Interest after: " << int(it.start) + 1 << "seconds\n";
    std::cout << "Scheduling Interest for: " << it.name << "\n";
    //Simulator::Schedule(Seconds(int(it.start) + 1), &CFN::DispatchRequest, this, m_val, false);
    Simulator::Schedule(Seconds(1), &CFN::DispatchRequest, this, m_val, false);
  }
  Simulator::Schedule(Seconds(0.2), &CFN::SendInitialInterest, this);

  //uint32_t updateTime = m_updateGen->GetValue(0, 2000);
  // Simulator::Schedule(MilliSeconds(updateTime), &CFN::GenerateUpdate, this);

  // Util Update Period
  m_period = ns3::Seconds(1);
  Simulator::Schedule(m_period, &CFN::sendUtilInterest, this);
  m_nodeToSend = "";
  m_cores = 4;
  m_util = 0;
  m_hopLimit = 2;
  is_node_stopped = false;

  // k = 10; // parameter k

  // std::ifstream i("/src/ndnSIM/apps/test.json");
  // json m_j;
  // i >> m_j;
}

// Processing when application is stopped
void
CFN::StopApplication()
{
  // cleanup ndn::App
  NS_LOG_DEBUG("App will stop");
  is_node_stopped = true;
  ndn::App::StopApplication();
}

// Callback that will be called when Interest arrives
void
CFN::OnInterest(std::shared_ptr<const ndn::Interest> interest)
{
  ndn::App::OnInterest(interest);

  NS_LOG_DEBUG("Received Interest packet for " << interest->getName());

  if (interest->getName().get(-2).toUri() == "initial") {
    std::cout << "Name(interest->getName().get(-1).toUri()).toUri(): " << Name(interest->getName().get(-1).toUri()).toUri() << std::endl;
    std::cout << "m_hint.toUri() " << m_hint.toUri() << std::endl;
    if (m_nodeToSend == "" && Name(interest->getName().get(-1).toUri()).toUri() != m_hint.toUri()) {
      m_nodeToSend = Name(interest->getName().get(-1).toUri());
    }
    //std::cout << "Node name: " << interest->getName().get(-1).toUri() << std::endl;
    if (interest->getName().get(-1).toUri() != m_hint.toUri()) {
      uint32_t hopCount = std::abs(int(m_node_num) - std::stoi(interest->getName().get(-3).toUri()));
      std::cout << "Hopcount: " << hopCount << std::endl;
      Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();
      if (rand->GetValue(0, 10) > 5)
        m_nodes.push_back(std::make_pair(Name(interest->getName().get(-1).toUri()), int(hopCount)));
      else
        m_nodes.insert(m_nodes.begin(), std::make_pair(Name(interest->getName().get(-1).toUri()), int(hopCount)));
      //send second initial request per node
      // Name nodeName = Name(interest->getName().get(-1).toUri());
      // NS_LOG_DEBUG("Will send 2nd initial request for " << nodeName);
      // nodeName.append(m_hint);
      // nodeName.append("initial");
      // auto interest = std::make_shared<ndn::Interest>(nodeName);
      // Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();
      // interest->setNonce(rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
      // interest->setInterestLifetime(ndn::time::seconds(1));
      // //interest->setMustBeFresh(true);
      //
      // m_transmittedInterests(interest, this, m_face);
      // m_appLink->onReceiveInterest(*interest);
    }
    return;
  }
  else if (interest->getName().get(-1).toUri() == "input") {
    auto data = std::make_shared<ndn::Data>(interest->getName());
    data->setFreshnessPeriod(ndn::time::milliseconds(0));
    data->setContent(std::make_shared< ::ndn::Buffer>(1024));
    ndn::StackHelper::getKeyChain().sign(*data);

    NS_LOG_DEBUG("Sending back input " << data->getName());
    m_transmittedDatas(data, this, m_face);

    m_appLink->onReceiveData(*data);
    return;
  }
  else if (interest->getName().size() >= 3 && interest->getName().get(2).toUri() == "update") {
    handleUpdate(interest);
  }
  else if (m_prefix.isPrefixOf(interest->getName())) {
    sendBackUpdate(interest);
    return;
  }
  else if (m_hint.isPrefixOf(interest->getName())) {
    // Interest for results
    Name interestName = interest->getName();
    interestName.append("results");
    auto data = std::make_shared<ndn::Data>(interestName);
    data->setFreshnessPeriod(ndn::time::milliseconds(1000));
    data->setContent(std::make_shared< ::ndn::Buffer>(1024));
    ndn::StackHelper::getKeyChain().sign(*data);

    NS_LOG_DEBUG("Sending back results " << data->getName());

    m_util--;
    m_transmittedDatas(data, this, m_face);

    m_appLink->onReceiveData(*data);
    return;
  }
  else if (interest->getName().get(0).toUri() == "util") {
    // Util Interest
    //NS_LOG_DEBUG("Received Util Interest" << *interest);
    handleUtilInterest(interest);
    return;
  }
  else {
    // Interest for function
    if (m_util == m_cores) {
      sendNackData(interest->getName());
      return;
    }
    // locality
    // check inputs
    Name func = interest->getName().getPrefix(2);
    std::cout << "func name: " << func.toUri() << std::endl;
    objectInfo i = m_computegraph.getInfo(func.toUri());
    std::cout << "object vector input: " << i.inputs.size() << std::endl;
    uint32_t maxInput = 0;
    std::string thunkOfMaxInput = "";
    std::vector<std::tuple<Name, int, int>> inputsVec;
    if (std::stoi(interest->getName().get(2).toUri()) > 100) {
      for (auto it = i.inputs.begin(); it != i.inputs.end(); it++) {
        std::cout << "input name: " << it->name << std::endl;
        objectInfo i2 = m_computegraph.getInfoInput(it->name);
        std::cout << "input name: " << i2.name << std::endl;
        std::cout << "input thunk: " << i2.thunk << std::endl;
        if (i2.thunk != "" && i2.thunk != "dupa") {
        //   if (it->size > maxInput) {
        //     maxInput = it->size;
        //     for (auto it2 = m_computegraph.items.begin(); it2 != m_computegraph.items.end(); it2++) {
        //       if (it2->outputs.begin()->name == it->name) {
        //         thunkOfMaxInput = it2->thunk;
        //         break;
        //       }
        //     }
        //   }
          if (((it->size) / k) == 0 && !m_hint.isPrefixOf(i2.thunk)) {
            inputsVec.push_back(std::make_tuple(Name(i2.thunk), 1, 0));
          }
          else if (!m_hint.isPrefixOf(i2.thunk)) {
            inputsVec.push_back(std::make_tuple(Name(i2.thunk), (it->size) / k, 0));
          }
          thunkOfMaxInput = m_computegraph.getLargestInputThunk(i2);
          continue;
        }
        else {
          std::cout << "sending retry Interest" << std::endl;
          sendRetryData(interest->getName());
          return;
        }
      }
      if (inputsVec.size() > 0) {
        m_inputState.push_back(std::make_tuple(interest->getName(), inputsVec));
        RequestInputs(interest->getName());
        return;
      }
    }
    m_util++;
    // delay for input fetching
    Name intName = interest->getName();
    // int hops = 0;
    // std::cout << "thunkOfMaxInput: " << thunkOfMaxInput << std::endl;
    // double delay = std::stoi(interest->getName().get(-1).toUri());
    // if (thunkOfMaxInput != "") {
    //   Name thunk = Name(thunkOfMaxInput);
    //   for (auto it = m_nodes.begin(); it != m_nodes.end(); it++) {
    //     std::cout << "it->first.toUri(): " << it->first.toUri() << std::endl;
    //     std::cout << "thunk.get(0).toUri(): " << thunk.get(0).toUri() << std::endl;
    //     if (it->first.isPrefixOf(thunk)) {
    //       hops = it->second;
    //       break;
    //     }
    //   }
    //   std::cout << "hops: " << hops << std::endl;
    int delay = std::stoi(interest->getName().get(3).toUri());
    //   std::cout << "delay: " << delay << std::endl;
    //   intName = Name(interest->getName().getPrefix(3).toUri());
    //   intName.append(std::to_string(delay));
    //   std::cout << "intName: " << intName.toUri() << std::endl;
    // }
    sendBackThunk(interest->getName(), delay);
    return;
  }

  auto data = std::make_shared<ndn::Data>(interest->getName());
  data->setFreshnessPeriod(ndn::time::milliseconds(1000));
  data->setContent(std::make_shared< ::ndn::Buffer>(1024));
  ndn::StackHelper::getKeyChain().sign(*data);

  NS_LOG_DEBUG("Sending ACK for CRDT update " << data->getName());

  // Call trace (for logging purposes)
  m_transmittedDatas(data, this, m_face);

  m_appLink->onReceiveData(*data);
}

// Callback that will be called when Data arrives
void
CFN::OnData(std::shared_ptr<const ndn::Data> data)
{
  if (data->getName().get(-1).toUri() == "NACK") {
    NS_LOG_DEBUG("Received NACK service response " << data->getName());
    Name functionName = Name(data->getName().getPrefix(-3).toUri());
    int counter = std::stoi(data->getName().get(2).toUri());
    std::cout << "counter: " << counter << std::endl;
    std::set<struct objectInfo>::iterator it;
    for (it = m_computegraph.items.begin(); it != m_computegraph.items.end(); it++) {
      //std::cout << "it->name: " << it->name << std::endl;
      //std::cout << "functionName.toUri() " << functionName.toUri() << std::endl;
      if (it->name == functionName.toUri()) {
        break;
      }
    }
    if(m_computegraph.items.end() == it) {
        NS_LOG_DEBUG("Could not find update to set thunk for: " << data->getName());
        return;
    }
    // if (m_computegraph.getLargestInputThunk(*it) != "" || m_computegraph.getLargestInputThunk(*it) != "dupa") {
    //   Simulator::Schedule(Seconds(1), &CFN::DispatchRequest, this, counter, true);
    //   return;
    // }
    else {
      if (m_nodes.size() == 1) {
        Simulator::Schedule(Seconds(5), &CFN::DispatchRequest, this, counter, true);
        return;
      }
      //auto it = std::find(m_nodes.begin(), m_nodes.end(), m_nodeToSend);
      // if (it == m_nodes.end() - 1) {
      //   it = m_nodes.begin();
      // }
      // else {
      //   m_nodeToSend = *(it+1);
      // }
      for (auto it2 = m_util_vector.end() - 1; it2 != m_util_vector.begin() - 1; it2--) {
        if (std::get<1>(*it2) < 1.0) {
          m_nodeToSend = std::get<0>(*it2);
          Simulator::Schedule(Seconds(1), &CFN::DispatchRequest, this, counter, true);
          return;
        }
      }
      Simulator::Schedule(Seconds(5), &CFN::DispatchRequest, this, counter, true);
    }
    return;
  }
  else if (data->getName().get(-1).toUri() == "input") {
    NS_LOG_DEBUG("Received Input data " << data->getName());
    Name taskName = Name(data->getName().getSubName(3, -1).toUri());
    //int seq_num = std::stoi(taskName.get(-2).toUri());
    //seq_num++;
    taskName = Name(taskName.getPrefix(-2).toUri());
    Name inputName = Name(data->getName().getPrefix(3).toUri());
    std::cout << "taskName: " << taskName.toUri() << std::endl;
    std::cout << "input: " << inputName.toUri() << std::endl;

    bool found = false;
    for (auto it = m_inputState.begin(); it != m_inputState.end(); it++) {
      if (std::get<0>(*it).toUri() == taskName.toUri()) {
        std::cout << "task found: " << std::get<0>(*it).toUri() << std::endl;
        found = true;
        for (auto it2 = std::get<1>(*it).begin(); it2 != std::get<1>(*it).end(); it2++) {
          std::cout << "input name: " << std::get<0>(*it2).toUri() << std::endl;
          std::cout << "input name received: " << inputName.toUri() << std::endl;
          if (std::get<0>(*it2).toUri() == inputName.toUri()) {
            if (std::get<2>(*it2) == std::get<1>(*it2)) {
              continue;
            }
            std::get<2>(*it2) = std::get<2>(*it2) + 1;
            std::cout << "Gotten: " << std::get<2>(*it2) << std::endl;
            std::cout << "Missing: " << std::get<1>(*it2) << std::endl;
            if (std::get<2>(*it2) < std::get<1>(*it2)) {
              Name newName = data->getName().getPrefix(-2);
              //newName.append(std::to_string(seq_num));
              SendInputInterest(newName);
              return;
            }
            break;
          }
        }
      }
    }

    if (!found) {
      return;
    }

    // check if we are done with inputs
    bool done = true;
    for (auto it = m_inputState.begin(); it != m_inputState.end(); it++) {
      if (std::get<0>(*it).toUri() == taskName.toUri()) {
        for (auto it2 = std::get<1>(*it).begin(); it2 != std::get<1>(*it).end(); it2++) {
          std::cout << "Input: " << std::get<0>(*it2).toUri() << std::endl;
          if (std::get<2>(*it2) < std::get<1>(*it2)) {
            std::cout << "Gotten: " << std::get<2>(*it2) << std::endl;
            std::cout << "Missing: " << std::get<1>(*it2) << std::endl;
            done = false;
            return;
          }
        }
        m_inputState.erase(it);
        break;
      }
    }
    if (done) {
      int delay = std::stoi(taskName.get(3).toUri());
      //m_util++;
      sendBackThunk(taskName, delay);
    }

    return;
  }
  else if (data->getName().get(-1).toUri() == "RETRY") {
    NS_LOG_DEBUG("Received RETRY service response " << data->getName());
    Name functionName = Name(data->getName().getPrefix(-2).toUri());
    int counter = std::stoi(data->getName().get(2).toUri());
    std::cout << "counter: " << counter << std::endl;
    std::set<struct objectInfo>::iterator it;
    for (it = m_computegraph.items.begin(); it != m_computegraph.items.end(); it++) {
      //std::cout << "it->name: " << it->name << std::endl;
      //std::cout << "functionName.toUri() " << functionName.toUri() << std::endl;
      if (it->name == functionName.toUri()) {
        break;
      }
      //counter++;
    }
    Simulator::Schedule(Seconds(1), &CFN::DispatchRequest, this, counter, true);
    return;
  }
  // else if (data->getName().get(-1).toUri() == "initial") {
  //   NS_LOG_DEBUG("Data for initial Interest " << data->getName());
  //   uint32_t hopCount = 0;
  //   auto hopCountTag = data->getTag<lp::HopCountTag>();
  //   if (hopCountTag != nullptr) { // e.g., packet came from local node's cache
  //     hopCount = *hopCountTag;
  //   }
  //   std::cout << "Hopcount: " << hopCount << std::endl;
  //   return;
  // }
  else if (data->getName().get(2).toUri() == "update") {
    NS_LOG_DEBUG("ACK for CRDT update " << data->getName());
    return;
  }
  else if (data->getName().get(-1).toUri() == "thunk") {
    NS_LOG_DEBUG("Received service response " << data->getName());
    // Decode thunk
    Block thunkBlock = data->getContent();
    thunkBlock.parse();
    Name thunkName;
    auto element = thunkBlock.elements_begin();
    //std::cout << "Element type: " << element->type() << std::endl;
    thunkName = Name(std::string((char*)element->value(), element->value_size()));
    Name functionName = Name(data->getName().getPrefix(-4).toUri());
    int counter = std::stoi(data->getName().get(-4).toUri());
    std::set<struct objectInfo>::iterator it;
    for (it = m_computegraph.items.begin(); it != m_computegraph.items.end(); it++) {
      //std::cout << "it->name: " << it->name << std::endl;
      //std::cout << "functionName.toUri() " << functionName.toUri() << std::endl;
      if (it->name == functionName.toUri()) {
        break;
      }
    }
    if(m_computegraph.items.end() == it){
        NS_LOG_DEBUG("Could not find update to set thunk for: " << data->getName());
        return;
    }
    it->thunk = thunkName.toUri();
    Simulator::Schedule(Seconds(std::stod(data->getName().get(-2).toUri())), &CFN::SendThunkInterest, this, data);
    GenerateUpdate(counter);
    g_globalCounter.GetValue(m_val);

    if (m_val.Get() < m_computegraph.items.size()) {
      if (m_caller_enabled == 0) {
        NS_LOG_DEBUG("Global counter: " << m_val.Get());
        g_globalCounter.SetValue(ns3::IntegerValue(m_val.Get() + 1));
        std::cout << "m_computegraph.items.size(): " << m_computegraph.items.size() << std::endl;
        Simulator::Schedule(Seconds(0.1), &CFN::DispatchRequest, this, m_val.Get(), false);
      }
      else {
        auto it2 = m_computegraph.findUpdate2(m_val.Get());
        for (auto i = m_called_functions.begin(); i != m_called_functions.end(); i++) {
          if (*i == it2.caller) {
            NS_LOG_DEBUG("Global counter: " << m_val.Get());
            m_called_functions.push_back(it2.name);
            g_globalCounter.SetValue(ns3::IntegerValue(m_val.Get() + 1));
            std::cout << "m_computegraph.items.size(): " << m_computegraph.items.size() << std::endl;
            Simulator::Schedule(Seconds(0.1), &CFN::DispatchRequest, this, m_val.Get(), false);
            break;
          }
        }
      }
    }
    return;
  }
  else if (data->getName().get(-1).toUri() == "results") {
    // received results
    NS_LOG_DEBUG("Received results for " << data->getName());
    return;
  }

  NS_LOG_DEBUG("DATA for missing CRDT update " << data->getName());

  Block content = data->getContent();
  content.parse();
  std::string updateHash;
  std::string sourceHash;
  std::string funcName;
  Type type;
  int dataSize;
  std::string hint;

  for (auto element = content.elements_begin(); element != content.elements_end(); ++element) {
    //std::cout << "In for loop" << std::endl;
    hint = std::string((char*)element->value(), element->value_size());
    element++;
    dataSize = std::stoi(std::string((char*)element->value(), element->value_size()));
    //std::cout << "dataSize: " << dataSize << std::endl;
    element++;
    type = Type(std::stoi(std::string((char*)element->value(), element->value_size())));
    //std::cout << "type: " << type << std::endl;
    element++;
    funcName = std::string((char*)element->value(), element->value_size());
    //std::cout << "funcName: " << funcName << std::endl;
    element++;
    sourceHash = std::string((char*)element->value(), element->value_size());
    //std::cout << "sourceHash: " << sourceHash << std::endl;
    element++;
    updateHash = std::string((char*)element->value(), element->value_size());
    //std::cout << "updateHash: " << updateHash << std::endl;
  }

  m_updates.push_back(std::make_tuple(updateHash, funcName, sourceHash, type, dataSize, hint));

  // find node in graph
  boost::graph_traits<Graph>::vertex_iterator vi, vi_end;
  boost::tie(vi, vi_end) = vertices(m_graph);
  auto vi_dest = findVertex(updateHash);
  auto vi_src = findVertex(sourceHash);
  if (vi_src == vi_end) {
      NS_LOG_WARN("Could not find vertex: " << m_sourceNode << " in the graph");
      // TODO: Request missing update
      requestMissingUpdate(sourceHash);
  }
  if (vi_dest == vi_end) {
    boost::add_edge(*vi_src, add_vertex(updateHash, m_graph), m_graph);
  }
  else {
    boost::add_edge(*vi_src, *vi_dest, m_graph);
  }

  // m_sourceNode = updateHash;
}

void
CFN::GenerateUpdate(int val)
{
  NS_LOG_DEBUG("Generating update");

  //auto it = m_computegraph.items.begin();
  auto info = m_computegraph.findUpdate2(val);
  //std::advance(it, val);
  //auto info = *it;
  // struct objectInfo info = m_computegraph.items[val];

  std::string str_hash = m_computegraph.createUpdate(info);
  std::hash<std::string> hasher;


  // Send Update
  // std::cout << "Creating CRDT update interest" << std::endl;
  Name interestName = Name(m_prefix.toUri());
  interestName.append("update").append(std::to_string(hasher(str_hash)));
  // interestName.append(m_sourceNode);
  // interestName.append(m_funcName);
  // interestName.append(std::to_string(type));
  // interestName.append(std::to_string(dataSize));
  // interestName.append(m_hint.toUri());
  auto interest = std::make_shared<ndn::Interest>(interestName);
  Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();
  interest->setNonce(rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
  interest->setInterestLifetime(ndn::time::seconds(1));

  //std::cout << "Hint: " << m_hint << std::endl;
  Block finalBlock(::ndn::tlv::Parameters);
  Block hintBlock = ::ndn::makeBinaryBlock(::ndn::tlv::Parameters, reinterpret_cast<const unsigned char *>(info.thunk.c_str()), info.thunk.size());
  finalBlock.push_back(hintBlock);
  //std::cout << "After first block creation" << std::endl;
  // Block inputSizeBlock = ::ndn::makeBinaryBlock(::ndn::tlv::Parameters, reinterpret_cast<const unsigned char *>(std::to_string(dataSize).c_str()), std::to_string(dataSize).length());
  // finalBlock.push_back(inputSizeBlock);

  Block typeBlock = ::ndn::makeBinaryBlock(::ndn::tlv::Parameters,reinterpret_cast<const unsigned char *>(std::to_string(info.type).c_str()), std::to_string(info.type).length());
  finalBlock.push_back(typeBlock);

  Block functionNameBlock = ::ndn::makeBinaryBlock(::ndn::tlv::Parameters,reinterpret_cast<const unsigned char *>(info.name.c_str()), info.name.size());
  finalBlock.push_back(functionNameBlock);

  // Block sourceHashBlock = ::ndn::makeBinaryBlock(::ndn::tlv::Parameters,reinterpret_cast<const unsigned char *>(m_sourceNode.c_str()), m_sourceNode.length());
  // finalBlock.push_back(sourceHashBlock);

  // encode inputs
  for (auto i = info.inputs.begin(); i != info.inputs.end(); i++) {
    //std::cout << "encode input" << std::endl;
    Block inputBlock = ::ndn::makeBinaryBlock(::ndn::tlv::InputTLV,reinterpret_cast<const unsigned char *>(i->name.c_str()), i->name.size());
    finalBlock.push_back(inputBlock);
  }

  // encode outputs
  for (auto i = info.outputs.begin(); i != info.outputs.end(); i++) {
    //std::cout << "encode input" << std::endl;
    Block outputBlock = ::ndn::makeBinaryBlock(::ndn::tlv::OutputTLV,reinterpret_cast<const unsigned char *>(i->name.c_str()), i->name.size());
    finalBlock.push_back(outputBlock);
  }

  Block startBlock = ::ndn::makeBinaryBlock(::ndn::tlv::Parameters,reinterpret_cast<const unsigned char *>(std::to_string(int(info.start)).c_str()), std::to_string(int(info.start)).length());
  finalBlock.push_back(startBlock);

  Block durationBlock = ::ndn::makeBinaryBlock(::ndn::tlv::Parameters,reinterpret_cast<const unsigned char *>(std::to_string(int(info.duration)).c_str()), std::to_string(int(info.duration)).length());
  finalBlock.push_back(durationBlock);

  Block hashBlock = ::ndn::makeBinaryBlock(::ndn::tlv::Parameters,reinterpret_cast<const unsigned char *>(str_hash.c_str()), str_hash.length());
  finalBlock.push_back(hashBlock);
  finalBlock.encode();

  interest->setParameters(finalBlock);

  NS_LOG_DEBUG("Sending CRDT update Interest for " << *interest);

  m_transmittedInterests(interest, this, m_face);
  m_appLink->onReceiveInterest(*interest);

  m_sourceNode = str_hash;

  // uint32_t updateTime = m_updateGen->GetValue(0, 2000);
  // Simulator::Schedule(MilliSeconds(updateTime), &CFN::GenerateUpdate, this);
}

boost::graph_traits<Graph>::vertex_iterator
CFN::findVertex(std::string vertexId)
{
  boost::graph_traits<Graph>::vertex_iterator vi, vi_end;
  for (boost::tie(vi, vi_end) = vertices(m_graph); vi != vi_end; ++vi) {
      //std::cout << "m_graph[*vi]: " << m_graph[*vi] << std::endl;
      //std::cout << "vertexId: " << vertexId << std::endl;
      if(m_graph[*vi] == vertexId) {
        return vi;
      }
  }
  return vi_end;
}

void
CFN::handleUpdate(std::shared_ptr<const ndn::Interest> interest)
{
  if (!interest->hasParameters()) {
    NS_LOG_DEBUG("Received CRDT Update Interest with no parameters" << interest->getName());
    return;
  }

  NS_LOG_DEBUG("Received CRDT Update Interest " << interest->getName());

  // decode parameters
  Block parameters = interest->getParameters();
  parameters.parse();

  std::string updateHash; //= interest->getName().get(3).toUri();
  // std::string sourceHash; // = interest->getName().get(4).toUri();
  // std::string funcName; //= interest->getName().get(5).toUri();
  // int type; // = Type(std::stoi(interest->getName().get(6).toUri()));
  // int dataSize; //= std::stoi(interest->getName().get(7).toUri());
  // std::string hint; // = interest->getName().get(8).toUri();
  // std::set<std::string> inputs;

  objectInfo info;

  // std::cout << "elements_size(): " << parameters.elements_size() << std::endl;
  for (auto element = parameters.elements_begin(); element != parameters.elements_end(); ++element) {
    //std::cout << "In for loop" << std::endl;
    info.thunk = std::string((char*)element->value(), element->value_size());
    element++;

    info.type = std::stoi(std::string((char*)element->value(), element->value_size()));
    //std::cout << "type: " << std::endl;
    element++;
    info.name = std::string((char*)element->value(), element->value_size());
    element++;

    //std::cout << "before loop" << std::endl;
    for (; element->type() == 45; element++) {
      //info.inputs.push_back(std::string((char*)element->value(), element->value_size());
      dataInfo dInfo;
      dInfo.name = std::string((char*)element->value(), element->value_size());
      dInfo.size = 0;
      info.inputs.insert(dInfo);
      //std::cout << "decoded input" << std::endl;
    }

    //std::cout << "before loop" << std::endl;
    for (; element->type() == 46; element++) {
      //info.inputs.push_back(std::string((char*)element->value(), element->value_size());
      dataInfo dInfo;
      dInfo.name = std::string((char*)element->value(), element->value_size());
      dInfo.size = 0;
      info.outputs.insert(dInfo);
      //std::cout << "decoded input" << std::endl;
    }

    //std::cout << "after loop" << std::endl;
    info.start = std::stoi(std::string((char*)element->value(), element->value_size()));
    element++;
    info.duration = std::stoi(std::string((char*)element->value(), element->value_size()));
    element++;
    //std::cout << "funcName: " << std::endl;
    //sourceHash = std::string((char*)element->value(), element->value_size());
    //std::cout << "sourceHash: " << sourceHash << std::endl;
    //element++;
    updateHash = std::string((char*)element->value(), element->value_size());
    //std::cout << "updateHash: " << updateHash << std::endl;
  }

  int counter = 0;
  std::set<struct objectInfo>::iterator it;
  for (it = m_computegraph.items.begin(); it != m_computegraph.items.end(); it++) {
    //std::cout << "it->name: " << it->name << std::endl;
    //std::cout << "functionName.toUri() " << info.name << std::endl;
    if (it->name == info.name) {
      break;
    }
    counter++;
  }
  if(m_computegraph.items.end() == it){
      NS_LOG_DEBUG("Received CRDT update, but could not find update to set thunk for: " << interest->getName());
      return;
  }
  std::cout << "CRDT update thunk: " << info.thunk << std::endl;
  it->thunk = info.thunk;
}

void
CFN::requestMissingUpdate(std::string updateHash)
{
  auto interest = std::make_shared<ndn::Interest>((Name(m_prefix.toUri()).append("update")).append(updateHash));
  Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();
  interest->setNonce(rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
  interest->setInterestLifetime(ndn::time::seconds(1));

  NS_LOG_DEBUG("Sending Interest for missing CRDT update " << *interest);

  m_transmittedInterests(interest, this, m_face);
  m_appLink->onReceiveInterest(*interest);
}

void
CFN::sendBackUpdate(std::shared_ptr<const ndn::Interest> interest)
{
  NS_LOG_DEBUG("Received request for missing CRDT update " << interest->getName());

  // look up if we have this update
  auto i = m_updates.begin();
  for (; i != m_updates.end(); i++) {
    if (std::get<0>(*i) == interest->getName().get(3).toUri()) {
      break;
    }
  }
  if (i == m_updates.end()) {
    NS_LOG_DEBUG("Do not have missing CRDT update " << interest->getName());
    return;
  }

  auto data = std::make_shared<ndn::Data>(interest->getName());
  data->setFreshnessPeriod(ndn::time::milliseconds(1000));

  Block finalBlock (reinterpret_cast<const unsigned char *>(std::get<5>(*i).toUri().c_str()), std::get<5>(*i).toUri().size());
  Block inputSizeBlock(reinterpret_cast<const unsigned char *>(std::to_string(std::get<4>(*i)).c_str()), std::to_string(std::get<4>(*i)).length());

  finalBlock.push_back(inputSizeBlock);

  Block typeBlock(reinterpret_cast<const unsigned char *>(std::to_string(std::get<3>(*i)).c_str()), std::to_string(std::get<3>(*i)).length());
  finalBlock.push_back(typeBlock);

  Block functionNameBlock(reinterpret_cast<const unsigned char *>(std::get<1>(*i).toUri().c_str()), std::get<1>(*i).toUri().size());
  finalBlock.push_back(functionNameBlock);

  Block sourceHashBlock(reinterpret_cast<const unsigned char *>(std::get<2>(*i).c_str()), std::get<2>(*i).length());
  finalBlock.push_back(sourceHashBlock);

  Block hashBlock(reinterpret_cast<const unsigned char *>(std::get<0>(*i).c_str()), std::get<0>(*i).length());
  finalBlock.push_back(hashBlock);
  finalBlock.encode();

  data->setContent(finalBlock);
  ndn::StackHelper::getKeyChain().sign(*data);

  NS_LOG_DEBUG("Sending missing CRDT update in data " << data->getName());

  // Call trace (for logging purposes)
  m_transmittedDatas(data, this, m_face);

  m_appLink->onReceiveData(*data);
}

void
CFN::DispatchRequest(ns3::IntegerValue val, bool pickNodeToSend)
{
  if (is_node_stopped) {
    return;
  }
  //auto it = m_computegraph.items.begin();
  //std::advance(it, val.Get());
  objectInfo info = m_computegraph.findUpdate2(val.Get());
  std::cout << "Val: " << val.Get() << std::endl;
  std::cout << "Name found: " << info.name << std::endl;
  // do scheduling
  std::string hintOfMaxData = m_computegraph.getLargestInputThunk(info);
  if (pickNodeToSend || hintOfMaxData == "" || hintOfMaxData == "dupa") {
    hintOfMaxData = m_nodeToSend.toUri();
  }
  if (m_hint.isPrefixOf(hintOfMaxData)) {
    // execute locally
    executeLocally(val);
  }
  // send function interest
  //std::hash<std::string> str_hash;
  NS_LOG_DEBUG("Will send request for " << info.name);
  Name intName = Name(info.name);
  intName.append(std::to_string(val.Get()));
  intName.append(std::to_string(info.duration / 10));
  auto interest = std::make_shared<ndn::Interest>(intName);
  Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();
  interest->setNonce(rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
  interest->setInterestLifetime(ndn::time::seconds(2));
  interest->setMustBeFresh(true);
  //std::cout << "Hint: " << hintOfMaxData << std::endl;
  ::ndn::DelegationList list({{10, Name(hintOfMaxData)}});
  interest->setForwardingHint(list);

  NS_LOG_DEBUG("Sending Interest for function " << *interest);

  m_transmittedInterests(interest, this, m_face);
  m_appLink->onReceiveInterest(*interest);
}

void
CFN::executeLocally(ns3::IntegerValue val)
{
  if (m_util >= m_cores) {
    Simulator::Schedule(Seconds(1), &CFN::executeLocally, this, val);
    return;
  }
  Simulator::Schedule(Seconds(1), &CFN::decrementUtil, this);
  m_util++;
  //auto it = m_computegraph.items.begin();
  // std::advance(it, val.Get());
  objectInfo info = m_computegraph.findUpdate2(val.Get());

  NS_LOG_DEBUG("Executing task locally " << info.name);
  std::cout << "val: " << val.Get() << std::endl;
  Name intName = Name(info.name);
  //intName.append(std::to_string(info.duration));
  Name thunkName = Name(m_hint.toUri());
  thunkName.append(intName);
  info.thunk = thunkName.toUri();

  for (auto it = m_computegraph.items.begin(); it != m_computegraph.items.end(); it++) {
    //std::cout << "it->name: " << it->name << std::endl;
    //std::cout << "functionName.toUri() " << functionName.toUri() << std::endl;
    if (it->name == intName.toUri()) {
      it->thunk = thunkName.toUri();
      break;
    }
    //counter++;
  }

  GenerateUpdate(val.Get());
  g_globalCounter.GetValue(m_val);

  NS_LOG_DEBUG("Global counter: " << m_val.Get());
  g_globalCounter.SetValue(ns3::IntegerValue(m_val.Get() + 1));
  std::cout << "m_computegraph.items.size(): " << m_computegraph.items.size() << std::endl;
  if (m_val.Get() < m_computegraph.items.size())
    Simulator::Schedule(Seconds(1), &CFN::DispatchRequest, this, m_val.Get(), false);
  return;
}

void
CFN::decrementUtil()
{
  m_util--;
}

void
CFN::sendBackThunk(Name interestName, double delay)
{
  Name thunkName = Name(m_hint.toUri());
  thunkName.append(interestName.getPrefix(2).toUri());
  interestName.append(std::to_string(delay));
  //std::cout << "Thunk name: " << thunkName.toUri() << std::endl;
  Block thunkBlock = ::ndn::makeBinaryBlock(::ndn::tlv::Name, reinterpret_cast<const unsigned char *>(thunkName.toUri().c_str()), thunkName.toUri().size());
  auto data = std::make_shared<ndn::Data>(interestName.append("thunk"));
  data->setFreshnessPeriod(ndn::time::milliseconds(1000));
  data->setContent(thunkBlock);
  ndn::StackHelper::getKeyChain().sign(*data);

  NS_LOG_DEBUG("Sending back thunk in data " << data->getName());

  // Call trace (for logging purposes)
  m_transmittedDatas(data, this, m_face);

  m_appLink->onReceiveData(*data);
}

void
CFN::SendThunkInterest(std::shared_ptr<const ndn::Data> data)
{
  Block thunkBlock = data->getContent();
  thunkBlock.parse();
  Name thunkName;
  auto element = thunkBlock.elements_begin();
  //std::cout << "Element type: " << element->type() << std::endl;
  thunkName = Name(std::string((char*)element->value(), element->value_size()));
  auto interest = std::make_shared<ndn::Interest>(thunkName);
  Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();
  interest->setNonce(rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
  interest->setInterestLifetime(ndn::time::seconds(1));
  interest->setMustBeFresh(true);

  NS_LOG_DEBUG("Sending Interest for thunk " << *interest);

  m_transmittedInterests(interest, this, m_face);
  m_appLink->onReceiveInterest(*interest);
}

void
CFN::sendUtilInterest()
{
  // util prefix
  Name name = Name("/util");
  // hint of node
  name.append(m_hint);
  // util
  name.append(std::to_string(double(1.0 * m_util/m_cores)));
  // hopCpunt
  name.append(std::to_string(0));

  auto interest = std::make_shared<ndn::Interest>(name);
  Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();
  interest->setNonce(rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
  interest->setInterestLifetime(ndn::time::seconds(1));

  //NS_LOG_DEBUG("Sending Util Interest" << *interest);

  m_transmittedInterests(interest, this, m_face);
  m_appLink->onReceiveInterest(*interest);

  Simulator::Schedule(m_period, &CFN::sendUtilInterest, this);
}

void
CFN::handleUtilInterest(std::shared_ptr<const ndn::Interest> interest)
{
  Name name = interest->getName();
  // handle utilization interest
  uint32_t hopCount = std::stoi(name.get(-1).toUri());
  double util = std::stod(name.get(-2).toUri());
  std::string hint = name.getSubName(1, name.size() - 3).toUri();

  if (hint == m_hint) {
    // ignore
    return;
  }

  //std::cout << "Hint: " << hint << std::endl;
  bool found = false;

  for (auto i = m_util_vector.begin(); i != m_util_vector.end(); i++) {
    if (std::get<0>(*i).toUri() == hint) {
      std::get<1>(*i) = util;
      found = true;
      break;
    }
  }

  if (!found) {
    m_util_vector.push_back(std::make_tuple(hint, util));
  }

  // std::cout << "hopCount: " << hopCount << std::endl;

  if (hopCount < m_hopLimit) {
    // util prefix
    Name name = Name("/util");
    // hint of node
    name.append(hint);
    // util
    name.append(std::to_string(util));
    // hopCpunt
    name.append(std::to_string(hopCount + 1));

    auto i = std::make_shared<ndn::Interest>(name);
    Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();
    i->setNonce(rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
    i->setInterestLifetime(ndn::time::seconds(1));

    //NS_LOG_DEBUG("Forwarding received Util Interest" << *i);
    m_transmittedInterests(i, this, m_face);
    m_appLink->onReceiveInterest(*i);
  }
}

void
CFN::SendInitialInterest() {
  Name name = Name(m_prefix.toUri());
  name.append(std::to_string(m_node_num));
  name.append("initial");
  name.append(m_hint.toUri());

  auto i = std::make_shared<ndn::Interest>(name);
  Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();
  i->setNonce(rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
  i->setInterestLifetime(ndn::time::seconds(1));

  NS_LOG_DEBUG("Sending Initial Interest" << *i);
  m_transmittedInterests(i, this, m_face);
  m_appLink->onReceiveInterest(*i);
}

void
CFN::sendNackData(Name name) {
  Name intName = Name(name.toUri());
  intName.append("NACK");

  auto data = std::make_shared<ndn::Data>(intName);
  data->setFreshnessPeriod(ndn::time::milliseconds(0));
  data->setContent(std::make_shared< ::ndn::Buffer>(1024));
  ndn::StackHelper::getKeyChain().sign(*data);

  NS_LOG_DEBUG("Sending NACK Data " << data->getName());

  // Call trace (for logging purposes)
  m_transmittedDatas(data, this, m_face);

  m_appLink->onReceiveData(*data);
}

void
CFN::sendRetryData(Name name) {
  Name intName = Name(name.toUri());
  intName.append("RETRY");

  auto data = std::make_shared<ndn::Data>(intName);
  data->setFreshnessPeriod(ndn::time::milliseconds(0));
  data->setContent(std::make_shared< ::ndn::Buffer>(1024));
  ndn::StackHelper::getKeyChain().sign(*data);

  NS_LOG_DEBUG("Sending RETRY Data " << data->getName());

  // Call trace (for logging purposes)
  m_transmittedDatas(data, this, m_face);

  m_appLink->onReceiveData(*data);
}

void
CFN::RequestInputs(Name taskName)
{
  int counter = 0;
  for (auto i = m_inputState.begin(); i != m_inputState.end(); i++) {
    if (std::get<0>(*i).toUri() == taskName.toUri()) {
      break;
    }
    counter++;
  }

  auto entry = m_inputState[counter];
  // start requesting inputs
  for (auto i = std::get<1>(entry).begin(); i != std::get<1>(entry).end(); i++) {
    Name n = Name(std::get<0>(*i).toUri());
    n.append(taskName);
    //n.append("1");
    SendInputInterest(n);
  }
}

void
CFN::SendInputInterest(Name inputName)
{
  Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();
  Name name = Name(inputName.toUri());
  name.append(std::to_string(rand->GetValue(0, std::numeric_limits<uint32_t>::max())));
  name.append("input");

  auto i = std::make_shared<ndn::Interest>(name);
  i->setNonce(rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
  i->setInterestLifetime(ndn::time::seconds(1));
  i->setMustBeFresh(true);

  NS_LOG_DEBUG("Sending Input Interest " << *i);
  m_transmittedInterests(i, this, m_face);
  m_appLink->onReceiveInterest(*i);
}


}
} // namespace ns3
