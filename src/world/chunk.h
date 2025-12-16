#pragma once
#include <glm/vec2.hpp>
namespace cgtee
{
    const int CHUNK_SIZE_X = 32;
    const int CHUNK_SIZE_Y = 256; // Might be higher for large worlds
    const int CHUNK_SIZE_Z = 32;
    using BlockID = unsigned char;
    struct Chunk{
        BlockID ChunkBlocks[CHUNK_SIZE_X][CHUNK_SIZE_Y][CHUNK_SIZE_Z];
        glm::vec2 ChunkLocation;

        bool MeshUpdateRequired = true;
        bool IsGenerated = false;
        // void render();
        // void MeshUpdate();
        // void init();
        // void generate();

    };
};