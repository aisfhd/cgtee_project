#pragma once
#include <vector>
#include <glm/glm.hpp>
#include "../other/enums.h"
struct ChunkRawData {
    int cx, cz;
    std::vector<glm::vec3> positions;
    std::vector<glm::vec4> colors;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> uvs;
    std::vector<unsigned int> indices;
    
    // Сюда сохраняем позиции объектов
    std::vector<std::pair<ObjectType, glm::vec3>> objects;
};

