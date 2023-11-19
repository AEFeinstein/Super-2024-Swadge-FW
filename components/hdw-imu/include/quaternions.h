#ifndef _QUATERNIONS_H_
#define _QUATERNIONS_H_

#include <float.h>
#include <stddef.h>
#include <stdint.h>

float rsqrtf(float x);
float mathsqrtf(float x);
void mathEulerToQuat(float* q, const float* euler);
void mathQuatApply(float* qout, const float* q1, const float* q2);
void mathQuatNormalize(float* qout, const float* qin);
void mathCrossProduct(float* p, const float* a, const float* b);
void mathRotateVectorByInverseOfQuaternion(float* pout, const float* q, const float* p);
void mathRotateVectorByQuaternion(float* pout, const float* q, const float* p);
void mathComputeQuaternionDeltaBetweenQuaternions(float * qOut, const float * q1, const float * b);

#endif
