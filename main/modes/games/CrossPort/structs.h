#ifndef LIST_MAKER_H
#define LIST_MAKER_H
#include <stdint.h>

typedef struct {
    int x, y, z;
} Vect3i;

typedef struct {
    float x, y, z;
} Vect3f;

typedef struct {
    int face[6];
} _2DArray;

typedef struct {
    Vect3f worldposition;
    float dist;
    int face[6];
    int objType;
    int facingID;
    Vect3f facingDir;
    Vect3f size;
} RenderBlock;

typedef struct {
    Vect3f position, rotation;
    float fov, nearPlane, farPlane;
} Camera;

typedef struct {
    int x, y, z;
} chunkpos;

typedef struct {
    Vect3f position;
    Vect3f rotation;
    Vect3f size;
    float dist;
    int model, rendered;
} Entity;

typedef struct {
    uint16_t input;
} buttonState;

#endif