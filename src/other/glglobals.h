#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
extern GLuint mvpLoc;
extern GLuint modelLoc;
extern GLuint viewPosLoc;
extern GLuint lightDirLoc;
extern GLuint lightColorLoc;
extern GLuint pointLightPosLoc;
extern GLuint pointLightColorLoc;
extern GLuint skyColorLoc;
extern GLuint texLoc;

extern GLuint shadowFBO;
extern GLuint shadowDepthTex;
extern const int shadowRes; // Resolution of the shadow map
extern GLuint depthProgramID;
extern GLuint depthMVPloc;
extern glm::mat4 lightSpaceMatrix; 