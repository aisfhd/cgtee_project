#include "shadow.h"
#include "../other/glglobals.h"
#include <iostream>
#include "shader.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <limits>

GLuint shadowFBO = 0;
GLuint shadowDepthTex = 0;
const int shadowRes = 2048;

GLuint depthProgramID;
GLuint depthMVPloc;

glm::mat4 lightSpaceMatrix;

void initShadowMapping() {
    // 1. Create Framebuffer Object
    glGenFramebuffers(1, &shadowFBO);

    // 2. Create Depth Texture
    glGenTextures(1, &shadowDepthTex);
    glBindTexture(GL_TEXTURE_2D, shadowDepthTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 
                 shadowRes, shadowRes, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    
    // Set texture parameters for shadow mapping 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // 3. Attach texture to FBO
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowDepthTex, 0);
    
    // Explicitly tell OpenGL we aren't rendering any color data 
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "Shadow Framebuffer is not complete!" << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // 4. Load Depth Shaders (provided in your files) [cite: 4, 5, 6]
    depthProgramID = LoadShadersFromFile("depth.vert", "depth.frag");
    depthMVPloc = glGetUniformLocation(depthProgramID, "depthMVP");
}

glm::mat4 getLightSpaceMatrix(glm::vec3 lightPos, glm::vec3 target) {
    float near_plane = 1.0f, far_plane = 5000.0f;
    // For Directional Light (Sun), use Orthographic projection
    glm::mat4 lightProjection = glm::ortho(-1000.0f, 1000.0f, -1000.0f, 1000.0f, near_plane, far_plane);
    glm::mat4 lightView = glm::lookAt(lightPos, target, glm::vec3(0.0, 1.0, 0.0));
    return lightProjection * lightView;
}