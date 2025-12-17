#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "render/shader.h"

#include <vector>
#include <iostream>
#include <algorithm>
#include <map>
#include <thread>
#include <mutex>
#include <queue>
#include <atomic>
#include <set>

#define _USE_MATH_DEFINES
#include <math.h>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tinygltf-2.9.7/stb_image.h"

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_STB_IMAGE 
#include "tinygltf-2.9.7/tiny_gltf.h" 

static GLFWwindow *window;
static int windowWidth = 1024;
static int windowHeight = 768;

const int CHUNK_SIZE = 16;
const float BLOCK_SCALE = 5.0f;
const int RENDER_DISTANCE = 8; 
const float NOISE_SCALE = 0.04f;
// Вода убрана по просьбе

// Глобальный шейдер
GLuint globalProgramID = 0;

glm::vec3 cameraPos = glm::vec3(0.0f, 60.0f, 0.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

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

// --- Шум ---
float noise(float x, float y) {
    int n = int(x * 40.0 + y * 6400.0);
    n = (n << 13) ^ n;
    return 1.0 - float((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0;
}
float interpolate(float a, float b, float x) {
    float ft = x * 3.1415927f;
    float f = (1 - cos(ft)) * 0.5f;
    return a * (1 - f) + b * f;
}
float smoothNoise(float x, float y) {
    float corners = (noise(x - 1, y - 1) + noise(x + 1, y - 1) + noise(x - 1, y + 1) + noise(x + 1, y + 1)) / 16.0f;
    float sides = (noise(x - 1, y) + noise(x + 1, y) + noise(x, y - 1) + noise(x, y + 1)) / 8.0f;
    float center = noise(x, y) / 4.0f;
    return corners + sides + center;
}
float perlinNoise(float x, float y) {
    float intX = floor(x);
    float fractionalX = x - intX;
    float intY = floor(y);
    float fractionalY = y - intY;
    return interpolate(interpolate(smoothNoise(intX, intY), smoothNoise(intX + 1, intY), fractionalX),
                       interpolate(smoothNoise(intX, intY + 1), smoothNoise(intX + 1, intY + 1), fractionalX), fractionalY);
}
int getWorldHeight(float x, float z) {
    float val = perlinNoise(x * NOISE_SCALE, z * NOISE_SCALE) * 1.0f + perlinNoise(x * NOISE_SCALE * 2.0f, z * NOISE_SCALE * 2.0f) * 0.5f;
    return (int)(val * 12.0f);
}

// Псевдо-случайное число для генерации деревьев (детерминированное по координатам)
float randomDeterministic(float x, float z) {
    return  glm::fract(sin(glm::dot(glm::vec2(x, z), glm::vec2(12.9898, 78.233))) * 43758.5453);
}

// --- Типы объектов ---
enum ObjectType {
    NONE = 0,
    TREE_SMALL,
    TREE_GIANT,
    MUSHROOM
};

struct ChunkRawData {
    int cx, cz;
    std::vector<glm::vec3> positions;
    std::vector<glm::vec4> colors;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> uvs;
    std::vector<unsigned int> indices;
    
    // Сюда сохраняем позиции объектов
    std::vector<std::pair<ObjectType, glm::vec3>> objects;
};

// --- Простой загрузчик GLTF Моделей ---
struct ModelGLTF {
    GLuint vao, vbo, ebo;
    int indexCount;
    bool loaded = false;

    void load(const std::string& filename) {
        tinygltf::Model model;
        tinygltf::TinyGLTF loader;
        std::string err;
        std::string warn;

        bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, filename); 
        // Если у тебя бинарный glb, используй LoadBinaryFromFile
        
        if (!warn.empty()) std::cout << "Warn: " << warn << std::endl;
        if (!err.empty()) std::cout << "Err: " << err << std::endl;
        if (!ret) {
            std::cout << "Failed to parse glTF: " << filename << std::endl;
            return;
        }

        // Упрощенная загрузка ПЕРВОГО примитива ПЕРВОГО меша
        if (model.meshes.size() > 0 && model.meshes[0].primitives.size() > 0) {
            const tinygltf::Primitive& primitive = model.meshes[0].primitives[0];
            
            // Получаем координаты (POSITION)
            if (primitive.attributes.find("POSITION") != primitive.attributes.end()) {
                const tinygltf::Accessor& accessor = model.accessors[primitive.attributes.at("POSITION")];
                const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
                const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
                
                // TODO: Здесь нужен более сложный код для полноценной загрузки
                // Для простоты примера мы предполагаем, что данные упакованы просто float3
                // В реальном проекте нужно учитывать stride, componentType и т.д.
                
                glGenVertexArrays(1, &vao);
                glBindVertexArray(vao);
                
                glGenBuffers(1, &vbo);
                glBindBuffer(GL_ARRAY_BUFFER, vbo);
                glBufferData(GL_ARRAY_BUFFER, bufferView.byteLength, &buffer.data[bufferView.byteOffset], GL_STATIC_DRAW);
                
                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
                glEnableVertexAttribArray(0);
                
                // Нормали и UV опускаем для краткости, но они нужны для освещения!
                // Если модель черная - значит нет нормалей. 
                
                if (primitive.indices >= 0) {
                    const tinygltf::Accessor& indexAccessor = model.accessors[primitive.indices];
                    const tinygltf::BufferView& indexBufferView = model.bufferViews[indexAccessor.bufferView];
                    const tinygltf::Buffer& indexBuffer = model.buffers[indexBufferView.buffer];
                    
                    glGenBuffers(1, &ebo);
                    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
                    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexBufferView.byteLength, &indexBuffer.data[indexBufferView.byteOffset], GL_STATIC_DRAW);
                    
                    indexCount = indexAccessor.count;
                }
                loaded = true;
                std::cout << "Loaded model: " << filename << " Verts: " << accessor.count << std::endl;
            }
        }
        glBindVertexArray(0);
    }

    void draw(glm::mat4 modelMatrix, GLuint modelLoc, GLuint colorLoc) {
        if (!loaded) return;
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &modelMatrix[0][0]);
        // Красим модель в белый (или текстуру, если есть)
        glVertexAttrib4f(1, 1.0f, 1.0f, 1.0f, 1.0f); 
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_SHORT, 0); // Обычно gltf использует short indices
        glBindVertexArray(0);
    }
};

