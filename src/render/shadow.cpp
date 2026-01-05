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

void initShadowMapping()
{
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
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    // ADD THESE TWO LINES:
    float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

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

glm::mat4 getLightSpaceMatrix(glm::vec3 lightPos, glm::vec3 target)
{
    // The Cornell box is roughly 550x550.
    // We shrink the ortho box to focus purely on the box area.
    float orthoSize = 200.0f;
    float near_plane = 1.0f, far_plane = 1500.0f; // Lowered far_plane for precision

    glm::mat4 lightProjection = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, near_plane, far_plane);

    // Use the actual light position from your Cornell box
    glm::mat4 lightView = glm::lookAt(lightPos, target, glm::vec3(0.0, 1.0, 0.0));

    return lightProjection * lightView;
}