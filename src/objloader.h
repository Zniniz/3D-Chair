#pragma once
#include <string>
#include <vector>
#include <glm/glm.hpp>

bool loadOBJ(const std::string& path,
             std::vector<glm::vec3>& outPositions,
             std::vector<unsigned int>& outIndices);