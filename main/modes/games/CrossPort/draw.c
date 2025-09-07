#include "draw.h"

// front/back/left/right/bottom/top
const int faceAdd[6] = {1, 2, 1, 2, 3, 0};
const int transparency[3] = {0, 0, 1};
const int blockColor[3] = {0, 1, 0};
const int pattern[4][2][2] = {
    {{0,1},{0,0}},
    {{1,0},{0,1}},
    {{1,1},{1,0}},
    {{1,1},{1,1}}
};

static void blockCol(int x, int y, int block, int faceIdx) {
    int shade = blockColor[block] + faceAdd[faceIdx];
    if (shade > 3){ shade = 3; }

    int px = x % 2;
    int py = y % 2;

    if (pattern[shade][py][px]){ TURBO_SET_PIXEL( x, y, c555 ); } else { TURBO_SET_PIXEL( x, y, c000 ); }
}

void project2D(int point[2], float verts[3], float fov, float nearPlane) {
    float z = verts[2];
    if (z < nearPlane) z = nearPlane;
    
    float projected_x = verts[0] * (fov / z);
    float projected_y = verts[1] * (fov / z);
    
    point[0] = (int)(projected_x + ((sW / 2) + sX));
    point[1] = (int)(-projected_y + ((sH / 2) + sY));
}

void RotationMatrix(float x, float y, float z, float sin1, float cos1, float sin2, float cos2, float sin3, float cos3, float rot[3]){
    float tempX = (z * sin1) + (x * cos1);
    float tempZ = (z * cos1) - (x * sin1);

    float tempY = (tempZ * sin2) + (y * cos2);
    float finalZ = (tempZ * cos2) - (y * sin2);

    float finalX = (tempX * cos3) - (tempY * sin3);
    float finalY = (tempX * sin3) + (tempY * cos3);

    rot[0] = finalX;
    rot[1] = finalY;
    rot[2] = finalZ;
}

void RotateVertexObject(float x, float y, float z, float objRotX, float objRotY, float objRotZ, float sx, float sy, float sz, float out[3]) {
    x *= sx;
    y *= sy;
    z *= sz;

    float sinX = sinf(objRotX), cosX = cosf(objRotX);
    float sinY = sinf(objRotY), cosY = cosf(objRotY);
    float sinZ = sinf(objRotZ), cosZ = cosf(objRotZ);
    
    float x1 = x;
    float y1 = y * cosX - z * sinX;
    float z1 = y * sinX + z * cosX;
    
    float x2 = x1 * cosY + z1 * sinY;
    float y2 = y1;
    float z2 = -x1 * sinY + z1 * cosY;
    
    float x3 = x2 * cosZ - y2 * sinZ;
    float y3 = x2 * sinZ + y2 * cosZ;
    float z3 = z2;

    out[0] = x3;
    out[1] = y3;
    out[2] = z3;
}

int windingOrder(int *p0, int *p1, int *p2) { return (p0[0]*p1[1] - p0[1]*p1[0] + p1[0]*p2[1] - p1[1]*p2[0] + p2[0]*p0[1] - p2[1]*p0[0] > 0); }

void drawTri(int tris[3][2]){
    drawLineFast(tris[0][0], tris[0][1], tris[1][0], tris[1][1], c000);
    drawLineFast(tris[1][0], tris[1][1], tris[2][0], tris[2][1], c000);
    drawLineFast(tris[2][0], tris[2][1], tris[0][0], tris[0][1], c000);
}

void drawFilledTris(int tris[3][2], int block, int index) {
    int sorted[3][2];
    for(int i=0;i<3;i++){
        for(int j=0;j<2;j++){
            sorted[i][j] = tris[i][j];
        }
    }

    if (sorted[0][1] > sorted[1][1]) swapVerts(sorted[0], sorted[1]);
    if (sorted[0][1] > sorted[2][1]) swapVerts(sorted[0], sorted[2]);
    if (sorted[1][1] > sorted[2][1]) swapVerts(sorted[1], sorted[2]);

    int* v0 = sorted[0];
    int* v1 = sorted[1];
    int* v2 = sorted[2];

    int totalHeight = v2[1] - v0[1];
    if (totalHeight == 0) return;

    int yStart = v0[1];
    int yEnd = v2[1];

    if (yStart < 0) yStart = 0;
    if (yEnd > sH) yEnd = sH;
    
    for (int y = yStart; y < yEnd; y++) {
        bool secondHalf = y > v1[1] || v1[1] == v0[1];

        int segmentHeight = secondHalf ? (v2[1] - v1[1]) : (v1[1] - v0[1]);
        if (segmentHeight == 0) continue;
        
        int alpha = ((y - v0[1]) << 16) / totalHeight;
        int beta = ((y - (secondHalf ? v1[1] : v0[1])) << 16) / segmentHeight;

        int ax = v0[0] + ((v2[0] - v0[0]) * alpha >> 16);
        int bx = secondHalf
            ? v1[0] + ((v2[0] - v1[0]) * beta >> 16)
            : v0[0] + ((v1[0] - v0[0]) * beta >> 16);

        if (ax > bx) {
            int tmp = ax; ax = bx; bx = tmp;
        }

        int xStart = ax < 0 ? 0 : ax;
        int xEnd = bx > sW ? sW : bx;

        for (int x = xStart; x < xEnd; x++) {
            // TURBO_SET_PIXEL( x, y, c555 );
            blockCol(x, y, block, index);
        }
    }
}

