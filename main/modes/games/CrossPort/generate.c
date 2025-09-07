#include "generate.h"

chunkpos chunksAlter = {MAX_DISTX*2 + 1, MAX_DISTY*2 + 1, MAX_DISTZ*2 + 1};

int blockOffset[6][3] = {
    {0, 0, -1}, {0, 0, 1},
    {-1, 0, 0}, {1, 0, 0},
    {0, 1, 0}, {0, -1, 0}
};

void checkFaces(int x, int y, int z, int c, int faces[6]) {
    int idx = (z * CHUNK_SIZEY * CHUNK_SIZEX) + (y * CHUNK_SIZEX) + x;

    for (int faceDir = 0; faceDir < 6; faceDir++) {
        faces[faceDir] = 0;

        int nx = x + blockOffset[faceDir][0];
        int ny = y + blockOffset[faceDir][1];
        int nz = z + blockOffset[faceDir][2];
        int wrapped = 0;
        int newC = c;

        // Handle wrapping into neighbor chunk
        if (blockOffset[faceDir][0] != 0 && CHUNK_DISTX > 0) {
            if (nx >= CHUNK_SIZEX) { nx = 0; wrapped = 1; }
            else if (nx < 0)       { nx = CHUNK_SIZEX - 1; wrapped = 1; }
        } 
        else if (blockOffset[faceDir][1] != 0 && CHUNK_DISTY > 0) {
            if (ny >= CHUNK_SIZEY) { ny = 0; wrapped = 1; }
            else if (ny < 0)       { ny = CHUNK_SIZEY - 1; wrapped = 1; }
        } 
        else if (blockOffset[faceDir][2] != 0 && CHUNK_DISTZ > 0) {
            if (nz >= CHUNK_SIZEZ) { nz = 0; wrapped = 1; }
            else if (nz < 0)       { nz = CHUNK_SIZEZ - 1; wrapped = 1; }
        }
        
        if (wrapped) {
            int cx = chunkOffsets[c].x + blockOffset[faceDir][0];
            int cy = chunkOffsets[c].y + blockOffset[faceDir][1];
            int cz = chunkOffsets[c].z + blockOffset[faceDir][2];

            if ((blockOffset[faceDir][0] != 0 && MAX_DISTX > 0 && (cx >  MAX_DISTX || cx < -MAX_DISTX)) ||
                (blockOffset[faceDir][1] != 0 && MAX_DISTY > 0 && (cy >  MAX_DISTY || cy < 0)) ||
                (blockOffset[faceDir][2] != 0 && MAX_DISTZ > 0 && (cz >  MAX_DISTZ || cz < -MAX_DISTZ))) {
                faces[faceDir] = 1;
                continue;
            }
    
            for (int ci = 0; ci < CHUNK_AMT; ci++) {
                if (chunkOffsets[ci].x == cx &&
                    chunkOffsets[ci].y == cy &&
                    chunkOffsets[ci].z == cz) {
                    newC = ci;
                    break;
                }
            }
            
            if (newC >= CHUNK_AMT || newC < 0) {
                faces[faceDir] = 1;
                continue;
            }
        }
        
        if (nx < 0 || ny < 0 || nz < 0 || nx >= CHUNK_SIZEX || ny >= CHUNK_SIZEY || nz >= CHUNK_SIZEZ) {
            faces[faceDir] = 1;
            continue;
        }

        int idx_n = (nz * CHUNK_SIZEY * CHUNK_SIZEX) + (ny * CHUNK_SIZEX) + nx;
        if (blocks[newC][idx_n] == 0) { faces[faceDir] = 1; }
    }
}

int getChunkIndex(int cx, int cy, int cz) {
    return cx + chunksAlter.x * (cz + chunksAlter.z * cy);
}

// flatland, hills, mountains, extreme
float terrainModifiers[] = { 0.1f, 0.2f, 0.4f };

