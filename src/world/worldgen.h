#pragma once
float noise(float x, float y);
float interpolate(float a, float b, float x);
float smoothNoise(float x, float y);
float perlinNoise(float x, float y);
int getWorldHeight(float x, float z);

float randomDeterministic(float x, float z);