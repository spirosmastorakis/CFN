#include "ComputationGraph.h"

#include <iostream>
#include <string>
#include "boost/property_tree/ptree.hpp"
#include "boost/property_tree/json_parser.hpp"


int ComputationGraph::updateGraph(std::string update){

  std::istringstream is(update);

  boost::property_tree::ptree pt;
  read_json(is, pt);

  for (auto & property: pt) {
    struct objectInfo info;
    std::cout << "Name:" << property.second.get<std::string>("name") << "\n";
    info.name = property.second.get<std::string>("name");

    std::cout << "Type:" << property.second.get<std::string>("type") << "\n";
    info.type = std::stoi(property.second.get<std::string>("type"));

    std::cout << "Inputs:";
    for (auto & input: property.second.get_child("inputs")){
      std::cout << " " << input.second.get_value < std::string > ();
      info.inputs.insert(input.second.get_value < std::string > ());
    }
    std::cout << "\n";

    std::cout << "Thunk:" << property.second.get<std::string>("thunk") << "\n";
    info.thunk = property.second.get<std::string>("thunk");

    info.start = std::stoi(property.second.get<std::string>("start"));
    std::cout << "Start:" << property.second.get<std::string>("start") << "\n";

    info.duration = std::stoi(property.second.get<std::string>("duration"));
    std::cout << "Duration:" << property.second.get<std::string>("duration") << "\n";

    items.insert(info);
  }

  return 0;
}


std::string ComputationGraph::createUpdate(struct objectInfo info){
  boost::property_tree::ptree pt;
  boost::property_tree::ptree inputs;
  boost::property_tree::ptree child1, child2, child3;


  std::set<std::string>::iterator it;
  for(it = info.inputs.begin(); it != info.inputs.end(); it++){
    boost::property_tree::ptree input_tree;
    input_tree.put("", *it);
    //std::cout << "Input: " << *it << "\n";
    inputs.push_back(std::make_pair("", input_tree));
  }

  pt.put("name", info.name);
  pt.put("type", info.type);
  pt.add_child("inputs", inputs);
  pt.put("thunk", info.thunk);
  pt.put("start", info.start);
  pt.put("duration", info.duration);

  std::stringstream ss;
  write_json(ss, pt);
  return ss.str();
}


std::string ComputationGraph::dump(){
  std::stringstream ss;
  ss << "[";
  bool first = true;
  for(auto& item: items){
    //std::cout << "Name:" << item.name << "\n";
    if(!first){
      ss << ", ";
    }else{
      first = false;
    }
    ss << this->createUpdate(item);
  }
  ss << "]";
  return ss.str();;
}

bool ComputationGraph::isStoredLocally(std::string name){
  struct objectInfo info;
  info.name = name;
  if(items.end() == items.find(info)){
      return false;
  }
  return true;

}

std::string ComputationGraph::getThunk(std::string name){
  struct objectInfo info;
  info.name = name;
  std::set<struct objectInfo>::iterator it = items.find(info);
  if(items.end() == it){
      return "";
  }
  return it->thunk;
}

struct objectInfo ComputationGraph::getInfo(std::string name){
  struct objectInfo emptyInfo;
  struct objectInfo info;
  info.name = name;
  std::set<struct objectInfo>::iterator it = items.find(info);
  if(items.end() == it){
      return emptyInfo;
  }
  return *it;
}

std::string JSON_STRING="[{\"name\": \"/exec/main/()\", \"type\": \"0\", \"inputs\": [\"input1\", \"input2\"], \"outputs\": [], \"thunk\": \"/node1/\", \"start\": 10, \"duration\": \"30\"}, {\"name\": \"f1\", \"type\": \"1\", \"inputs\": [\"input3\", \"input4\"], \"outputs\": [], \"thunk\": \"/node2/\", \"start\": 20, \"duration\": \"15\"}]";
int main(){
  ComputationGraph graph1;
  graph1.updateGraph(JSON_STRING);
  struct objectInfo info = graph1.getInfo("/exec/main/()");
  std::cout << "Returned name: " << info.name << "\n";
  std::cout << "Returned updated: " << graph1.createUpdate(info) << "\n";
  std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~\n";
  std::cout << "Dumped graph1: " << graph1.dump();
  std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~\n";
  ComputationGraph graph2;
  graph2.updateGraph(graph1.dump());
  std::cout << "Dumped graph2: " << graph2.dump() << "\n";
  return 0;
}
