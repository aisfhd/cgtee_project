#include "worldgen.h"
#include "../other/globals.h"
#include <math.h>
#include <glm/glm.hpp>
float noise(float x, float y)
{
    int n = int(x * 40.0 + y * 6400.0);
    n = (n << 13) ^ n;
    return 1.0 - float((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0;
}
float interpolate(float a, float b, float x)
{
    float ft = x * 3.1415927f;
    float f = (1 - cos(ft)) * 0.5f;
    return a * (1 - f) + b * f;
}
float smoothNoise(float x, float y)
{
    float corners = (noise(x - 1, y - 1) + noise(x + 1, y - 1) + noise(x - 1, y + 1) + noise(x + 1, y + 1)) / 16.0f;
    float sides = (noise(x - 1, y) + noise(x + 1, y) + noise(x, y - 1) + noise(x, y + 1)) / 8.0f;
    float center = noise(x, y) / 4.0f;
    return corners + sides + center;
}
float perlinNoise(float x, float y)
{
    float intX = floor(x);
    float fractionalX = x - intX;
    float intY = floor(y);
    float fractionalY = y - intY;
    return interpolate(interpolate(smoothNoise(intX, intY), smoothNoise(intX + 1, intY), fractionalX),
                       interpolate(smoothNoise(intX, intY + 1), smoothNoise(intX + 1, intY + 1), fractionalX), fractionalY);
}
int getWorldHeight(float x, float z)
{
    float val = perlinNoise(x * NOISE_SCALE, z * NOISE_SCALE) * 1.0f + perlinNoise(x * NOISE_SCALE * 2.0f, z * NOISE_SCALE * 2.0f) * 0.5f;
    return (int)(val * 12.0f);
}

float randomDeterministic(float x, float z)
{
    return glm::fract(sin(glm::dot(glm::vec2(x, z), glm::vec2(12.9898, 78.233))) * 43758.5453);
}