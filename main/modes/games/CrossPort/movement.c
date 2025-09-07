#include "movement.h"

float pMovement[3];
int grounded = 0;
float frict = 1.5f;
float fallVel = 0.03f;

int rayPosition[2][4];
int rayBlock[3];
int placeable = 0;
int lockMine = 0;
int mineTimer = 0;
int chunkDirty[CHUNK_AMT];

int lastRay[4];
int rayCnk[3];

#define LARGE_FLOAT 1e30f

static int checkCollisions(Vect3i block, Camera* pos) {
    float bw = 0.3f;
    float bht = 0.5f;
    float bhb = 1.0f;
    float bd = 0.3f;

    int minX = block.x;
    int minY = block.y;
    int minZ = block.z;

    int maxX = block.x + 1;
    int maxY = block.y + 1;
    int maxZ = block.z + 1;

    if (pos->position.x + bw > minX && pos->position.x - bw < maxX &&
        pos->position.y + bht > minY && pos->position.y - bhb < maxY &&
        pos->position.z + bd > minZ && pos->position.z - bd < maxZ) {
        return 1;
    }

    return 0;
}

static void collCheck(Camera* c, Vect3i* cList, int listAmt){
    for (int i=0; i < 3; i++){
        if (i==0){
            moveCamera(c, 0,  pMovement[1], 0);
            grounded = 0;
        } else if (i==1){
            moveCamera(c, pMovement[0], 0, 0);
        } else {
            moveCamera(c, 0, 0,  pMovement[2]);
        }
        
        for (int amt=0; amt < listAmt; amt++){
            if (i==0){
                if (checkCollisions(cList[amt], c)){
                    moveCamera(c, 0, -pMovement[1], 0);
                    grounded = 1;
                    pMovement[1] = 0.0f;
                    break;
                }
            } else if (i==1){
                if (checkCollisions(cList[amt], c)){
                    moveCamera(c, -pMovement[0], 0, 0);
                    pMovement[0] = 0.0f;
                    break;
                }
            } else {
                if (checkCollisions(cList[amt], c)){
                    moveCamera(c, 0, 0, -pMovement[2]);
                    pMovement[2] = 0.0f;
                    break;
                }
            }
        }
    }

    pMovement[0] /= frict;
    pMovement[2] /= frict;
    pMovement[1] -= fallVel;
}

void movePlayer(Camera* c, Vect3i* Collidables, int amt){
    float move_delta = 0.03f;
    float rotY_delta = -0.03f;
    float rotX_delta = -0.1f;
    
    float yaw = c->rotation.y;
    float rotation[3] = {0.0f, 0.0f, 0.0f};

    // === Movement ===
    if (btn.input & PB_A && grounded==1){
        pMovement[1] = 0.3f;
        pMovement[0] *= 3.2f;
        pMovement[2] *= 3.2f;
        grounded = 0;
    }

    if (btn.input & PB_UP) {
        pMovement[0] += move_delta * sinf(yaw);
        pMovement[2] += move_delta * cosf(yaw);
    }
    if (btn.input & PB_DOWN) {
        pMovement[0] -= move_delta * sinf(yaw);
        pMovement[2] -= move_delta * cosf(yaw);
    }

    if (pMovement[0] < -2.0f || pMovement[0] > 2.0f){
        pMovement[0] /= 2.0f;
    }

    if (pMovement[1] < -1.5f){
        pMovement[1] = -1.5f;
    }

    if (pMovement[2] > 2.0f){
        pMovement[2] = 2.0f;
    } else if (pMovement[2] < -2.0f){
        pMovement[2] = -2.0f;
    }

    // === Rotation ===
    // float crankDelta = pd->system->getCrankChange();
    // rotation[0] += crankDelta * rotY_delta;
    if (btn.input & PB_LEFT) {
        rotation[1] += rotX_delta;
    }
    if (btn.input & PB_RIGHT) {
        rotation[1] -= rotX_delta;
    }
    
    rotateCamera(c, rotation[0], rotation[1], rotation[2]);
    if (c->rotation.y < 0.0f) {
        c->rotation.y = degToRad(360.0f);
    }
    if (c->rotation.y > degToRad(360.0f)){
        c->rotation.y = 0.0f;
    }

    if (c->rotation.x < degToRad(-90.0f)){
        c->rotation.x = degToRad(-90.0f);
    } else if (c->rotation.x > degToRad(90.0f)) {
        c->rotation.x = degToRad(90.0f);
    }

    collCheck(c, Collidables, amt);
}

