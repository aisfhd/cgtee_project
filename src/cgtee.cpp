#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "render/shader.h" 

#include <vector>
#include <iostream>
#include <algorithm>
#define _USE_MATH_DEFINES
#include <math.h>

// --- Глобальные настройки ---
static GLFWwindow *window;
static int windowWidth = 1024;
static int windowHeight = 768;

// --- ИГРОК ---
// Позиция камеры - это "ГЛАЗА" игрока. Ноги находятся ниже на playerHeight.
glm::vec3 cameraPos = glm::vec3(0.0f, 50.0f, 500.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

float verticalVelocity = 0.0f;
const float gravity = -1200.0f;   // Чуть усилил гравитацию для резкости
const float jumpForce = 500.0f;
const float playerHeight = 20.0f; // Расстояние от глаз до ног
const float playerWidth = 8.0f;   // "Толщина" игрока
bool isGrounded = false;

// Мышь
bool firstMouse = true;
float yaw = -90.0f;
float pitch = 0.0f;
float lastX = windowWidth / 2.0;
float lastY = windowHeight / 2.0;
float fov = 90.0f;

// Время
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// --- КОЛЛИЗИЯ (AABB) ---
struct AABB {
    glm::vec3 min;
    glm::vec3 max;
};

// Проверка пересечения двух AABB
bool CheckCollision(const AABB& one, const AABB& two) {
    return (one.min.x <= two.max.x && one.max.x >= two.min.x) &&
           (one.min.y <= two.max.y && one.max.y >= two.min.y) &&
           (one.min.z <= two.max.z && one.max.z >= two.min.z);
}

// --- MESH ---
struct Mesh
{
    glm::vec3 position;
    glm::vec3 scale;
    glm::vec3 rotation;

    std::vector<glm::vec3> vertices_pos;
    std::vector<glm::vec3> vertices_colors;
    std::vector<glm::ivec3> indeces;

    AABB localBounds; 
    AABB worldBounds; // Это границы, с которыми мы будем сталкиваться

    GLuint vertexArrayID, vertexBufferID, colorBufferID, indexBufferID;
    GLuint programID, mvpMatrixID;

    void initialize(std::vector<glm::vec3> v_pos, std::vector<glm::vec3> v_col, std::vector<glm::ivec3> idx)
    {
        this->vertices_pos = v_pos;
        this->vertices_colors = v_col;
        this->indeces = idx;
        
        this->position = glm::vec3(0);
        this->scale = glm::vec3(1);
        this->rotation = glm::vec3(0);

        setupBuffers();
        calculateLocalBounds();
        updateWorldBounds();
    }

    void setTransform(glm::vec3 pos, glm::vec3 scl, glm::vec3 rot) {
        this->position = pos;
        this->scale = scl;
        this->rotation = rot;
        updateWorldBounds();
    }

    void setupBuffers() {
        glGenVertexArrays(1, &vertexArrayID);
        glBindVertexArray(vertexArrayID);

        glGenBuffers(1, &vertexBufferID);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
        glBufferData(GL_ARRAY_BUFFER, vertices_pos.size() * sizeof(glm::vec3), &vertices_pos[0], GL_STATIC_DRAW);

        glGenBuffers(1, &colorBufferID);
        glBindBuffer(GL_ARRAY_BUFFER, colorBufferID);
        glBufferData(GL_ARRAY_BUFFER, vertices_colors.size() * sizeof(glm::vec3), &vertices_colors[0], GL_STATIC_DRAW);

        glGenBuffers(1, &indexBufferID);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indeces.size() * sizeof(glm::ivec3), &indeces[0], GL_STATIC_DRAW);

        programID = LoadShadersFromFile("src/shaders/cube.vert", "src/shaders/cube.frag");
        mvpMatrixID = glGetUniformLocation(programID, "MVP");
    }

    void calculateLocalBounds() {
        if (vertices_pos.empty()) return;
        localBounds.min = vertices_pos[0];
        localBounds.max = vertices_pos[0];
        for (const auto& v : vertices_pos) {
            localBounds.min.x = std::min(localBounds.min.x, v.x);
            localBounds.min.y = std::min(localBounds.min.y, v.y);
            localBounds.min.z = std::min(localBounds.min.z, v.z);
            localBounds.max.x = std::max(localBounds.max.x, v.x);
            localBounds.max.y = std::max(localBounds.max.y, v.y);
            localBounds.max.z = std::max(localBounds.max.z, v.z);
        }
    }

    // Обновляем границы в мировых координатах
    void updateWorldBounds() {
        // Упрощенный расчет AABB для повернутых объектов (описываем прямоугольником)
        // Для точной физики наклонных поверхностей нужен Raycasting, но для блоков это сработает.
        glm::vec3 minPos = position + (localBounds.min * scale);
        glm::vec3 maxPos = position + (localBounds.max * scale);
        
        // Пересчитываем min/max на случай отрицательного скейла
        worldBounds.min.x = std::min(minPos.x, maxPos.x);
        worldBounds.min.y = std::min(minPos.y, maxPos.y);
        worldBounds.min.z = std::min(minPos.z, maxPos.z);
        
        worldBounds.max.x = std::max(minPos.x, maxPos.x);
        worldBounds.max.y = std::max(minPos.y, maxPos.y);
        worldBounds.max.z = std::max(minPos.z, maxPos.z);
    }

    void render(glm::mat4 vpMatrix)
    {
        glUseProgram(programID);
        
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, colorBufferID);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, position);
        // Вращение пока отключим для AABB физики (или используй 0,90,180,270 градусов)
        // Если объект реально наклонен, AABB станет большим "пузырем" вокруг него
        model = glm::rotate(model, rotation.x, glm::vec3(1, 0, 0));
        model = glm::rotate(model, rotation.y, glm::vec3(0, 1, 0));
        model = glm::rotate(model, rotation.z, glm::vec3(0, 0, 1));
        model = glm::scale(model, scale);

        glm::mat4 mvp = vpMatrix * model;
        glUniformMatrix4fv(mvpMatrixID, 1, GL_FALSE, &mvp[0][0]);

        glDrawElements(GL_TRIANGLES, indeces.size() * 3, GL_UNSIGNED_INT, (void *)0);

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
    }

    void cleanup() {
        glDeleteBuffers(1, &vertexBufferID);
        glDeleteBuffers(1, &colorBufferID);
        glDeleteBuffers(1, &indexBufferID);
        glDeleteVertexArrays(1, &vertexArrayID);
        glDeleteProgram(programID);
    }
};

