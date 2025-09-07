#ifndef DRAW_H
#define DRAW_H
#include "library.h"

void project2D(int point[2], float verts[3], float fov, float nearPlane);
void drawTri(int tris[3][2], paletteColor_t color);
void drawTriOutline(int tris[3][2], int t);
void RotationMatrix(float x, float y, float z, float sin1, float cos1, float sin2, float cos2, float sin3, float cos3, float rot[3]);
void RotateVertexObject(float x, float y, float z, float objRotX, float objRotY, float objRotZ, float sx, float sy, float sz, float out[3]);
void drawFilledTris(int tris[3][2], int block, int index);
int TriangleClipping(float verts[3][3], float outTri1[3][3], float outTri2[3][3], float fovX, float fovY, float nearPlane, float farPlane);
int windingOrder(int *p0, int *p1, int *p2);

#endif