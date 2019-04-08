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

#include "ndn-sync.hpp"

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

#include "ns3/ndnSIM/helper/ndn-stack-helper.hpp"
#include "ns3/ndnSIM/helper/ndn-fib-helper.hpp"

#include "ns3/random-variable-stream.h"

#include <ndn-cxx/encoding/block-helpers.hpp>

NS_LOG_COMPONENT_DEFINE("ndn.Sync");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(Sync);

// register NS-3 type
TypeId
Sync::GetTypeId()
{
  static TypeId tid = TypeId("ns3::ndn::Sync").SetGroupName("Ndn").SetParent<App>().AddConstructor<Sync>()
  .AddAttribute("Prefix", "Name of the Interest", StringValue("/"),
                MakeNameAccessor(&Sync::m_prefix), MakeNameChecker())
  .AddAttribute("Function", "Name of Function", StringValue("/"),
                MakeNameAccessor(&Sync::m_funcName), MakeNameChecker())
  .AddAttribute("Hint", "Forwarding hint of node", StringValue("/"),
                MakeNameAccessor(&Sync::m_hint), MakeNameChecker());
  return tid;
}

Sync::Sync()
  : m_graph(root)
  , m_updateGen(CreateObject<UniformRandomVariable>())
  , m_rand(CreateObject<UniformRandomVariable>())
{
  m_sourceNode = "root";
  auto v = add_vertex(m_sourceNode, m_graph);
  boost::add_edge(root, v, m_graph);
}

// Processing upon start of the application
void
Sync::StartApplication()
{
  // initialize ndn::App
  ndn::App::StartApplication();

  // Add FIB entries
  ndn::FibHelper::AddRoute(GetNode(), m_prefix, m_face, 0);
  ndn::FibHelper::AddRoute(GetNode(), m_hint, m_face, 0);

  // First update
  uint32_t updateTime = m_updateGen->GetValue(0, 2000);
  Simulator::Schedule(MilliSeconds(updateTime), &Sync::GenerateUpdate, this);
}

// Processing when application is stopped
void
Sync::StopApplication()
{
  // cleanup ndn::App
  ndn::App::StopApplication();
}