ModelGLTF treeModel;
ModelGLTF giantTreeModel;
ModelGLTF mushroomModel;

struct Mesh {
    GLuint vao, vbo, cbo, nbo, tbo, ebo;
    int indexCount;
    bool isReady = false;
    
    // Храним объекты для этого чанка
    std::vector<std::pair<ObjectType, glm::vec3>> chunkObjects;

    void cleanup() {
        if (isReady) {
            glDeleteVertexArrays(1, &vao);
            glDeleteBuffers(1, &vbo);
            glDeleteBuffers(1, &cbo);
            glDeleteBuffers(1, &nbo);
            glDeleteBuffers(1, &tbo);
            glDeleteBuffers(1, &ebo);
            isReady = false;
        }
    }

    void initialize(const ChunkRawData &data) {
        chunkObjects = data.objects; // Копируем список объектов
        
        if (data.positions.empty()) return;

        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, data.positions.size() * sizeof(glm::vec3), &data.positions[0], GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
        glEnableVertexAttribArray(0);

        glGenBuffers(1, &cbo);
        glBindBuffer(GL_ARRAY_BUFFER, cbo);
        glBufferData(GL_ARRAY_BUFFER, data.colors.size() * sizeof(glm::vec4), &data.colors[0], GL_STATIC_DRAW);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, (void *)0); 
        glEnableVertexAttribArray(1);

        glGenBuffers(1, &nbo);
        glBindBuffer(GL_ARRAY_BUFFER, nbo);
        glBufferData(GL_ARRAY_BUFFER, data.normals.size() * sizeof(glm::vec3), &data.normals[0], GL_STATIC_DRAW);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
        glEnableVertexAttribArray(2);

