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
//ModelGLTF giantTreeModel;
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

    glm::vec3 lightDir = glm::normalize(glm::vec3(0.5f, 1.0f, 0.3f));
    glm::vec3 lightColor(1.0f, 0.95f, 0.9f);
    glm::vec3 skyColor(0.5f, 0.7f, 0.9f);

    GLuint mvpLoc = glGetUniformLocation(globalProgramID, "MVP");
    GLuint modelLoc = glGetUniformLocation(globalProgramID, "Model");
    GLuint viewPosLoc = glGetUniformLocation(globalProgramID, "viewPos");
    GLuint lightDirLoc = glGetUniformLocation(globalProgramID, "lightDir");
    GLuint lightColorLoc = glGetUniformLocation(globalProgramID, "lightColor");
    GLuint pointLightPosLoc = glGetUniformLocation(globalProgramID, "pointLightPos");
    GLuint pointLightColorLoc = glGetUniformLocation(globalProgramID, "pointLightColor");
    GLuint skyColorLoc = glGetUniformLocation(globalProgramID, "skyColor");
    GLuint texLoc = glGetUniformLocation(globalProgramID, "texture1");

    // Цвет для моделей (чтобы не использовать текстуру блоков)
    GLuint colorAttribLoc = 1;

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);
        updateChunksManager();

        glClearColor(skyColor.x, skyColor.y, skyColor.z, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(globalProgramID);

        glm::mat4 projection = glm::perspective(glm::radians(fov), (float)windowWidth / (float)windowHeight, 0.1f, 3000.0f);
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 vp = projection * view;

        glUniform3fv(lightDirLoc, 1, &lightDir[0]);
        glUniform3fv(lightColorLoc, 1, &lightColor[0]);
        glUniform3fv(skyColorLoc, 1, &skyColor[0]);
        glUniform3fv(viewPosLoc, 1, &cameraPos[0]);

        // Point Light
        float time = (float)glfwGetTime();
        glm::vec3 pLightPos = glm::vec3(sin(time) * 50.0f, 50.0f, cos(time) * 50.0f);
        glm::vec3 pLightColor = glm::vec3(1.0f, 0.5f, 0.0f);
        glUniform3fv(pointLightPosLoc, 1, &pLightPos[0]);
        glUniform3fv(pointLightColorLoc, 1, &pLightColor[0]);

        // 1. Рисуем МИР (Чанки)
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, blockTexture);
        glUniform1i(texLoc, 0);

        for (auto &kv : activeChunks)
        {
            if (!kv.second.isReady)
                continue;
            glm::mat4 model = glm::mat4(1.0f);
            glm::mat4 mvp = vp * model;
            glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, &mvp[0][0]);
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);
            kv.second.render();
        }

        // 2. Рисуем ОБЪЕКТЫ (Деревья, Грибы)
        // Они используют тот же шейдер, но мы можем отключить текстуру или использовать другую
        // Для простоты используем ту же текстуру или белый цвет (если у моделей нет UV)

        for (auto &kv : activeChunks)
        {
            if (!kv.second.isReady)
                continue;

            for (auto &obj : kv.second.chunkObjects)
            {
                glm::mat4 objModel = glm::translate(glm::mat4(1.0f), obj.second);
                // Настраиваем масштаб моделей (gltf часто бывают огромными или маленькими)
                float scale = 1.0f;

                if (obj.first == TREE_SMALL)
                {
                    scale = 4.0f; // Подбери значение
                    objModel = glm::scale(objModel, glm::vec3(scale));
                    // objModel = objModel * treeModel.modelTransform;  // Removed to avoid double application
                    // Считаем MVP и рисуем
                    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, &(vp * objModel)[0][0]);
                    treeModel.draw(objModel, modelLoc, colorAttribLoc);
                }
                else if (obj.first == MUSHROOM)
                {
                    scale = 3.0f; // Маленький гриб
                    objModel = glm::scale(objModel, glm::vec3(scale));
                    // objModel = objModel * mushroomModel.modelTransform;  // Removed to avoid double application
                    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, &(vp * objModel)[0][0]);
                    mushroomModel.draw(objModel, modelLoc, colorAttribLoc);
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