std::vector<Mesh> worldObjects;

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void processInput(GLFWwindow *window);

// Хелпер создания куба
void createCubeData(std::vector<glm::vec3>& pos, std::vector<glm::vec3>& col, std::vector<glm::ivec3>& idx) {
    pos = { {-1, -1, 1}, {1, -1, 1}, {1, 1, 1}, {-1, 1, 1}, {1, -1, -1}, {-1, -1, -1}, {-1, 1, -1}, {1, 1, -1} };
    col = { {1,0,0}, {1,0,0}, {1,0,0}, {1,0,0}, {0,1,0}, {0,1,0}, {0,1,0}, {0,1,0} };
    idx = { {0, 1, 2}, {2, 3, 0}, {1, 4, 7}, {7, 2, 1}, {4, 5, 6}, {6, 7, 4}, {5, 0, 3}, {3, 6, 5}, {3, 2, 7}, {7, 6, 3}, {5, 4, 1}, {1, 0, 5} };
}

// Получить AABB игрока в точке (координаты ног)
AABB getPlayerBounds(glm::vec3 feetPos) {
    AABB pBox;
    // Ноги - это feetPos. Голова - это feetPos + playerHeight.
    // Делаем коробку чуть меньше высоты игрока, чтобы не цепляться головой за низкие потолки при ходьбе
    pBox.min = glm::vec3(feetPos.x - playerWidth, feetPos.y, feetPos.z - playerWidth);
    pBox.max = glm::vec3(feetPos.x + playerWidth, feetPos.y + playerHeight, feetPos.z + playerWidth);
    return pBox;
}