        glGenBuffers(1, &tbo);
        glBindBuffer(GL_ARRAY_BUFFER, tbo);
        glBufferData(GL_ARRAY_BUFFER, data.uvs.size() * sizeof(glm::vec2), &data.uvs[0], GL_STATIC_DRAW);
        glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 0, (void *)0);
        glEnableVertexAttribArray(3);

        glGenBuffers(1, &ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, data.indices.size() * sizeof(unsigned int), &data.indices[0], GL_STATIC_DRAW);

        indexCount = data.indices.size();
        isReady = true;
    }

    void render() {
        if (!isReady) return;
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
    }
};

struct ChunkCoord {
    int x, z;
    bool operator<(const ChunkCoord &o) const { return x < o.x || (x == o.x && z < o.z); }
};

std::map<ChunkCoord, Mesh> activeChunks;
std::queue<ChunkCoord> loadQueue;
std::mutex loadQueueMutex;
std::vector<ChunkRawData> uploadQueue;
std::mutex uploadQueueMutex;
std::atomic<bool> threadRunning{true};
std::set<ChunkCoord> chunksBeingProcessed;

// --- Генерация Куба ---
void addCubeToRaw(ChunkRawData &data, float x, float y, float z, float s, glm::vec4 color, int texID) {
    float h = s / 2.0f;
    glm::vec3 v[] = {
        {x - h, y - h, z + h}, {x + h, y - h, z + h}, {x + h, y + h, z + h}, {x - h, y + h, z + h},
        {x + h, y - h, z - h}, {x - h, y - h, z - h}, {x - h, y + h, z - h}, {x + h, y + h, z - h},
    };

    float uMin = (texID % 2) * 0.5f;
    float vMin = (texID / 2) * 0.5f;
    float uMax = uMin + 0.5f;
    float vMax = vMin + 0.5f;
    glm::vec2 uv0(uMin, vMin), uv1(uMax, vMin), uv2(uMax, vMax), uv3(uMin, vMax);

    unsigned int idx_offset = data.positions.size();
    auto addFace = [&](int a, int b, int c, int d, glm::vec3 n) {
        data.positions.push_back(v[a]); data.positions.push_back(v[b]);
        data.positions.push_back(v[c]); data.positions.push_back(v[d]);
        for (int i = 0; i < 4; i++) { data.colors.push_back(color); data.normals.push_back(n); }
        data.uvs.push_back(uv0); data.uvs.push_back(uv1);
        data.uvs.push_back(uv2); data.uvs.push_back(uv3);
        data.indices.push_back(idx_offset); data.indices.push_back(idx_offset + 1);
        data.indices.push_back(idx_offset + 2); data.indices.push_back(idx_offset + 2);
        data.indices.push_back(idx_offset + 3); data.indices.push_back(idx_offset);
        idx_offset += 4;
    };

    addFace(0, 1, 2, 3, glm::vec3(0, 0, 1));  addFace(4, 5, 6, 7, glm::vec3(0, 0, -1));
    addFace(1, 4, 7, 2, glm::vec3(1, 0, 0));  addFace(5, 0, 3, 6, glm::vec3(-1, 0, 0));
    addFace(3, 2, 7, 6, glm::vec3(0, 1, 0));  addFace(5, 4, 1, 0, glm::vec3(0, -1, 0));
}

