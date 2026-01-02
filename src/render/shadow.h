#pragma once
#include "../other/glglobals.h"
#include <glm/glm.hpp>

void initShadowMapping();

glm::mat4 getLightSpaceMatrix(glm::vec3 lightPos, glm::vec3 target);