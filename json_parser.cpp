#include <iostream>
#include <string>
#include "boost/property_tree/ptree.hpp"
#include "boost/property_tree/json_parser.hpp"

template <typename T>
std::vector<T> as_vector(boost::property_tree::ptree const& pt, boost::property_tree::ptree::key_type const& key)
{
    std::vector<T> r;
    for (auto& item : pt.get_child(key))
        r.push_back(item.second.get_value<T>());
    return r;
}

int main() {
    using boost::property_tree::ptree;

    std::ifstream jsonFile("test.json");
    //std::ifstream jsonFile("input.txt");

    ptree pt;
    read_json(jsonFile, pt);

    for (auto & array_element: pt) {
      std::cout << "element" << "\n";
        bool flag = false;
        for (auto & property: array_element.second) {
            if(flag){
              std::cout << "hash: " << property.second.get_value < std::string > () << "\n";
            }else{
              std::cout << "Name:" << property.second.get<std::string>("name") << "\n";
              std::cout << "Start:" << property.second.get<std::string>("start") << "\n";
              std::cout << "Duration:" << property.second.get<std::string>("duration") << "\n";
              std::cout << "Inputs:";
              for (auto & input: property.second.get_child("inputs")){
                std::cout << " " << input.second.get_value < std::string > ();
              }
              std::cout << "\n";
              std::cout << "Outputs:";
              for (auto & output: property.second.get_child("outputs")){
                std::cout << " " << output.second.get_value < std::string > ();
              }
              std::cout << "\n";
              std::cout << "Depends:";
              for (auto & depend: property.second.get_child("depends")){
                std::cout << " " << depend.second.get_value < std::string > ();
              }
              std::cout << "\n";
            }
            flag = !flag;
        }
    }
}
