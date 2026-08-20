#include "objloader.h"
#include <fstream> 
#include <sstream> 
#include <iostream>

bool loadOBJ(const std::string& path, std::vector<glm::vec3>& outPositions, std::vector<unsigned int>& outIndices){
    std::ifstream file(path);
    if(!file.is_open()){
        std::cerr << "ERROR: cannot open OBJ file: " << path << "\n";
        return false;
    }
    std::cout << "opened OK: " << path << "\n";
    
    std::string line;
    while(std::getline(file, line)){
        std::istringstream ss(line);
        std::string prefix;
        ss >> prefix;
        if (prefix == "v"){  
            float x, y, z;
            ss >> x >> y >> z;
            outPositions.push_back(glm::vec3(x, y, z));
        }else if(prefix == "f"){
            std::vector<unsigned int> face;
            std::string token;
            while (ss >> token) {
                size_t slash = token.find('/');
                std::string idxStr = (slash == std::string::npos) ? token : token.substr(0, slash);
                face.push_back(std::stoul(idxStr) - 1);
            }
            for (size_t i = 1; i + 1 < face.size(); ++i) {
                outIndices.push_back(face[0]);
                outIndices.push_back(face[i]);
                outIndices.push_back(face[i + 1]);
            }
        }
    }
    for (unsigned int i : outIndices) { 
        if (i >= outPositions.size()) {
            std::cerr << "ERROR: index " << i << " out of range\n";
            return false;
        }
    }
    return true;
}


