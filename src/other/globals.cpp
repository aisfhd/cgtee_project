#include "globals.h"
GLFWwindow *window;
int windowWidth = 1024;
int windowHeight = 768;

const int CHUNK_SIZE = 16;
const float BLOCK_SCALE = 5.0f;
const int RENDER_DISTANCE = 8; 
const float NOISE_SCALE = 0.04f;

GLuint globalProgramID = 0;

glm::vec3 cameraPos = glm::vec3(0.0f, 60.0f, 0.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);