#pragma once
#include "../other/globals.h"
#include <iostream>
#define APIENTRY __stdcall
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_STB_IMAGE
#define MAX_BONE_INFLUENCE 4
#include <tinygltf-2.9.7/tiny_gltf.h>
struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texcoord;

    int m_BoneIDs[MAX_BONE_INFLUENCE];
    float m_Weights[MAX_BONE_INFLUENCE];

};
struct BoneInfo {
    int id;
    glm::mat4 offset;
};
struct SubMesh {
    GLuint vao, vbo, ebo;
    int indexCount;
    GLenum indexType;
    GLuint textureID;
    glm::mat4 subTransform = glm::mat4(1.0f);
};
struct ModelGLTF
{
    bool loaded = false;
    glm::mat4 modelTransform = glm::mat4(1.0f);
    std::vector<SubMesh> subMeshes;

    std::map<std::string, BoneInfo> m_BoneInfoMap;
    int m_BoneCounter = 0;
    std::vector<glm::mat4> finalBoneTransforms;
    bool isAnimated = false;

    void load(const std::string &modelName);

    void draw(glm::mat4 vp, glm::mat4 modelMatrix, GLuint mvpLoc, GLuint modelLoc, GLuint colorLoc);

    void SetVertexBoneDataToDefault(Vertex& vertex);
    void SetVertexBoneData(Vertex& vertex, int boneID, float weight);
    void updateAnimation(float time);
};
