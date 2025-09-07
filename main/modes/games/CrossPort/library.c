#include "library.h"

#define PERMUTATION_SIZE 256
static int perm[PERMUTATION_SIZE * 2];

int sX = 0;
int sY = 0;
int sW = 280;
int sH = 240;
int* blocks[CHUNK_AMT];
chunkpos chunkOffsets[CHUNK_AMT];
Vect3i tilePosMove[36];
buttonState btn;

const int uv[2][3][2] = {
    {{0, 1}, {1, 0},  {0, 0}},
    {{0, 1}, {1, 1}, {1, 0}}
};

void setupTilePos(){
    int idx = 0;
    for (int x = -1; x <= 1; x++) {
        for (int y = -2; y <= 1; y++) {
            for (int z = -1; z <= 1; z++) {
                tilePosMove[idx++] = (Vect3i){.x = x, .y = y, .z = z};
            }
        }
    }
}

void swapVerts(int a[2], int b[2]) {
    int tmp[2] = { a[0], a[1] };
    a[0] = b[0]; a[1] = b[1];
    b[0] = tmp[0]; b[1] = tmp[1];
}

int randomInt(int a, int b) { return a + rand() % (b - a + 1); }
float randomFloat(float a, float b) { return (float)(a + rand() % ((int)b - (int)a + 1)); }
float degToRad(float deg) { return deg * (M_PI / 180.0f); }
float radToDeg(float rad) { return rad * (180.0f / M_PI); }
float fade(float t) { return t * t * t * (t * (t * 6 - 15) + 10); }
float lerpF(float t, float a, float b) { return a + t * (b - a); }
float dot(Vect3f a, Vect3f b) { return a.x*b.x + a.y*b.y + a.z*b.z; }

static float grad(int hash, float x, float y, float z) {
    int h = hash & 15;
    float u = h < 8 ? x : y;
    float v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
    return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
}
void initPerlinNoise(unsigned int seed) {
    srand(seed);
    for (int i = 0; i < PERMUTATION_SIZE; i++) {
        perm[i] = i;
    }

    for (int i = 0; i < PERMUTATION_SIZE; i++) {
        int j = rand() % PERMUTATION_SIZE;
        int tmp = perm[i];
        perm[i] = perm[j];
        perm[j] = tmp;
    }

    for (int i = 0; i < PERMUTATION_SIZE; i++) {
        perm[PERMUTATION_SIZE + i] = perm[i];
    }
}
float perlinNoise2D(float x, float y) {
    int X = (int)floorf(x) & 255;
    int Y = (int)floorf(y) & 255;

    x -= floorf(x);
    y -= floorf(y);

    float u = fade(x);
    float v = fade(y);

    int A  = perm[X] + Y;
    int B  = perm[X + 1] + Y;
    
    float res = lerpF(v,
                     lerpF(u, grad(perm[A], x, y, 0),
                             grad(perm[B], x - 1, y, 0)),
                     lerpF(u, grad(perm[A + 1], x, y - 1, 0),
                             grad(perm[B + 1], x - 1, y - 1, 0))
                    );
    return res;
}
float perlinNoise3D(float x, float y, float z) {
    int X = (int)floorf(x) & 255;
    int Y = (int)floorf(y) & 255;
    int Z = (int)floorf(z) & 255;

    x -= floorf(x);
    y -= floorf(y);
    z -= floorf(z);

    float u = fade(x);
    float v = fade(y);
    float w = fade(z);

    int A  = perm[X] + Y;
    int AA = perm[A] + Z;
    int AB = perm[A + 1] + Z;
    int B  = perm[X + 1] + Y;
    int BA = perm[B] + Z;
    int BB = perm[B + 1] + Z;

    return lerpF(w, lerpF(v, lerpF(u, grad(perm[AA], x, y, z),
                                   grad(perm[BA], x - 1, y, z)),
                           lerpF(u, grad(perm[AB], x, y - 1, z),
                                   grad(perm[BB], x - 1, y - 1, z))),
                   lerpF(v, lerpF(u, grad(perm[AA + 1], x, y, z - 1),
                                   grad(perm[BA + 1], x - 1, y, z - 1)),
                           lerpF(u, grad(perm[AB + 1], x, y - 1, z - 1),
                                   grad(perm[BB + 1], x - 1, y - 1, z - 1))));
}

int viewFrustrum3D(float x, float y, float z, float fovX, float fovY, float nearPlane, float farPlane) {
    if (z < nearPlane || z > farPlane) return 1;

    float halfWidth  = z * tanf(fovX * 0.5f);
    float halfHeight = z * tanf(fovY * 0.5f);

    if (x < -halfWidth || x > halfWidth) return 1;
    if (y < -halfHeight || y > halfHeight) return 1;

    return 0;
}