// Callback that will be called when Interest arrives
void
Sync::OnInterest(std::shared_ptr<const ndn::Interest> interest)
{
  ndn::App::OnInterest(interest);

  NS_LOG_DEBUG("Received Interest packet for " << interest->getName());

  if (interest->getName().get(2).toUri() == "update") {
    handleUpdate(interest);
  }
  else {
    sendBackUpdate(interest);
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
Sync::OnData(std::shared_ptr<const ndn::Data> data)
{
  if (data->getName().get(2).toUri() == "update") {
    NS_LOG_DEBUG("ACK for CRDT update " << data->getName());
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
  
  m_sourceNode = updateHash;
}

void
Sync::GenerateUpdate()
{
  NS_LOG_DEBUG("Generating update");

  // Create a "big" string with all the info. Then get the hash of it
  // Info: source node in CRDT + function name + type + data size + forwarding hint of node
  // Random type, data size
  std::string accumulated_string;
  accumulated_string.append(m_sourceNode);
  accumulated_string.append(m_funcName.toUri());
  Type type = Type(m_rand->GetValue(1, 3));
  accumulated_string.append(std::to_string(type));
  uint32_t dataSize = m_rand->GetValue(100, 1024);
  accumulated_string.append(std::to_string(dataSize));
  accumulated_string.append(m_hint.toUri());

  std::size_t str_hash = std::hash<std::string>{}(accumulated_string);

  auto v_dest = add_vertex(std::to_string(str_hash), m_graph);
  auto vi = findVertex(m_sourceNode);
  boost::graph_traits<Graph>::vertex_iterator vi_test, vi_end;
  boost::tie(vi, vi_end) = vertices(m_graph);
  if (vi == vi_end) {
      NS_LOG_WARN("Could not find vertex: " << m_sourceNode << " in the graph");
      // TODO: Request missing update
      requestMissingUpdate(m_sourceNode);
      return;
  }
  boost::add_edge(*vi, v_dest, m_graph);

  m_updates.push_back(std::make_tuple(std::to_string(str_hash), m_funcName, m_sourceNode, type, dataSize, m_hint));

  // Send Update
  // std::cout << "Creating CRDT update interest" << std::endl;
  Name interestName = Name(m_prefix.toUri()).append("update").append(std::to_string(str_hash));
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
  Block hintBlock = ::ndn::makeBinaryBlock(::ndn::tlv::Parameters, reinterpret_cast<const unsigned char *>(m_hint.toUri().c_str()), m_hint.toUri().size());
  finalBlock.push_back(hintBlock);
  //std::cout << "After first block creation" << std::endl;
  Block inputSizeBlock = ::ndn::makeBinaryBlock(::ndn::tlv::Parameters, reinterpret_cast<const unsigned char *>(std::to_string(dataSize).c_str()), std::to_string(dataSize).length());
  finalBlock.push_back(inputSizeBlock);

  Block typeBlock = ::ndn::makeBinaryBlock(::ndn::tlv::Parameters,reinterpret_cast<const unsigned char *>(std::to_string(type).c_str()), std::to_string(type).length());
  finalBlock.push_back(typeBlock);

  Block functionNameBlock = ::ndn::makeBinaryBlock(::ndn::tlv::Parameters,reinterpret_cast<const unsigned char *>(m_funcName.toUri().c_str()), m_funcName.toUri().size());
  finalBlock.push_back(functionNameBlock);

  Block sourceHashBlock = ::ndn::makeBinaryBlock(::ndn::tlv::Parameters,reinterpret_cast<const unsigned char *>(m_sourceNode.c_str()), m_sourceNode.length());
  finalBlock.push_back(sourceHashBlock);

  Block hashBlock = ::ndn::makeBinaryBlock(::ndn::tlv::Parameters,reinterpret_cast<const unsigned char *>(std::to_string(str_hash).c_str()), std::to_string(str_hash).length());
  finalBlock.push_back(hashBlock);
  finalBlock.encode();

  interest->setParameters(finalBlock);

  NS_LOG_DEBUG("Sending CRDT update Interest for " << *interest);

  m_transmittedInterests(interest, this, m_face);
  m_appLink->onReceiveInterest(*interest);

  m_sourceNode = std::to_string(str_hash);

  uint32_t updateTime = m_updateGen->GetValue(0, 2000);
  Simulator::Schedule(MilliSeconds(updateTime), &Sync::GenerateUpdate, this);
}

boost::graph_traits<Graph>::vertex_iterator
Sync::findVertex(std::string vertexId)
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
Sync::handleUpdate(std::shared_ptr<const ndn::Interest> interest)
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
  std::string sourceHash; // = interest->getName().get(4).toUri();
  std::string funcName; //= interest->getName().get(5).toUri();
  Type type; // = Type(std::stoi(interest->getName().get(6).toUri()));
  int dataSize; //= std::stoi(interest->getName().get(7).toUri());
  std::string hint; // = interest->getName().get(8).toUri();

  // std::cout << "elements_size(): " << parameters.elements_size() << std::endl;
  for (auto element = parameters.elements_begin(); element != parameters.elements_end(); ++element) {
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
      NS_LOG_WARN("Could not find vertex: " << sourceHash << " in the graph");
      // TODO: Request missing update
      requestMissingUpdate(sourceHash);
  }
  if (vi_dest == vi_end) {
    boost::add_edge(*vi_src, add_vertex(updateHash, m_graph), m_graph);
  }
  else {
    boost::add_edge(*vi_src, *vi_dest, m_graph);
  }

  m_sourceNode = updateHash;
}

void
Sync::requestMissingUpdate(std::string updateHash)
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
Sync::sendBackUpdate(std::shared_ptr<const ndn::Interest> interest)
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

}
} // namespace ns3
