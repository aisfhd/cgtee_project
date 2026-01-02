#include "other/globals.h"
#include <glad/glad.h>

#include "other/glglobals.h"
#include "other/pathglobals.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "render/shader.h"
#include "render/texture.h"
#include "render/model.h"
#include "render/shadow.h"

#include "world/chunkrawdata.h"
#include "world/mesh.h"
#include "world/worldgen.h"
#include "world/chunk.h"

#include "other/controls.h"

#include <vector>
#include <iostream>
#include <thread>
#include <filesystem>

#define _USE_MATH_DEFINES
#include <math.h>

float verticalVelocity = 0.0f;
const float gravity = -1000.0f;
const float jumpForce = 350.0f;
const float playerHeight = 15.0f;
bool isGrounded = false;

bool firstMouse = true;
float yaw = -90.0f;
float pitch = 0.0f;
float lastX = windowWidth / 2.0;
float lastY = windowHeight / 2.0;
float fov = 90.0f;
float deltaTime = 0.0f;
float lastFrame = 0.0f;

ModelGLTF treeModel;
// ModelGLTF giantTreeModel;
ModelGLTF tree2Model;
ModelGLTF mushroomModel;

// --- Генерация Куба ---

int main(void)
{
    system("echo %CD%");
    if (!glfwInit())
        return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(windowWidth, windowHeight, "Voxel World - Objects", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        return -1;

    // --- ОБЯЗАТЕЛЬНО: Инициализация системы теней ---
    initShadowMapping(); // Без этого вызова теней не будет вообще!

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    globalProgramID = LoadShadersFromFile("lighting.vert", "lighting.frag");

    treeModel.load("tree");
    tree2Model.load("tree2");
    mushroomModel.load("mushroom");

    GLuint blockTexture = loadTexture("../assets/textures/block.png");

    std::thread generator(chunkWorkerThread);
    generator.detach();

    // Параметры света
    glm::vec3 lightDir = glm::normalize(glm::vec3(1.0f, 0.5f, 0.3f));
    glm::vec3 lightColor(1.0f, 0.95f, 0.9f);
    glm::vec3 skyColor(0.2f, 0.3f, 0.5f); // Darker sky color

    // Получаем локации униформ один раз (для оптимизации)
    GLuint mvpLoc = glGetUniformLocation(globalProgramID, "MVP");
    GLuint modelLoc = glGetUniformLocation(globalProgramID, "Model");
    GLuint viewPosLoc = glGetUniformLocation(globalProgramID, "viewPos");
    GLuint lightDirLoc = glGetUniformLocation(globalProgramID, "lightDir");
    GLuint lightColorLoc = glGetUniformLocation(globalProgramID, "lightColor");
    GLuint pointLightPosLoc = glGetUniformLocation(globalProgramID, "pointLightPos");
    GLuint pointLightColorLoc = glGetUniformLocation(globalProgramID, "pointLightColor");
    GLuint skyColorLoc = glGetUniformLocation(globalProgramID, "skyColor");
    GLuint texLoc = glGetUniformLocation(globalProgramID, "texture1");
    GLuint shadowMapLoc = glGetUniformLocation(globalProgramID, "shadowMap");
    GLuint lightSpaceMatrixLoc = glGetUniformLocation(globalProgramID, "lightSpaceMatrix");

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);
        updateChunksManager();

        // 1. Сначала рассчитываем матрицы для текущего кадра
        glm::mat4 projection = glm::perspective(glm::radians(fov), (float)windowWidth / (float)windowHeight, 0.1f, 3000.0f);
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 vp = projection * view;

        // Позиция "солнца" следует за игроком для покрытия тенями области вокруг него
        glm::vec3 lightPos = cameraPos + lightDir * 1000.0f;
        lightSpaceMatrix = getLightSpaceMatrix(lightPos, cameraPos);

        // --- ПАСС 1: РЕНДЕР В КАРТУ ТЕНЕЙ ---
        glViewport(0, 0, shadowRes, shadowRes);
        glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
        glClear(GL_DEPTH_BUFFER_BIT);

        glUseProgram(depthProgramID);
        glEnable(GL_DEPTH_TEST);
        glCullFace(GL_BACK); // Changed back to BACK

        // Рисуем чанки в карту теней
        glm::mat4 modelIdentity = glm::mat4(1.0f);
        for (auto &kv : activeChunks)
        {
            if (!kv.second.isReady)
                continue;
            glm::mat4 depthMVP = lightSpaceMatrix * modelIdentity;
            glUniformMatrix4fv(depthMVPloc, 1, GL_FALSE, &depthMVP[0][0]);
            kv.second.render();
        }

        // Рисуем объекты в карту теней
        for (auto &kv : activeChunks)
        {
            if (!kv.second.isReady)
                continue;
            for (auto &obj : kv.second.chunkObjects)
            {
                glm::mat4 objModel = glm::translate(glm::mat4(1.0f), obj.second);
                float scale = (obj.first == TREE_SMALL) ? 4.0f : 3.0f;
                objModel = glm::scale(objModel, glm::vec3(scale));

                glm::mat4 depthMVP = lightSpaceMatrix * objModel;
                // glUniformMatrix4fv(depthMVPloc, 1, GL_FALSE, &depthMVP[0][0]); // Now set inside draw

                if (obj.first == TREE_SMALL)
                    treeModel.draw(lightSpaceMatrix, objModel, depthMVPloc, 0, 0);
                else if (obj.first == MUSHROOM)
                    mushroomModel.draw(lightSpaceMatrix, objModel, depthMVPloc, 0, 0);
            }
        }

        glCullFace(GL_BACK);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // --- ПАСС 2: ОСНОВНОЙ РЕНДЕР НА ЭКРАН ---
        glViewport(0, 0, windowWidth, windowHeight);
        glClearColor(skyColor.x, skyColor.y, skyColor.z, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(globalProgramID);

        // Передаем униформы света
        glUniformMatrix4fv(lightSpaceMatrixLoc, 1, GL_FALSE, &lightSpaceMatrix[0][0]);
        glUniform3fv(lightDirLoc, 1, &lightDir[0]);
        glUniform3fv(lightColorLoc, 1, &lightColor[0]);
        glUniform3fv(skyColorLoc, 1, &skyColor[0]);
        glUniform3fv(viewPosLoc, 1, &cameraPos[0]);

        // Привязываем текстуру теней к Unit 1
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, shadowDepthTex);
        glUniform1i(shadowMapLoc, 1);

        // Привязываем текстуру блоков к Unit 0
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, blockTexture);
        glUniform1i(texLoc, 0);

        // Точечный свет (летающий шар)
        float time = (float)glfwGetTime();
        glm::vec3 pLightPos = glm::vec3(sin(time) * 50.0f, 50.0f, cos(time) * 50.0f);
        glUniform3fv(pointLightPosLoc, 1, &pLightPos[0]);
        glUniform3fv(pointLightColorLoc, 1, &glm::vec3(0.2f, 0.1f, 0.0f)[0]);

        // Рисуем чанки
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &modelIdentity[0][0]);
        for (auto &kv : activeChunks)
        {
            if (!kv.second.isReady)
                continue;
            glm::mat4 mvp = vp * modelIdentity;
            glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, &mvp[0][0]);
            kv.second.render();
        }

        // Рисуем объекты (Деревья, Грибы)
        for (auto &kv : activeChunks)
        {
            if (!kv.second.isReady)
                continue;
            for (auto &obj : kv.second.chunkObjects)
            {
                glm::mat4 objModel = glm::translate(glm::mat4(1.0f), obj.second);
                if (obj.first == TREE_SMALL)
                {
                    objModel = glm::scale(objModel, glm::vec3(4.0f));
                    // glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, &(vp * objModel)[0][0]); // Now set inside draw
                    treeModel.draw(vp, objModel, mvpLoc, modelLoc, 1);
                }
                else if (obj.first == MUSHROOM)
                {
                    objModel = glm::scale(objModel, glm::vec3(3.0f));
                    // glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, &(vp * objModel)[0][0]); // Now set inside draw
                    mushroomModel.draw(vp, objModel, mvpLoc, modelLoc, 1);
                }
            }
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    threadRunning = false;
    glfwTerminate();
    return 0;
}