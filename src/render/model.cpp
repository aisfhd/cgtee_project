#define TINYGLTF_IMPLEMENTATION
#include "model.h"
#include "../other/pathglobals.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include "texture.h"
void ModelGLTF::draw(glm::mat4 vp, glm::mat4 modelMatrix, GLuint mvpLoc, GLuint modelLoc, GLuint colorLoc)
{
    if (!loaded)
        return;
    glm::mat4 fullModel = modelMatrix * modelTransform;
    // Красим модель в белый (или текстуру, если есть)
    if (colorLoc > 0)
        glVertexAttrib4f(colorLoc, 1.0f, 1.0f, 1.0f, 1.0f);
    for (const auto& subMesh : subMeshes) {
        glm::mat4 subModel = fullModel * subMesh.subTransform;
        glm::mat4 mvp = vp * subModel;
        glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, &mvp[0][0]);
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &subModel[0][0]);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, subMesh.textureID);
        glBindVertexArray(subMesh.vao);
        glDrawElements(GL_TRIANGLES, subMesh.indexCount, GL_UNSIGNED_SHORT, 0);
        glBindVertexArray(0);
    }
}

void ModelGLTF::load(const std::string &modelName)
{
    std::string gltfModelDir = globalPath + assetsPath + "\\models\\" + modelName;
    std::string gltfFilename = gltfModelDir + "\\scene.gltf";
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;

    // Set dummy image loader to avoid errors
    loader.SetImageLoader([](tinygltf::Image *image, const int image_idx, std::string *err, std::string *warn, int req_width, int req_height, const unsigned char *bytes, int size, void *user_data) {
        return true;
    }, nullptr);

    bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, gltfFilename);
    // Если у тебя бинарный glb, используй LoadBinaryFromFile

    if (!warn.empty())
        std::cout << "Warn: " << warn << std::endl;
    if (!err.empty())
        std::cout << "Err: " << err << std::endl;
    if (!ret)
    {
        std::cout << "Failed to parse glTF: " << gltfFilename << std::endl;
        return;
    }

    // Compute model transform from the first node in the default scene
    glm::mat4 rootTransform = glm::mat4(1.0f);
    if (model.scenes.size() > 0) {
        int scene_idx = 0;
        const tinygltf::Scene& scene = model.scenes[scene_idx];
        if (scene.nodes.size() > 0) {
            int node_idx = scene.nodes[0];
            const tinygltf::Node& node = model.nodes[node_idx];
            if (node.matrix.size() == 16) {
                for (int i = 0; i < 16; ++i) {
                    rootTransform[i / 4][i % 4] = static_cast<float>(node.matrix[i]);
                }
            } else {
                glm::vec3 scale(1.0f), trans(0.0f);
                glm::quat rot(1.0f, 0.0f, 0.0f, 0.0f);
                if (node.scale.size() == 3) {
                    scale = glm::vec3(static_cast<float>(node.scale[0]), static_cast<float>(node.scale[1]), static_cast<float>(node.scale[2]));
                }
                if (node.rotation.size() == 4) {
                    rot = glm::quat(static_cast<float>(node.rotation[3]), static_cast<float>(node.rotation[0]), static_cast<float>(node.rotation[1]), static_cast<float>(node.rotation[2]));
                }
                if (node.translation.size() == 3) {
                    trans = glm::vec3(static_cast<float>(node.translation[0]), static_cast<float>(node.translation[1]), static_cast<float>(node.translation[2]));
                }
                rootTransform = glm::translate(glm::mat4(1.0f), trans) * glm::mat4_cast(rot) * glm::scale(glm::mat4(1.0f), scale);
            }
        }
    }

    // Set modelTransform to identity to apply root transform to submeshes
    modelTransform = glm::mat4(1.0f);

    // Helper function to get node transform
    auto getNodeTransform = [](const tinygltf::Node& node) -> glm::mat4 {
        if (!node.matrix.empty()) {
            glm::mat4 m(1.0f);
            for (int i = 0; i < 16; ++i) {
                m[i / 4][i % 4] = static_cast<float>(node.matrix[i]);
            }
            return m;
        } else {
            glm::vec3 translation(0.0f);
            if (!node.translation.empty()) {
                translation = glm::vec3(node.translation[0], node.translation[1], node.translation[2]);
            }
            glm::quat rotation(1.0f, 0.0f, 0.0f, 0.0f);
            if (!node.rotation.empty()) {
                rotation = glm::quat(node.rotation[3], node.rotation[0], node.rotation[1], node.rotation[2]);
            }
            glm::vec3 scale(1.0f);
            if (!node.scale.empty()) {
                scale = glm::vec3(node.scale[0], node.scale[1], node.scale[2]);
            }
            return glm::translate(glm::mat4(1.0f), translation) * glm::mat4_cast(rotation) * glm::scale(glm::mat4(1.0f), scale);
        }
    };

    // Recursive function to process nodes
    std::function<void(const tinygltf::Node&, glm::mat4, std::vector<SubMesh>&)> processNode = [&](const tinygltf::Node& node, glm::mat4 parentTransform, std::vector<SubMesh>& subMeshes) {
        glm::mat4 localTransform = getNodeTransform(node);
        glm::mat4 worldTransform = parentTransform * localTransform;

        if (node.mesh >= 0) {
            const tinygltf::Mesh& mesh = model.meshes[node.mesh];
            for (size_t primIdx = 0; primIdx < mesh.primitives.size(); ++primIdx) {
                const tinygltf::Primitive& primitive = mesh.primitives[primIdx];

                SubMesh subMesh;
                subMesh.subTransform = worldTransform;
                subMesh.textureID = 0; // default

                // Get texture from material
                if (primitive.material >= 0 && primitive.material < model.materials.size()) {
                    const tinygltf::Material& mat = model.materials[primitive.material];
                    if (mat.pbrMetallicRoughness.baseColorTexture.index >= 0) {
                        int texIndex = mat.pbrMetallicRoughness.baseColorTexture.index;
                        const tinygltf::Texture& texture = model.textures[texIndex];
                        int imgIndex = texture.source;
                        const tinygltf::Image& image = model.images[imgIndex];
                        std::string texturePath = gltfModelDir + "\\" + image.uri;
                        subMesh.textureID = loadTexture(texturePath.c_str());
                    }
                }

                // Load POSITION
                if (primitive.attributes.find("POSITION") != primitive.attributes.end()) {
                    const tinygltf::Accessor& posAccessor = model.accessors[primitive.attributes.at("POSITION")];
                    const tinygltf::BufferView& posBufferView = model.bufferViews[posAccessor.bufferView];
                    const tinygltf::Buffer& posBuffer = model.buffers[posBufferView.buffer];

                    std::vector<glm::vec3> positions;
                    const unsigned char* posData = &posBuffer.data[posBufferView.byteOffset + posAccessor.byteOffset];
                    size_t posStride = posBufferView.byteStride;
                    if (posStride == 0) posStride = 3 * sizeof(float);
                    for (size_t i = 0; i < posAccessor.count; ++i) {
                        const float* pos = (const float*)(posData + i * posStride);
                        positions.push_back(glm::vec3(pos[0], pos[1], pos[2]));
                    }

                    // Load NORMAL
                    std::vector<glm::vec3> normals;
                    if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
                        const tinygltf::Accessor& normAccessor = model.accessors[primitive.attributes.at("NORMAL")];
                        const tinygltf::BufferView& normBufferView = model.bufferViews[normAccessor.bufferView];
                        const tinygltf::Buffer& normBuffer = model.buffers[normBufferView.buffer];
                        const unsigned char* normData = &normBuffer.data[normBufferView.byteOffset + normAccessor.byteOffset];
                        size_t normStride = normBufferView.byteStride;
                        if (normStride == 0) normStride = 3 * sizeof(float);
                        for (size_t i = 0; i < normAccessor.count; ++i) {
                            const float* norm = (const float*)(normData + i * normStride);
                            normals.push_back(glm::vec3(norm[0], norm[1], norm[2]));
                        }
                    } else {
                        normals.assign(positions.size(), glm::vec3(0, 1, 0));
                    }

                    // Load TEXCOORD_0
                    std::vector<glm::vec2> texcoords;
                    if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
                        const tinygltf::Accessor& texAccessor = model.accessors[primitive.attributes.at("TEXCOORD_0")];
                        const tinygltf::BufferView& texBufferView = model.bufferViews[texAccessor.bufferView];
                        const tinygltf::Buffer& texBuffer = model.buffers[texBufferView.buffer];
                        const unsigned char* texData = &texBuffer.data[texBufferView.byteOffset + texAccessor.byteOffset];
                        size_t texStride = texBufferView.byteStride;
                        if (texStride == 0) texStride = 2 * sizeof(float);
                        for (size_t i = 0; i < texAccessor.count; ++i) {
                            const float* tex = (const float*)(texData + i * texStride);
                            texcoords.push_back(glm::vec2(tex[0], 1.0f - tex[1]));
                        }
                    } else {
                        texcoords.assign(positions.size(), glm::vec2(0, 0));
                    }

                    // Create vertices
                    std::vector<Vertex> vertices;
                    for (size_t i = 0; i < positions.size(); ++i) {
                        vertices.push_back({ positions[i], normals[i], texcoords[i] });
                    }

                    glGenVertexArrays(1, &subMesh.vao);
                    glBindVertexArray(subMesh.vao);

                    glGenBuffers(1, &subMesh.vbo);
                    glBindBuffer(GL_ARRAY_BUFFER, subMesh.vbo);
                    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

                    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
                    glEnableVertexAttribArray(0);
                    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(float)));
                    glEnableVertexAttribArray(2);
                    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(6 * sizeof(float)));
                    glEnableVertexAttribArray(3);

                    // Load indices
                    if (primitive.indices >= 0) {
                        const tinygltf::Accessor& indexAccessor = model.accessors[primitive.indices];
                        const tinygltf::BufferView& indexBufferView = model.bufferViews[indexAccessor.bufferView];
                        const tinygltf::Buffer& indexBuffer = model.buffers[indexBufferView.buffer];

                        std::vector<unsigned short> indices;
                        const unsigned char* indexData = &indexBuffer.data[indexBufferView.byteOffset + indexAccessor.byteOffset];
                        size_t indexStride = indexAccessor.ByteStride(indexBufferView);
                        for (size_t i = 0; i < indexAccessor.count; ++i) {
                            indices.push_back(*(unsigned short*)(indexData + i * indexStride));
                        }

                        glGenBuffers(1, &subMesh.ebo);
                        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, subMesh.ebo);
                        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned short), indices.data(), GL_STATIC_DRAW);

                        subMesh.indexCount = indexAccessor.count;
                    }

                    subMeshes.push_back(subMesh);
                }
            }
        }

        for (int childIdx : node.children) {
            processNode(model.nodes[childIdx], worldTransform, subMeshes);
        }
    };

    // Traverse the scene
    std::vector<SubMesh> tempSubMeshes;
    if (!model.scenes.empty()) {
        int sceneIdx = model.defaultScene >= 0 ? model.defaultScene : 0;
        const tinygltf::Scene& scene = model.scenes[sceneIdx];
        for (int nodeIdx : scene.nodes) {
            processNode(model.nodes[nodeIdx], glm::mat4(1.0f), tempSubMeshes);
        }
    }

    subMeshes = tempSubMeshes;
    loaded = true;
    std::cout << "Loaded model: " << gltfFilename << " with " << subMeshes.size() << " submeshes" << std::endl;

    // Apply correction to upright the models
    glm::mat4 correction = glm::inverse(rootTransform);
    for (auto& sub : subMeshes) {
        sub.subTransform = correction * sub.subTransform;
    }

    // Adjust submesh transforms for mushroom
    if (modelName == "mushroom" && subMeshes.size() == 2) {
        subMeshes[1].subTransform = subMeshes[1].subTransform * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 15.0f, 0.0f));
    }
    glBindVertexArray(0);
}