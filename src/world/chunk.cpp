#include "chunk.h"
#include "../other/globals.h"

#include "worldgen.h"
#include <thread>

ChunkRawData generateChunkDataCPU(int cx, int cz)
{
    ChunkRawData data;
    data.cx = cx;
    data.cz = cz;
    for (int x = 0; x < CHUNK_SIZE; x++)
    {
        for (int z = 0; z < CHUNK_SIZE; z++)
        {
            int worldX = cx * CHUNK_SIZE + x;
            int worldZ = cz * CHUNK_SIZE + z;
            int h = getWorldHeight((float)worldX, (float)worldZ);
            float posX = worldX * BLOCK_SCALE;
            float posZ = worldZ * BLOCK_SCALE;

            glm::vec4 color(0.2f, 0.7f, 0.2f, 1.0f); // Трава
            int texID = 0;
            if (h > 6)
            {
                color = glm::vec4(0.9f, 0.9f, 0.95f, 1.0f);
                texID = 1;
            } // Снег
            else if (h <= 0)
            {
                color = glm::vec4(0.8f, 0.8f, 0.4f, 1.0f);
                texID = 3;
            } // Песок

            addCubeToRaw(data, posX, h * BLOCK_SCALE, posZ, BLOCK_SCALE, color, texID);

            // Земля под блоком
            for (int d = 1; d <= 2; d++)
            {
                addCubeToRaw(data, posX, (h - d) * BLOCK_SCALE, posZ, BLOCK_SCALE, glm::vec4(0.4f, 0.3f, 0.2f, 1.0f), 3);
            }

            // --- Генерация объектов (Деревья, Грибы) ---
            // Генерируем только на траве (h > 0 и h <= 6)
            if (h > 0 && h <= 6)
            {
                float r = randomDeterministic((float)worldX, (float)worldZ);

                // Чтобы не пересекались, используем else if
                // Настраиваем вероятность (0.98 = редко)
                if (r > 0.99985f)
                {
                    // Обычное дерево
                    data.objects.push_back({TREE_SMALL, glm::vec3(posX, h * BLOCK_SCALE + (BLOCK_SCALE / 2.0f), posZ)});
                }
                else if (r > 0.9997f)
                {
                    // Гриб
                    data.objects.push_back({MUSHROOM, glm::vec3(posX, h * BLOCK_SCALE + (BLOCK_SCALE / 2.0f) + 1.0f * BLOCK_SCALE, posZ)});
                }
            }
        }
    }
    return data;
}

void chunkWorkerThread()
{
    while (threadRunning)
    {
        ChunkCoord target = {0, 0};
        bool hasJob = false;
        {
            std::lock_guard<std::mutex> lock(loadQueueMutex);
            if (!loadQueue.empty())
            {
                target = loadQueue.front();
                loadQueue.pop();
                hasJob = true;
            }
        }
        if (hasJob)
        {
            ChunkRawData rawData = generateChunkDataCPU(target.x, target.z);
            {
                std::lock_guard<std::mutex> lock(uploadQueueMutex);
                uploadQueue.push_back(rawData);
            }
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }
}

void updateChunksManager()
{
    int pChunkX = (int)floor(cameraPos.x / (CHUNK_SIZE * BLOCK_SCALE));
    int pChunkZ = (int)floor(cameraPos.z / (CHUNK_SIZE * BLOCK_SCALE));
    for (auto it = activeChunks.begin(); it != activeChunks.end();)
    {
        ChunkCoord c = it->first;
        if (abs(c.x - pChunkX) > RENDER_DISTANCE + 1 || abs(c.z - pChunkZ) > RENDER_DISTANCE + 1)
        {
            it->second.cleanup();
            it = activeChunks.erase(it);
            chunksBeingProcessed.erase(c);
        }
        else
        {
            ++it;
        }
    }
    for (int x = -RENDER_DISTANCE; x <= RENDER_DISTANCE; x++)
    {
        for (int z = -RENDER_DISTANCE; z <= RENDER_DISTANCE; z++)
        {
            ChunkCoord target = {pChunkX + x, pChunkZ + z};
            if (activeChunks.find(target) == activeChunks.end() && chunksBeingProcessed.find(target) == chunksBeingProcessed.end())
            {
                chunksBeingProcessed.insert(target);
                std::lock_guard<std::mutex> lock(loadQueueMutex);
                loadQueue.push(target);
            }
        }
    }
    int uploads = 0;
    {
        std::lock_guard<std::mutex> lock(uploadQueueMutex);
        while (!uploadQueue.empty() && uploads < 4)
        {
            ChunkRawData data = uploadQueue.back();
            uploadQueue.pop_back();
            Mesh newMesh;
            newMesh.initialize(data);
            activeChunks[{data.cx, data.cz}] = newMesh;
            uploads++;
        }
    }
}

void addCubeToRaw(ChunkRawData &data, float x, float y, float z, float s, glm::vec4 color, int texID)
{
    float h = s / 2.0f;
    glm::vec3 v[] = {
        {x - h, y - h, z + h},
        {x + h, y - h, z + h},
        {x + h, y + h, z + h},
        {x - h, y + h, z + h},
        {x + h, y - h, z - h},
        {x - h, y - h, z - h},
        {x - h, y + h, z - h},
        {x + h, y + h, z - h},
    };

    float uMin = (texID % 2) * 0.5f;
    float vMin = (texID / 2) * 0.5f;
    float uMax = uMin + 0.5f;
    float vMax = vMin + 0.5f;
    glm::vec2 uv0(uMin, vMin), uv1(uMax, vMin), uv2(uMax, vMax), uv3(uMin, vMax);

    unsigned int idx_offset = data.positions.size();
    auto addFace = [&](int a, int b, int c, int d, glm::vec3 n)
    {
        data.positions.push_back(v[a]);
        data.positions.push_back(v[b]);
        data.positions.push_back(v[c]);
        data.positions.push_back(v[d]);
        for (int i = 0; i < 4; i++)
        {
            data.colors.push_back(color);
            data.normals.push_back(n);
        }
        data.uvs.push_back(uv0);
        data.uvs.push_back(uv1);
        data.uvs.push_back(uv2);
        data.uvs.push_back(uv3);
        data.indices.push_back(idx_offset);
        data.indices.push_back(idx_offset + 1);
        data.indices.push_back(idx_offset + 2);
        data.indices.push_back(idx_offset + 2);
        data.indices.push_back(idx_offset + 3);
        data.indices.push_back(idx_offset);
        idx_offset += 4;
    };

    addFace(0, 1, 2, 3, glm::vec3(0, 0, 1));
    addFace(4, 5, 6, 7, glm::vec3(0, 0, -1));
    addFace(1, 4, 7, 2, glm::vec3(1, 0, 0));
    addFace(5, 0, 3, 6, glm::vec3(-1, 0, 0));
    addFace(3, 2, 7, 6, glm::vec3(0, 1, 0));
    addFace(5, 4, 1, 0, glm::vec3(0, -1, 0));
}
