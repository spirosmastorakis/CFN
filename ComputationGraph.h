#ifndef JSON_PARSER_H
#define JSON_PARSER_H

#include <string>

enum Types { TRANSPARENT, OPAQUE, DATA};

struct objectInfo{
  std::string name;
  int type;
  std::set<std::string> dependencies;
};

class ComputationGraph{
    /*
    * update the computation graph with an update expressed as a JSON string
    */
    int updateGraph(std::string update);

    /*
    * create a JSON update from an objectInfo to send to others
    */
    std::string createUpdate(struct objectInfo info);

    /*
    * check if an entry for given name is present in the computation graph
    */
    bool isStoredLocally(std::string name);

    /*
    * get a thunk for given name
    */
    std::string getThunk(std::string name);

    /*
    * get info about object with a given name
    */
    struct objectInfo getInfo(std::string name);

};

#endif

/*
updateGraph(update) - you just give me a new update when you receive one
isStoredLocally(hash) - check if we have the required update. if we don't you'll have to request it,
getThunk(name) - I return you a thunk for given data
*/
