#pragma once
#include "../other/globals.h"
#include <iostream>
#define APIENTRY __stdcall
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_STB_IMAGE
#include <tinygltf-2.9.7/tiny_gltf.h>
struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texcoord;
};
struct SubMesh {
    GLuint vao, vbo, ebo;
    int indexCount;
    GLuint textureID;
    glm::mat4 subTransform = glm::mat4(1.0f);
};
struct ModelGLTF
{
    bool loaded = false;
    glm::mat4 modelTransform = glm::mat4(1.0f);
    std::vector<SubMesh> subMeshes;

    void load(const std::string &modelName);

    void draw(glm::mat4 modelMatrix, GLuint modelLoc, GLuint colorLoc);
};