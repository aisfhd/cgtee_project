#pragma once
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <map>
#include <vector>
#include <atomic>
#include <mutex>
#include <queue>
#include <set>
#include "../world/mesh.h"
#include "../world/chunkrawdata.h"
#include "../world/chunkcoord.h"
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

extern float verticalVelocity;
extern const float gravity;
extern const float jumpForce;
extern const float playerHeight;

extern GLuint sunVAO, sunVBO, sunEBO;
extern int sunIndexCount;
extern bool isGrounded;

extern bool firstMouse;
extern float yaw;
extern float pitch;
extern float lastX;
extern float lastY;
extern float fov;
extern float deltaTime;
extern float lastFrame;

extern std::map<ChunkCoord, Mesh> activeChunks;
extern std::queue<ChunkCoord> loadQueue;
extern std::mutex loadQueueMutex;
extern std::vector<ChunkRawData> uploadQueue;
extern std::mutex uploadQueueMutex;
extern std::atomic<bool> threadRunning;
extern std::set<ChunkCoord> chunksBeingProcessed;