#ifndef CAM_H
#define CAM_H
#include "library.h"

Camera createCamera(float x, float y, float z, float rotX, float rotY, float rotZ, float fov, float near, float far);

void moveCamera(Camera* cam, float dx, float dy, float dz);
void rotateCamera(Camera* cam, float rx, float ry, float rz);
void destroyCamera(Camera* cam);

#endif