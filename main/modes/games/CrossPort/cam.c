#include "cam.h"

Camera createCamera(float x, float y, float z, float rotX, float rotY, float rotZ, float fov, float near, float far) {
    Camera cam;
    cam.position.x = x;
    cam.position.y = y;
    cam.position.z = z;
    cam.rotation.x = rotX;
    cam.rotation.y = rotY;
    cam.rotation.z = rotZ;
    cam.fov = fov;
    cam.nearPlane = near;
    cam.farPlane = far;
    return cam;
}

void moveCamera(Camera* cam, float dx, float dy, float dz) { if (cam) { cam->position.x += dx; cam->position.y += dy; cam->position.z += dz; } }
void rotateCamera(Camera* cam, float rx, float ry, float rz) { if (cam) { cam->rotation.x += rx; cam->rotation.y += ry; cam->rotation.z += rz; } }
void destroyCamera(Camera* cam) { if (cam) { heap_caps_free(cam); } }