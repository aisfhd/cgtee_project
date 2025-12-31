#pragma once
#include "chunkrawdata.h"

ChunkRawData generateChunkDataCPU(int cx, int cz);

void chunkWorkerThread();
void updateChunksManager();

void addCubeToRaw(ChunkRawData &data, float x, float y, float z, float s, glm::vec4 color, int texID);