// --- Генерация с БИОМАМИ и МОДЕЛЯМИ ---
ChunkRawData generateChunkDataCPU(int cx, int cz) {
    ChunkRawData data;
    data.cx = cx; data.cz = cz;
    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int z = 0; z < CHUNK_SIZE; z++) {
            int worldX = cx * CHUNK_SIZE + x;
            int worldZ = cz * CHUNK_SIZE + z;
            int h = getWorldHeight((float)worldX, (float)worldZ);
            float posX = worldX * BLOCK_SCALE;
            float posZ = worldZ * BLOCK_SCALE;
            
            glm::vec4 color(0.2f, 0.7f, 0.2f, 1.0f); // Трава
            int texID = 0;
            if (h > 6) { color = glm::vec4(0.9f, 0.9f, 0.95f, 1.0f); texID = 1; } // Снег
            else if (h <= 0) { color = glm::vec4(0.8f, 0.8f, 0.4f, 1.0f); texID = 3; } // Песок

            addCubeToRaw(data, posX, h * BLOCK_SCALE, posZ, BLOCK_SCALE, color, texID);

            // Земля под блоком
            for (int d = 1; d <= 2; d++) {
                addCubeToRaw(data, posX, (h - d) * BLOCK_SCALE, posZ, BLOCK_SCALE, glm::vec4(0.4f, 0.3f, 0.2f, 1.0f), 3);
            }

            // --- Генерация объектов (Деревья, Грибы) ---
            // Генерируем только на траве (h > 0 и h <= 6)
            if (h > 0 && h <= 6) {
                float r = randomDeterministic((float)worldX, (float)worldZ);
                
                // Чтобы не пересекались, используем else if
                // Настраиваем вероятность (0.98 = редко)
                if (r > 0.99995f) {
                    // Гигантское дерево (Очень редко)
                    data.objects.push_back({TREE_GIANT, glm::vec3(posX, h * BLOCK_SCALE + (BLOCK_SCALE/2.0f), posZ)});
                } 
                else if (r > 0.99985f) {
                    // Обычное дерево
                    data.objects.push_back({TREE_SMALL, glm::vec3(posX, h * BLOCK_SCALE + (BLOCK_SCALE/2.0f), posZ)});
                }
                else if (r > 0.9997f) {
                    // Гриб
                    data.objects.push_back({MUSHROOM, glm::vec3(posX, h * BLOCK_SCALE + (BLOCK_SCALE/2.0f), posZ)});
                }
            }
        }
    }
    return data;
}

