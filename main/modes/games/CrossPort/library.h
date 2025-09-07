#ifndef LIBRARY_H
#define LIBRARY_H

#include <math.h>
#include <stdint.h>
#include <esp_log.h>
#include <esp_random.h>
#include "shapes.h"
#include "fill.h"
#include "hdw-tft.h"
#include "esp_heap_caps.h"
#include "hdw-btn.h"

#include "structs.h"

#define M_PI 3.1415927f
#define TWO_PI 6.2831853f

#define MAX_DISTX 0
#define MAX_DISTY 20
#define MAX_DISTZ 0

#define CHUNK_DISTX 1
#define CHUNK_DISTY 1
#define CHUNK_DISTZ 1
#define CHUNK_AMT ((2 * CHUNK_DISTX + 1) * (2 * CHUNK_DISTY + 1) * (2 * CHUNK_DISTZ + 1))
#define CHUNK_SIZEX 7
#define CHUNK_SIZEY 7
#define CHUNK_SIZEZ 7
#define MAX_OBJECTS ((CHUNK_SIZEX * CHUNK_SIZEY * CHUNK_SIZEZ) * CHUNK_AMT)
#define MAX_OBJECTS_RENDERED ((MAX_OBJECTS * 2) / 3)

#define TEXTURE_LIMIT 1

extern int sX;
extern int sY;
extern int sW;
extern int sH;
extern int* blocks[CHUNK_AMT];
extern int chunkDirty[CHUNK_AMT];
extern chunkpos chunkOffsets[CHUNK_AMT];
extern Vect3i tilePosMove[36];
extern const int uv[2][3][2];
extern int rayPosition[2][4];
extern buttonState btn;

int randomInt(int a, int b);
float randomFloat(float a, float b);
void placeBlock(int block, int x, int y, int z);
float degToRad(float deg);
float radToDeg(float rad);
float lerp(float t, float a, float b);
float fade(float t);
float dot(Vect3f a, Vect3f b);
float perlinNoise2D(float x, float y);
float perlinNoise3D(float x, float y, float z);
void initPerlinNoise(unsigned int seed);
int viewFrustrum3D(float x, float y, float z, float fovX, float fovY, float nearPlane, float farPlane);
void swapVerts(int a[2], int b[2]);
void setupTilePos();

#endif