#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include "../other/enums.h"
#include "chunkrawdata.h"
struct Mesh {
    GLuint vao, vbo, cbo, nbo, tbo, ebo;
    int indexCount;
    bool isReady = false;
    
    std::vector<std::pair<ObjectType, glm::vec3>> chunkObjects;

    void update();
    void initialize(const ChunkRawData &data);
    void render();
    void cleanup();
};
