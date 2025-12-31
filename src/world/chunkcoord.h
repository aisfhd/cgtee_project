#pragma once
struct ChunkCoord {
    int x, z;
    bool operator<(const ChunkCoord &o) const { return x < o.x || (x == o.x && z < o.z); }
};