void raycast(Camera* c) {
    lastRay[0] = 0;
    lastRay[1] = 0;
    lastRay[2] = 0;
    lastRay[3] = 0;

    rayCnk[0] = 0;
    rayCnk[1] = 0;
    rayCnk[2] = 0;

    float rayX = c->position.x;
    float rayY = c->position.y;
    float rayZ = c->position.z;

    float yaw = c->rotation.y;
    float pitch = c->rotation.x;

    float dx = cosf(pitch) * sinf(yaw);
    float dy = sinf(pitch);
    float dz = cosf(pitch) * cosf(yaw);
    
    int ix = (int)floorf(rayX);
    int iy = (int)floorf(rayY);
    int iz = (int)floorf(rayZ);
    
    int stepX = (dx > 0) ? 1 : -1;
    int stepY = (dy > 0) ? 1 : -1;
    int stepZ = (dz > 0) ? 1 : -1;
    
    float tDeltaX = (dx != 0.0f && !isnan(dx)) ? fabsf(1.0f / dx) : LARGE_FLOAT;
    float tDeltaY = (dy != 0.0f && !isnan(dy)) ? fabsf(1.0f / dy) : LARGE_FLOAT;
    float tDeltaZ = (dz != 0.0f && !isnan(dz)) ? fabsf(1.0f / dz) : LARGE_FLOAT;

    float tMaxX = (dx > 0) ? (floorf(rayX + 1) - rayX) * tDeltaX : (rayX - floorf(rayX)) * tDeltaX;
    float tMaxY = (dy > 0) ? (floorf(rayY + 1) - rayY) * tDeltaY : (rayY - floorf(rayY)) * tDeltaY;
    float tMaxZ = (dz > 0) ? (floorf(rayZ + 1) - rayZ) * tDeltaZ : (rayZ - floorf(rayZ)) * tDeltaZ;

    int chnk = -1;
    placeable = 0;

    for (int i = 0; i < 50; i++) {
        rayCnk[0] = (int)floorf((float)ix / CHUNK_SIZEX);
        rayCnk[1] = (int)floorf((float)iy / CHUNK_SIZEY);
        rayCnk[2] = (int)floorf((float)iz / CHUNK_SIZEZ);

        chnk = -1;
        for (int c_ = 0; c_ < CHUNK_AMT; c_++) {
            if (chunkOffsets[c_].x == rayCnk[0] &&
                chunkOffsets[c_].y == rayCnk[1] &&
                chunkOffsets[c_].z == rayCnk[2]) {
                chnk = c_;
                break;
            }
        }

        if (chnk != -1) {
            int lx = ix - rayCnk[0] * CHUNK_SIZEX;
            int ly = iy - rayCnk[1] * CHUNK_SIZEY;
            int lz = iz - rayCnk[2] * CHUNK_SIZEZ;

            if (lx >= 0 && lx < CHUNK_SIZEX &&
                ly >= 0 && ly < CHUNK_SIZEY &&
                lz >= 0 && lz < CHUNK_SIZEZ) {
                
                int idx = (lz * CHUNK_SIZEY * CHUNK_SIZEX) + (ly * CHUNK_SIZEX) + lx;
                if (blocks[chnk][idx] >= 1) {
                    rayPosition[0][0] = lastRay[0];
                    rayPosition[0][1] = lastRay[1];
                    rayPosition[0][2] = lastRay[2];
                    rayPosition[0][3] = lastRay[3];

                    rayPosition[1][0] = lx;
                    rayPosition[1][1] = ly;
                    rayPosition[1][2] = lz;
                    rayPosition[1][3] = chnk;

                    placeable = 1;
                    break;
                }

                lastRay[0] = lx;
                lastRay[1] = ly;
                lastRay[2] = lz;
                lastRay[3] = chnk;
            }
        }

        rayBlock[0] = ix;
        rayBlock[1] = iy;
        rayBlock[2] = iz;

        if (tMaxX < tMaxY && tMaxX < tMaxZ) {
            tMaxX += tDeltaX;
            ix += stepX;
        } else if (tMaxY < tMaxZ) {
            tMaxY += tDeltaY;
            iy += stepY;
        } else {
            tMaxZ += tDeltaZ;
            iz += stepZ;
        }
    }
}

int insideBlock(Camera* c, int rx, int ry, int rz) {
    float halfSize = 0.4f;
    if (rx >= (int)floorf(c->position.x - halfSize) && rx <= (int)floorf(c->position.x + halfSize) &&
        ry >= (int)floorf(c->position.y) && ry <= (int)floorf(c->position.y + 1) &&
        rz >= (int)floorf(c->position.z - halfSize) && rz <= (int)floorf(c->position.z + halfSize)) {
        return 1;
    }
    return 0;
}

void placingBlocks(Camera* c){
    int idx0 = (rayPosition[0][2] * CHUNK_SIZEY * CHUNK_SIZEX) + (rayPosition[0][1] * CHUNK_SIZEX) + rayPosition[0][0];
    int idx1 = (rayPosition[1][2] * CHUNK_SIZEY * CHUNK_SIZEX) + (rayPosition[1][1] * CHUNK_SIZEX) + rayPosition[1][0];

    if (placeable){
        if (mineTimer != 0 && mineTimer < 20 && !(btn.input & PB_B)){
            if (idx0 >= 0 && idx0 < (CHUNK_SIZEX * CHUNK_SIZEY * CHUNK_SIZEZ)) {
                int chnk = rayPosition[0][3];
                if (!blocks[chnk][idx0] && !insideBlock(c, rayBlock[0], rayBlock[1]+1, rayBlock[2])){
                    blocks[chnk][idx0] = 1;
                    chunkDirty[chnk] = 1;
                }
            }
        } else if (mineTimer >= 20) {
            int chnk = rayPosition[1][3];
            if (blocks[chnk][idx1]) {
                blocks[chnk][idx1] = 0;
                chunkDirty[chnk] = 1;
            }

            mineTimer = 0;
            lockMine = 1;
        }
    }

    if (btn.input & PB_B) {
        if (!lockMine){
            mineTimer += 1;
        }
    } else {
        mineTimer = 0;
        lockMine = 0;
    }
}