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

    int updateGraph(std::string update);

    bool isStoredLocally(std::string name);

    std::string getThunk(std::string name);

    struct objectInfo getInfo(std::string name);

};

#endif

/*
updateGraph(update) - you just give me a new update when you receive one
isStoredLocally(hash) - check if we have the required update. if we don't you'll have to request it,
getThunk(name) - I return you a thunk for given data
*/
