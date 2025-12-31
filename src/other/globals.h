#pragma once
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
extern GLFWwindow *window;
extern int windowWidth;
extern int windowHeight;
extern const int CHUNK_SIZE;
extern const float BLOCK_SCALE;
extern const int RENDER_DISTANCE; 
extern const float NOISE_SCALE;
extern GLuint globalProgramID;


extern glm::vec3 cameraPos;
extern glm::vec3 cameraFront;
extern glm::vec3 cameraUp;

enum ObjectType {
    NONE = 0,
    TREE_SMALL,
    TREE_GIANT,
    MUSHROOM
};