int main(void)
{
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    window = glfwCreateWindow(windowWidth, windowHeight, "Physics Gravity Test", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;
    glEnable(GL_DEPTH_TEST);

    // --- ГЕНЕРАЦИЯ МИРА ---
    std::vector<glm::vec3> cPos, cCol;
    std::vector<glm::ivec3> cIdx;
    createCubeData(cPos, cCol, cIdx);

    // 1. ЗЕМЛЯ (Теперь это просто огромный плоский куб)
    Mesh ground;
    ground.initialize(cPos, cCol, cIdx);
    // Растягиваем куб (высота 10, чтобы было "толстое" основание)
    // Позиция Y = -10, значит верхняя грань будет на Y=0 (так как куб от -1 до 1, скейл 10 -> высота 20, центр -10)
    // Лучше сдвинем так: Центр Y=-5, Скейл Y=5. Верх = 0.
    ground.setTransform(glm::vec3(0, -5, 0), glm::vec3(1000, 5, 1000), glm::vec3(0));
    worldObjects.push_back(ground);

    // 2. Бревно / Камень (Препятствие)
    Mesh stone;
    stone.initialize(cPos, cCol, cIdx);
    stone.setTransform(glm::vec3(0, 10, -100), glm::vec3(20, 10, 20), glm::vec3(0));
    worldObjects.push_back(stone);

    // 3. Лестница
    for(int i=0; i<6; i++) {
        Mesh step;
        step.initialize(cPos, cCol, cIdx);
        // Ступеньки поднимаются
        step.setTransform(glm::vec3(50, 5 + i * 10, -50 - i * 20), glm::vec3(15, 5, 15), glm::vec3(0));
        worldObjects.push_back(step);
    }

    // --- Цикл ---
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        glClearColor(0.5f, 0.7f, 1.0f, 1.0f); // Небо
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 projection = glm::perspective(glm::radians(fov), (float)windowWidth / (float)windowHeight, 0.1f, 3000.0f);
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 vp = projection * view;

        for (auto &obj : worldObjects) {
            obj.render(vp);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    for(auto &obj : worldObjects) obj.cleanup();
    glfwTerminate();
    return 0;
}

// --- УЛУЧШЕННАЯ ФИЗИКА ---
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // 1. Определяем вектор движения (WASD)
    float speed = 300.0f * deltaTime;
    glm::vec3 frontXZ = glm::normalize(glm::vec3(cameraFront.x, 0.0f, cameraFront.z));
    glm::vec3 rightXZ = glm::normalize(glm::cross(cameraFront, cameraUp));
    
    glm::vec3 movement = glm::vec3(0.0f);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) movement += frontXZ * speed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) movement -= frontXZ * speed;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) movement -= rightXZ * speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) movement += rightXZ * speed;

    // Считаем позицию НОГ (для коллизии удобнее работать от ног)
    glm::vec3 feetPos = cameraPos;
    feetPos.y -= playerHeight;

    // --- ШАГ 1: ГРАВИТАЦИЯ И ВЕРТИКАЛЬ ---
    
    // Применяем гравитацию к скорости
    verticalVelocity += gravity * deltaTime;
    
    // Предсказываем позицию ног по Y
    float nextY = feetPos.y + verticalVelocity * deltaTime;
    glm::vec3 nextFeetPos = feetPos;
    nextFeetPos.y = nextY;

    bool collidedY = false;
    AABB playerBox = getPlayerBounds(nextFeetPos);

    for (const auto& obj : worldObjects) {
        if (CheckCollision(playerBox, obj.worldBounds)) {
            // Столкновение по вертикали!
            collidedY = true;

            // Если падали вниз (скорость < 0) - значит приземлились НА объект
            if (verticalVelocity <= 0) {
                // Ставим ноги ровно на крышу объекта
                feetPos.y = obj.worldBounds.max.y;
                verticalVelocity = 0.0f;
                isGrounded = true;
            }
            // Если летели вверх (прыжок) и ударились головой
            else {
                // Стукаемся об низ (можно добавить логику отскока, но пока просто стоп)
                verticalVelocity = 0.0f;
                // feetPos.y не меняем, просто перестаем лететь вверх
            }
            break; // Обработали столкновение с ближайшим, выходим (можно улучшить сортировкой)
        }
    }

    if (!collidedY) {
        // Если не столкнулись, принимаем новую позицию
        feetPos.y = nextY;
        isGrounded = false; // Мы в воздухе
    }

    // --- ШАГ 2: ДВИЖЕНИЕ ПО ГОРИЗОНТАЛИ (X и Z раздельно для скольжения) ---
    
    // -- Ось X --
    glm::vec3 nextFeetX = feetPos;
    nextFeetX.x += movement.x;
    // Делаем чуть меньше AABB для ходьбы, чтобы "пролезать" в двери
    AABB boxX = getPlayerBounds(nextFeetX); 
    // Важный хак: чуть приподнимаем проверочный бокс снизу, чтобы не спотыкаться об "пол" при ходьбе
    boxX.min.y += 0.1f; 

    bool collisionX = false;
    for (const auto& obj : worldObjects) {
        if (CheckCollision(boxX, obj.worldBounds)) {
            collisionX = true;
            break;
        }
    }
    if (!collisionX) feetPos.x = nextFeetX.x;

    // -- Ось Z --
    glm::vec3 nextFeetZ = feetPos;
    nextFeetZ.z += movement.z;
    AABB boxZ = getPlayerBounds(nextFeetZ);
    boxZ.min.y += 0.1f; // Хак "шаг над полом"

    bool collisionZ = false;
    for (const auto& obj : worldObjects) {
        if (CheckCollision(boxZ, obj.worldBounds)) {
            collisionZ = true;
            break;
        }
    }
    if (!collisionZ) feetPos.z = nextFeetZ.z;

    // --- ШАГ 3: ПРЫЖОК ---
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && isGrounded)
    {
        verticalVelocity = jumpForce;
        isGrounded = false;
        // Небольшой хак: чуть поднимем игрока, чтобы он сразу оторвался от земли и AABB не пересекался
        feetPos.y += 0.1f; 
    }

    // Обновляем камеру (привязываем глаза к ногам)
    cameraPos = feetPos;
    cameraPos.y += playerHeight;
}

// --- ДОБАВИТЬ ЭТО В КОНЕЦ ФАЙЛА cgtee.cpp ---

void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // Обратный порядок вычитания, так как Y растет снизу вверх
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
    windowWidth = width;
    windowHeight = height;
}