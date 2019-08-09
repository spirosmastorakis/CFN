#ifndef JSON_PARSER_H
#define JSON_PARSER_H

#include <string>
#include <set>

#include <iostream>
#include <string>
#include "boost/property_tree/ptree.hpp"
#include "boost/property_tree/json_parser.hpp"

enum Types { TRANSPARENT, OPAQUE, DATA};

struct dataInfo{
  std::string name;
  int size;
};

struct objectInfo{
  int start;
  std::string name;
  int type;
  std::string caller;
  mutable std::string thunk;
  std::set<struct dataInfo> inputs;
  std::set<struct dataInfo> outputs;
  unsigned int duration;
};

inline bool operator<(const dataInfo& lhs, const dataInfo& rhs)
{
 return lhs.name < rhs.name;
}

inline bool operator<(const objectInfo& lhs, const objectInfo& rhs)
{
   return lhs.start < rhs.start;
}

class ComputationGraph{
public:
    /*
    * update the computation graph with an update expressed as a JSON string
    */
    int updateGraph(std::string update) {
      std::istringstream is(update);

      boost::property_tree::ptree pt;
      read_json(is, pt);

      for (auto & property: pt) {
        struct objectInfo info;
        //std::cout << "Name:" << property.second.get<std::string>("name") << "\n";
        info.name = property.second.get<std::string>("name");

        //std::cout << "Type:" << property.second.get<std::string>("type") << "\n";
        info.type = std::stoi(property.second.get<std::string>("type"));

        //std::cout << "Caller:" << property.second.get<std::string>("caller") << "\n";
        info.caller = property.second.get<std::string>("caller");

        //std::cout << "Inputs:";
        for (auto & input: property.second.get_child("inputs")){
          //std::cout << " " << input.second.get_value < std::string > ();
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
        //std::cout << "\n";

        //std::cout << "Outputs:";
        for (auto & output: property.second.get_child("outputs")){
          //std::cout << " " << output.second.get_value < std::string > ();
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
        //std::cout << "\n";


        //std::cout << "Thunk:" << property.second.get<std::string>("thunk") << "\n";
        info.thunk = property.second.get<std::string>("thunk");

        info.start = std::stoi(property.second.get<std::string>("start"));
        //std::cout << "Start:" << property.second.get<std::string>("start") << "\n";

        info.duration = std::stoi(property.second.get<std::string>("duration"));
        //std::cout << "Duration:" << property.second.get<std::string>("duration") << "\n";

        items.insert(info);
      }

      return 0;
    }

    /*
    * create a JSON update from an objectInfo to send to others
    */
    std::string createUpdate(struct objectInfo info) {
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
      pt.add_child("inputs", inputs);
      pt.add_child("outputs", outputs);
      pt.put("thunk", info.thunk);
      pt.put("start", info.start);
      pt.put("duration", info.duration);

      std::stringstream ss;
      write_json(ss, pt);
      return ss.str();
    }

    std::string dump(){
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
      return ss.str();
    }

    /*
    * check if an entry for given name is present in the computation graph
    */
    bool isStoredLocally(std::string name) {
      struct objectInfo info;
      info.name = name;
      if(items.end() == items.find(info)){
          return false;
      }
      return true;
    }

    /*
    * get a thunk for given name
    */
    std::string getThunk(std::string name) {
      struct objectInfo info;
      info.name = name;
      //std::set<struct objectInfo>::iterator it = items.find(info);
      for (auto it = items.begin(); it != items.end(); it++) {
          if (it->name == name) {
            return it->thunk;
          }
      }
      return "";
    }

    /*
    * get info about object with a given name
    */
    struct objectInfo getInfo(std::string name) {
      struct objectInfo emptyInfo;
      struct objectInfo info;
      info.name = name;
      //std::set<struct objectInfo>::iterator it = items.find(info);
      //if(items.end() == it){
      //    return emptyInfo;
      //}
      for (auto it = items.begin(); it != items.end(); it++) {
          if (it->name == name) {
            return *it;
          }
      }
      return emptyInfo;
    }

    struct objectInfo getInfoInput(std::string name) {
      struct objectInfo emptyInfo;
      struct objectInfo info;
      info.name = name;
      for (auto it = items.begin(); it != items.end(); it++) {
        for (auto it2 = it->outputs.begin(); it2 != it->outputs.end(); it2++) {
          if (it2->name == name) {
            return *it;
          }
        }
      }
      return emptyInfo;
    }

    struct objectInfo findUpdate(int i) {
      std::string func = "/exec/f";
      struct objectInfo emptyInfo;
      for (auto it = items.begin(); it != items.end(); it++) {
        if (it->name == func + std::to_string(i)) {
          return *it;
        }
        //return *it;
      }
      return emptyInfo;
    }

    struct objectInfo findUpdate2(int i) {
      std::string func = "/exec/f";
      struct objectInfo emptyInfo;
      int counter = 0;
      for (auto it = items.begin(); it != items.end(); it++) {
        // if (it->name == func + std::to_string(i)) {
        //   return *it;
        // }
        if (counter == i)
          return *it;
        counter++;
      }
      return emptyInfo;
    }

    int getUpdateSeqNum(std::string name) {
      int counter = 0;
      for (auto it = items.begin(); it != items.end(); it++) {
        if (it->name == name) {
          return counter;
        }
        counter++;
      }
    }

    std::string getLargestInputThunk(struct objectInfo info){
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

  public:
    std::set<struct objectInfo> items;
    //std::set<struct objectInfo, bool(*)(const struct objectInfo&, const struct objectInfo&)> items(&comparator);


};

#endif

/*
updateGraph(update) - you just give me a new update when you receive one
isStoredLocally(hash) - check if we have the required update. if we don't you'll have to request it,
getThunk(name) - I return you a thunk for given data
*/
