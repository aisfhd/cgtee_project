#define TINYGLTF_IMPLEMENTATION
#include "model.h"
void ModelGLTF::draw(glm::mat4 modelMatrix, GLuint modelLoc, GLuint colorLoc)
{
    if (!loaded)
        return;
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &modelMatrix[0][0]);
    // Красим модель в белый (или текстуру, если есть)
    glVertexAttrib4f(1, 1.0f, 1.0f, 1.0f, 1.0f);
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_SHORT, 0); // Обычно gltf использует short indices
    glBindVertexArray(0);
}

void ModelGLTF::load(const std::string &filename)
{
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;

    bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, filename);
    // Если у тебя бинарный glb, используй LoadBinaryFromFile

    if (!warn.empty())
        std::cout << "Warn: " << warn << std::endl;
    if (!err.empty())
        std::cout << "Err: " << err << std::endl;
    if (!ret)
    {
        std::cout << "Failed to parse glTF: " << filename << std::endl;
        return;
    }

    // Упрощенная загрузка ПЕРВОГО примитива ПЕРВОГО меша
    if (model.meshes.size() > 0 && model.meshes[0].primitives.size() > 0)
    {
        const tinygltf::Primitive &primitive = model.meshes[0].primitives[0];

        // Получаем координаты (POSITION)
        if (primitive.attributes.find("POSITION") != primitive.attributes.end())
        {
            const tinygltf::Accessor &accessor = model.accessors[primitive.attributes.at("POSITION")];
            const tinygltf::BufferView &bufferView = model.bufferViews[accessor.bufferView];
            const tinygltf::Buffer &buffer = model.buffers[bufferView.buffer];

            // TODO: Здесь нужен более сложный код для полноценной загрузки
            // Для простоты примера мы предполагаем, что данные упакованы просто float3
            // В реальном проекте нужно учитывать stride, componentType и т.д.

            glGenVertexArrays(1, &vao);
            glBindVertexArray(vao);

            glGenBuffers(1, &vbo);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, bufferView.byteLength, &buffer.data[bufferView.byteOffset], GL_STATIC_DRAW);

            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
            glEnableVertexAttribArray(0);

            // Нормали и UV опускаем для краткости, но они нужны для освещения!
            // Если модель черная - значит нет нормалей.

            if (primitive.indices >= 0)
            {
                const tinygltf::Accessor &indexAccessor = model.accessors[primitive.indices];
                const tinygltf::BufferView &indexBufferView = model.bufferViews[indexAccessor.bufferView];
                const tinygltf::Buffer &indexBuffer = model.buffers[indexBufferView.buffer];

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