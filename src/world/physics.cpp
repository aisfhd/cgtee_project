#include "physics.h"
#include "../other/globals.h"
#include "worldgen.h"
#include "mesh.h"
#include <map>

extern std::map<ChunkCoord, Mesh> activeChunks;

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

    int pChunkX = (int)floor(pos.x / (CHUNK_SIZE * BLOCK_SCALE));
    int pChunkZ = (int)floor(pos.z / (CHUNK_SIZE * BLOCK_SCALE));

    for (int cx = pChunkX - 1; cx <= pChunkX + 1; cx++) {
        for (int cz = pChunkZ - 1; cz <= pChunkZ + 1; cz++) {
            ChunkCoord coord = {cx, cz};
            if (activeChunks.find(coord) != activeChunks.end()) {
                Mesh &mesh = activeChunks[coord];
                
                for (auto &obj : mesh.chunkObjects) {
                    float dx = pos.x - obj.second.x;
                    float dz = pos.z - obj.second.z;
                    float distSq = dx * dx + dz * dz;

                    float colRadius = 0.0f;
                    if (obj.first == TREE_SMALL) colRadius = 8.0f;
                    else if (obj.first == MUSHROOM) colRadius = 4.0f;

                    if (distSq < colRadius * colRadius && distSq > 0.001f) {
                        float dist = sqrt(distSq);
                        float overlap = colRadius - dist;
                        pos.x += (dx / dist) * overlap;
                        pos.z += (dz / dist) * overlap;
                    }
                }
            }
        }
    }
}