int TriangleClipping(float verts[3][3], float outTri1[3][3], float outTri2[3][3], float fovX, float fovY, float nearPlane, float farPlane){
    // float t = (nearPlane - verts[out][2]) - (verts[in][2] - verts[out][2])
    // float xAtCrossing = lerp(t, verts[out][0], verts[in][0])
    // float yAtCrossing = lerp(t, verts[out][1], verts[in][1])
    // float crossingPoint = {xAtCrossing, yAtCrossing, nearPlane};

    int inScreen[3];
    int outScreen[3];
    int inAmt = 0;
    int outAmt = 0;

    for (int i=0; i < 3; i++){
        float z = verts[i][2];
        if (z >= nearPlane && z <= farPlane) {
            inScreen[inAmt++] = i;
        } else {
            outScreen[outAmt++] = i;
        }
    }

    if (inAmt == 0){
        return 0;
    } else if (inAmt == 3){
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                outTri1[i][j] = verts[i][j];
            }
        }
        return 1;
    } else if (inAmt == 1) {
        int in0 = inScreen[0];
        int out0 = outScreen[0];
        int out1 = outScreen[1];

        float cross0[3];
        float cross1[3];

        for(int j=0;j<3;j++){
            cross0[j] = verts[out0][j] + ((nearPlane - verts[out0][2]) / (verts[in0][2] - verts[out0][2])) * (verts[in0][j] - verts[out0][j]);
            cross1[j] = verts[out1][j] + ((nearPlane - verts[out1][2]) / (verts[in0][2] - verts[out1][2])) * (verts[in0][j] - verts[out1][j]);
        }

        outTri1[0][0] = verts[in0][0];
        outTri1[0][1] = verts[in0][1];
        outTri1[0][2] = verts[in0][2];
        outTri1[1][0] = cross0[0];
        outTri1[1][1] = cross0[1];
        outTri1[1][2] = cross0[2];
        outTri1[2][0] = cross1[0];
        outTri1[2][1] = cross1[1];
        outTri1[2][2] = cross1[2];

        return 1;
    } else if (inAmt == 2) {
        int in0 = inScreen[0];
        int in1 = inScreen[1];
        int out0 = outScreen[0];
        
        float cross0[3];
        float cross1[3];
    
        float t0 = (nearPlane - verts[out0][2]) / (verts[in0][2] - verts[out0][2]);
        float t1 = (nearPlane - verts[out0][2]) / (verts[in1][2] - verts[out0][2]);

        for (int j=0; j<3; j++){
            cross0[j] = verts[out0][j] + t0 * (verts[in0][j] - verts[out0][j]);
            cross1[j] = verts[out0][j] + t1 * (verts[in1][j] - verts[out0][j]);
        }
        
        outTri1[0][0] = verts[in0][0];
        outTri1[0][1] = verts[in0][1];
        outTri1[0][2] = verts[in0][2];
        outTri1[1][0] = verts[in1][0];
        outTri1[1][1] = verts[in1][1];
        outTri1[1][2] = verts[in1][2];
        outTri1[2][0] = cross1[0];
        outTri1[2][1] = cross1[1];
        outTri1[2][2] = cross1[2];

        outTri2[0][0] = verts[in0][0];
        outTri2[0][1] = verts[in0][1];
        outTri2[0][2] = verts[in0][2];
        outTri2[1][0] = cross0[0];
        outTri2[1][1] = cross0[1];
        outTri2[1][2] = cross0[2];
        outTri2[2][0] = cross1[0];
        outTri2[2][1] = cross1[1];
        outTri2[2][2] = cross1[2];
    
        return 2;
    }
    return 0;
}