void chunkWorkerThread() {
    while (threadRunning) {
        ChunkCoord target = {0, 0};
        bool hasJob = false;
        {
            std::lock_guard<std::mutex> lock(loadQueueMutex);
            if (!loadQueue.empty()) { target = loadQueue.front(); loadQueue.pop(); hasJob = true; }
        }
        if (hasJob) {
            ChunkRawData rawData = generateChunkDataCPU(target.x, target.z);
            { std::lock_guard<std::mutex> lock(uploadQueueMutex); uploadQueue.push_back(rawData); }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }
}

void updateChunksManager() {
    int pChunkX = (int)floor(cameraPos.x / (CHUNK_SIZE * BLOCK_SCALE));
    int pChunkZ = (int)floor(cameraPos.z / (CHUNK_SIZE * BLOCK_SCALE));
    for (auto it = activeChunks.begin(); it != activeChunks.end();) {
        ChunkCoord c = it->first;
        if (abs(c.x - pChunkX) > RENDER_DISTANCE + 1 || abs(c.z - pChunkZ) > RENDER_DISTANCE + 1) {
            it->second.cleanup(); it = activeChunks.erase(it); chunksBeingProcessed.erase(c);
        } else { ++it; }
    }
    for (int x = -RENDER_DISTANCE; x <= RENDER_DISTANCE; x++) {
        for (int z = -RENDER_DISTANCE; z <= RENDER_DISTANCE; z++) {
            ChunkCoord target = {pChunkX + x, pChunkZ + z};
            if (activeChunks.find(target) == activeChunks.end() && chunksBeingProcessed.find(target) == chunksBeingProcessed.end()) {
                chunksBeingProcessed.insert(target);
                std::lock_guard<std::mutex> lock(loadQueueMutex); loadQueue.push(target);
            }
        }
    }
    int uploads = 0;
    {
        std::lock_guard<std::mutex> lock(uploadQueueMutex);
        while (!uploadQueue.empty() && uploads < 4) {
            ChunkRawData data = uploadQueue.back(); uploadQueue.pop_back();
            Mesh newMesh; newMesh.initialize(data);
            activeChunks[{data.cx, data.cz}] = newMesh;
            uploads++;
        }
    }
}

GLuint loadTexture(const char *path) {
    stbi_set_flip_vertically_on_load(true);
    GLuint textureID; glGenTextures(1, &textureID); glBindTexture(GL_TEXTURE_2D, textureID);
    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data) {
        GLenum format = (nrComponents == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        stbi_image_free(data);
    } else {
        unsigned char white[] = {255, 255, 255, 255};
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    return textureID;
}

void checkCollisions(glm::vec3 &pos, float &velY) {
    int bX = (int)round(pos.x / BLOCK_SCALE);
    int bZ = (int)round(pos.z / BLOCK_SCALE);
    int h = getWorldHeight((float)bX, (float)bZ);
    float groundHeight = h * BLOCK_SCALE + (BLOCK_SCALE / 2.0f) + playerHeight;
    
    if (pos.y < groundHeight) {
        if (velY <= 0) {
            pos.y = groundHeight;
            velY = 0;
            isGrounded = true;
        }
    } else {
        isGrounded = false;
    }
}

void processInput(GLFWwindow *window);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void framebuffer_size_callback(GLFWwindow *window, int width, int height);

int main(void) {
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(windowWidth, windowHeight, "Voxel World - Objects", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    globalProgramID = LoadShadersFromFile("src/shaders/lighting.vert", "src/shaders/lighting.frag");
    
    treeModel.load("src/assets/models/tree/scene.gltf");
    giantTreeModel.load("src/assets/models/giant_tree/scene.gltf");
    mushroomModel.load("src/assets/models/mushroom/scene.gltf");


    GLuint blockTexture = loadTexture("src/block.png");

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

    while (!glfwWindowShouldClose(window)) {
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

        for (auto &kv : activeChunks) {
            if (!kv.second.isReady) continue;
            glm::mat4 model = glm::mat4(1.0f);
            glm::mat4 mvp = vp * model;
            glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, &mvp[0][0]);
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);
            kv.second.render();
        }

        // 2. Рисуем ОБЪЕКТЫ (Деревья, Грибы)
        // Они используют тот же шейдер, но мы можем отключить текстуру или использовать другую
        // Для простоты используем ту же текстуру или белый цвет (если у моделей нет UV)
        
        for (auto &kv : activeChunks) {
            if (!kv.second.isReady) continue;
            
            for (auto &obj : kv.second.chunkObjects) {
                glm::mat4 objModel = glm::translate(glm::mat4(1.0f), obj.second);
                // Настраиваем масштаб моделей (gltf часто бывают огромными или маленькими)
                float scale = 1.0f; 
                
                if (obj.first == TREE_SMALL) {
                    scale = 20.0f; // Подбери значение
                    objModel = glm::scale(objModel, glm::vec3(scale));
                    // Считаем MVP и рисуем
                    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, &(vp * objModel)[0][0]);
                    treeModel.draw(objModel, modelLoc, colorAttribLoc);
                }
                else if (obj.first == TREE_GIANT) {
                    scale = 50.0f; // Большое дерево
                    objModel = glm::scale(objModel, glm::vec3(scale));
                    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, &(vp * objModel)[0][0]);
                    giantTreeModel.draw(objModel, modelLoc, colorAttribLoc);
                }
                else if (obj.first == MUSHROOM) {
                    scale = 3.0f; // Маленький гриб
                    objModel = glm::scale(objModel, glm::vec3(scale));
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

// ... (Остальные функции ввода без изменений)
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    float speed = 100.0f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        speed *= 2.5f;
    glm::vec3 frontXZ = glm::normalize(glm::vec3(cameraFront.x, 0.0f, cameraFront.z));
    glm::vec3 rightXZ = glm::normalize(glm::cross(cameraFront, cameraUp));
    glm::vec3 movement = glm::vec3(0.0f);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) movement += frontXZ * speed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) movement -= frontXZ * speed;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) movement -= rightXZ * speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) movement += rightXZ * speed;
    cameraPos += movement;
    verticalVelocity += gravity * deltaTime;
    cameraPos.y += verticalVelocity * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && isGrounded) {
        verticalVelocity = jumpForce;
        isGrounded = false;
        cameraPos.y += 1.0f;
    }
    checkCollisions(cameraPos, verticalVelocity);
}

void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) { lastX = xpos; lastY = ypos; firstMouse = false; }
    float xoffset = xpos - lastX; float yoffset = lastY - ypos;
    lastX = xpos; lastY = ypos;
    float sensitivity = 0.1f;
    yaw += xoffset * sensitivity; pitch += yoffset * sensitivity;
    if (pitch > 89.0f) pitch = 89.0f; if (pitch < -89.0f) pitch = -89.0f;
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    glViewport(0, 0, width, height);
    windowWidth = width; windowHeight = height;
}