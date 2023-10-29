#ifndef _MATRIX_MATH_H_
#define _MATRIX_MATH_H_

void identityMatrix(float out[4][4]);
void zeroMatrix(float out[4][4]);
void scaleMatrix(float mat[4][4], const float scale[3]);
void unscaleMatrix(float mat[4][4], const float scale[3]);
void rotateMatrix(float mat[4][4], const float q[4]);
void unrotateMatrix(float mat[4][4], const float quat[4]);
void translateMatrix(float mat[4][4], const float translate[3]);
void untranslateMatrix(float mat[4][4], const float translate[3]);
void multiplyMatrixVector(float out[4], const float mat[4][4], const float vec[4]);
void multiplyMatrixMatrix(float out[4][4], const float a[4][4], const float b[4][4]);
void invertMatrix(float m[4][4]);
void invertMatrixTo(float out[4][4], const float in[4][4]);

void createTransformMatrix(float transform[4][4], const float translate[3], const float rotate[4], const float scale[3]);

void createViewMatrix(float transform[4][4], const float translate[3], const float rotate[4], const float scale[3]);
void createPerspectiveMatrix(float mat[4][4], float fovy, float aspect, float zNear, float zFar);
void lookAtMatrix(float mat[4][4], const float eye[3], const float lookAt[3], const float up[3]);

#endif
