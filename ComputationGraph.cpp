#include "ComputationGraph.h"

#include <iostream>
#include <string>
#include "boost/property_tree/ptree.hpp"
#include "boost/property_tree/json_parser.hpp"
#include <vector>


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

    std::cout << "Caller:" << property.second.get<std::string>("caller") << "\n";
    info.caller = property.second.get<std::string>("caller");

    std::cout << "Inputs:";
    for (auto & input: property.second.get_child("inputs")){
      std::cout << " " << input.second.get_value < std::string > ();
      std::vector<std::string> decomposed;
      std::istringstream f(input.second.get_value < std::string > ());
      std::string s;
      while (getline(f, s, ':')) {
        decomposed.push_back(s);
      }
      struct dataInfo data;
      data.name = decomposed[0];
      data.size = std::stoi(decomposed[1]);
      info.inputs.insert(data);
    }
    std::cout << "\n";

    std::cout << "Outputs:";
    for (auto & output: property.second.get_child("outputs")){
      std::cout << " " << output.second.get_value < std::string > ();
      std::vector<std::string> decomposed;
      std::istringstream f(output.second.get_value < std::string > ());
      std::string s;
      while (getline(f, s, ':')) {
        decomposed.push_back(s);
      }
      struct dataInfo data;
      data.name = decomposed[0];
      data.size = std::stoi(decomposed[1]);
      info.outputs.insert(data);
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
  boost::property_tree::ptree outputs;


  std::set<struct dataInfo>::iterator it;
  for(it = info.inputs.begin(); it != info.inputs.end(); it++){
    std::stringstream ss;
    ss << it->name << ":" << it->size;
    boost::property_tree::ptree input_tree;
    input_tree.put("", ss.str());
    inputs.push_back(std::make_pair("", input_tree));
  }

  for(it = info.outputs.begin(); it != info.outputs.end(); it++){
    std::stringstream ss;
    ss << it->name << ":" << it->size;
    boost::property_tree::ptree output_tree;
    output_tree.put("", ss.str());
    outputs.push_back(std::make_pair("", output_tree));
  }

  pt.put("name", info.name);
  pt.put("type", info.type);
  pt.put("caller", info.caller);
  pt.add_child("inputs", inputs);
  pt.add_child("outputs", outputs);
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

std::string ComputationGraph::getLargestInputThunk(struct objectInfo info){
  int max_size = 0;
  std::string largest_name;
  for(auto & input: info.inputs){
    if(max_size < input.size){
      max_size = input.size;
      largest_name = input.name;
    }
  }

  //if there are not input params
  if(!max_size) return "";

  for(auto & item: items){
    for(auto & output: item.outputs){
      if(!output.name.compare(largest_name)){
        return item.thunk;
      }
    }
  }
  return "";
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

std::string JSON_STRING="[{\"name\": \"/exec/main/()\", \"type\": \"0\", \"inputs\": [], \"outputs\": [\"data1:10\", \"data2:20\"], \"thunk\": \"/node1/\", \"start\": 10, \"duration\": \"30\"}, {\"name\": \"f1\", \"type\": \"1\", \"inputs\": [\"data1:10\", \"data2:20\"], \"outputs\": [], \"thunk\": \"/node2/\", \"start\": 20, \"duration\": \"15\"}]";
int main(){
  ComputationGraph graph1;


//  graph1.updateGraph(JSON_STRING);

  std::ifstream t("locality.json");
  std::stringstream buffer;
  buffer << t.rdbuf();
  std::cout << "Got string:" << buffer.str() << "\n";
  graph1.updateGraph(buffer.str());

  //struct objectInfo info = graph1.getInfo("/exec/main/()");
  struct objectInfo info = graph1.getInfo("/exec/f1");
  std::cout << "Returned name: " << info.name << "\n";
  std::cout << "Returned updated: " << graph1.createUpdate(info) << "\n";
  std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~\n";
  std::cout << "Dumped graph1: " << graph1.dump();
  std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~\n";
  ComputationGraph graph2;
  graph2.updateGraph(graph1.dump());
  std::cout << "Dumped graph2: " << graph2.dump() << "\n";

  //std::cout << "Thunk of the large input param of f1: " << graph2.getLargestInputThunk(graph2.getInfo("f1")) << "\n";
  std::cout << "Thunk of the large input param of f1: " << graph2.getLargestInputThunk(graph2.getInfo("/exec/f2")) << "\n";

  return 0;
}
