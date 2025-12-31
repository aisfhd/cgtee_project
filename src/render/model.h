#pragma once
#include "../other/globals.h"
#include <iostream>
#define APIENTRY __stdcall
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_STB_IMAGE
#include <tinygltf-2.9.7/tiny_gltf.h>
struct ModelGLTF
{
    GLuint vao, vbo, ebo;
    int indexCount;
    bool loaded = false;

    void load(const std::string &filename);

    void draw(glm::mat4 modelMatrix, GLuint modelLoc, GLuint colorLoc);
};