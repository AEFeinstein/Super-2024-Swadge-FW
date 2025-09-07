#ifndef MESH_H
#define MESH_H

typedef struct { float x, y, z; } Vect3v;
typedef struct { int p[3]; } Triangle;
typedef struct { Triangle tris[2]; } Quad;

typedef struct {
    Vect3v* verts;
    Quad* faces;
    int count;
} Mesh;

#endif