void generateWorld(int chunkX, int chunkY, int chunkZ, int c, int seed){
    int chunkSeed = (chunkX * 73856093) ^ (chunkY * 19349663) ^ (chunkZ * 83492791);
    initPerlinNoise((int)seed ^ chunkSeed);

    float amplitude = 8.0f;
    float frequency = 0.3f;

    if (MAX_DISTX>0 && chunkX > MAX_DISTX) { return; }
    else if (MAX_DISTX>0 && chunkX < -MAX_DISTX) { return; }
    if (MAX_DISTY>0 && chunkY > MAX_DISTY) { return; }
    else if (MAX_DISTY>0 && chunkY < 0) { return; }
    if (MAX_DISTZ>0 && chunkZ > MAX_DISTZ) { return; }
    else if (MAX_DISTZ>0 && chunkZ < -MAX_DISTZ) { return; }

    int MIN_TERRAIN_Y = 2;
    int MAX_TERRAIN_Y = CHUNK_SIZEY * MAX_DISTY - 2;

    float n1 = perlinNoise2D(chunkX * 0.1f, chunkZ * 0.1f);
    float n2 = perlinNoise2D(chunkX * 0.3f, chunkZ * 0.3f) * 0.5f;
    float n3 = perlinNoise2D(chunkX * 0.6f, chunkZ * 0.6f) * 0.25f;

    float combined = n1 + n2 + n3;
    int terrainType = (int)((combined + 1.75f) / 3.5f * 2);
    if (terrainType < 0) terrainType = 0;
    if (terrainType > 2) terrainType = 2;

    for (int y = 0; y < CHUNK_SIZEY; y++) {
        for (int x = 0; x < CHUNK_SIZEX; x++) {
            for (int z = 0; z < CHUNK_SIZEZ; z++) {
    
                int worldX = chunkX * CHUNK_SIZEX + x;
                int worldY = chunkY * CHUNK_SIZEY + y;
                int worldZ = chunkZ * CHUNK_SIZEZ + z;
    
                float noise = perlinNoise3D(
                    worldX * frequency,
                    worldY * frequency * terrainModifiers[terrainType],
                    worldZ * frequency
                );
                noise = (noise + 1.0f) * 0.5f;

                float scaledNoise = (noise + 1.0f) * 0.5f;
                int range = MAX_TERRAIN_Y - MIN_TERRAIN_Y;
                int maxHeight = MIN_TERRAIN_Y + (int)(scaledNoise * range);
    
                int idx = (z * CHUNK_SIZEX * CHUNK_SIZEY) + (y * CHUNK_SIZEX) + x;
                int idx_y = (y > 0) ? idx - CHUNK_SIZEX : -1;
                int idx_yc = (z * CHUNK_SIZEX * CHUNK_SIZEY) + ((CHUNK_SIZEY-1) * CHUNK_SIZEX) + x;
    
                blocks[c][idx] = (worldY <= maxHeight) ? 1 : 0;

                if (y > 0) {
                    if ((blocks[c][idx_y] == 1 && blocks[c][idx] == 1) || (blocks[c][idx_y] == 2 && blocks[c][idx] == 2)) {
                        blocks[c][idx_y] = 2;
                    }
                } else if (y == 0 && chunkY > 0) {
                    int lowerCHNK = getChunkIndex(chunkX, chunkY - 1, chunkZ);
                    if (lowerCHNK >= 0 && lowerCHNK < CHUNK_AMT && idx_yc >= 0) {
                        if ((blocks[lowerCHNK][idx_yc] == 1 && blocks[c][idx] == 1) ||
                            (blocks[lowerCHNK][idx_yc] == 2 && blocks[c][idx] == 2)) {
                            blocks[lowerCHNK][idx_yc] = 2;
                        }
                    }
                }
            }
        }
    }

    // generateTrees(c, chunkX, chunkZ);
}

void cleanWorld(){
    int total = (CHUNK_SIZEX * CHUNK_SIZEY * CHUNK_SIZEZ);
    for (int c=0; c < CHUNK_AMT; c++){
        for (int i=0; i < total; i++){
            blocks[c][i] = randomInt(0, 1);
        }
    }
}