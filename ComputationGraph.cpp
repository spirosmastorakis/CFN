#include "ComputationGraph.h"

#include <iostream>
#include <string>
#include "boost/property_tree/ptree.hpp"
#include "boost/property_tree/json_parser.hpp"

int ComputationGraph::updateGraph(std::string update){

  std::istringstream is(update);

  boost::property_tree::ptree pt;
  read_json(is, pt);

  bool flag = false;
  struct objectInfo info;
  for (auto & property: pt) {
    if(flag){
      std::cout << "hash: " << property.second.get_value < std::string > () << "\n";
    }else{
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
      /*std::cout << "Outputs:";
      for (auto & output: property.second.get_child("outputs")){
        std::cout << " " << output.second.get_value < std::string > ();
      }
      std::cout << "\n";
      std::cout << "Depends:";
      for (auto & depend: property.second.get_child("depends")){
        std::cout << " " << depend.second.get_value < std::string > ();
      }
      std::cout << "\n";*/
    }
          flag = !flag;
  }

  items.insert(info);


  return 0;
}


std::string ComputationGraph::createUpdate(struct objectInfo info){
  /*ptree pt;
  pt.put ("name", "bar");
  std::ostringstream buf;
  write_json (buf, pt, false);
  std::string json = buf.str(); // {"foo":"bar"}*/
  return "";

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

std::string JSON_STRING="[{\"name\": \"/exec/main/()\", \"type\": \"0\", \"inputs\": [\"input1\", \"input2\"], \"outputs\": [], \"thunk\": \"/node1/\"}, \"c97a021a755c03c8402319f7ed8a344a\"]";
int main(){
  ComputationGraph graph;
  graph.updateGraph(JSON_STRING);
  return 0;
}
