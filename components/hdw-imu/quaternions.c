#include "quaternions.h"

#include <math.h>

/**
 * @brief Perform a fast, approximate reciprocal square root
 *
 * @param x The number to take a recriprocal square root of.
 * @return approximately 1/sqrt(x)
 */
float rsqrtf(float x)
{
    typedef union
    {
        int32_t i;
        float f;
    } fiunion;
    const float xhalf = 0.5f * x;
    fiunion i         = {.f = x};
    i.i               = 0x5f375a86 - (i.i >> 1);
    x                 = i.f;
    x                 = x * (1.5f - xhalf * x * x);
    x                 = x * (1.5f - xhalf * x * x);
    return x;
}

/**
 * @brief Perform a fast, approximate square root
 *
 * @param x The number to take a square root of.
 * @return approximately sqrt(x) (but is much faster)
 */
float mathsqrtf(float x)
{
    // Trick to do approximate, fast square roots. (Though it is surprisingly fast)
    int sign = x < 0;
    if (sign)
        x = -x;
    if (x < 0.0000001)
        return 0.0001;
    float o = x;
    o       = (o + x / o) / 2;
    o       = (o + x / o) / 2;
    o       = (o + x / o) / 2;
    o       = (o + x / o) / 2;
    if (sign)
        return -o;
    else
        return o;
}

/**
 * @brief convert euler angles (in radians) to a quaternion.
 *
 * @param q Pointer to the wxyz quat (float[4]) to be written.
 * @param euler Pointer to a float[3] of euler angles.
 */
void mathEulerToQuat(float* q, const float* euler)
{
    float pitch = euler[0];
    float yaw   = euler[1];
    float roll  = euler[2];
    float cr    = cosf(pitch * 0.5);
    float sr    = sinf(pitch * 0.5); // Pitch: About X
    float cp    = cosf(yaw * 0.5);
    float sp    = sinf(yaw * 0.5); // Yaw:   About Y
    float cy    = cosf(roll * 0.5);
    float sy    = sinf(roll * 0.5); // Roll:  About Z
    q[0]        = cr * cp * cy + sr * sp * sy;
    q[1]        = sr * cp * cy - cr * sp * sy;
    q[2]        = cr * sp * cy + sr * cp * sy;
    q[3]        = cr * cp * sy - sr * sp * cy;
}

/**
 * @brief Rotate one quaternion by another (and do not normalize)
 *
 * @param qout Pointer to the wxyz quat (float[4]) to be written.
 * @param q1 First quaternion to be rotated.
 * @param q2 Quaternion to rotate q1 by.
 */
void mathQuatApply(float* qout, const float* q1, const float* q2)
{
    // NOTE: Does not normalize - you will need to normalize eventually.
    float tmpw, tmpx, tmpy;
    tmpw    = (q1[0] * q2[0]) - (q1[1] * q2[1]) - (q1[2] * q2[2]) - (q1[3] * q2[3]);
    tmpx    = (q1[0] * q2[1]) + (q1[1] * q2[0]) + (q1[2] * q2[3]) - (q1[3] * q2[2]);
    tmpy    = (q1[0] * q2[2]) - (q1[1] * q2[3]) + (q1[2] * q2[0]) + (q1[3] * q2[1]);
    qout[3] = (q1[0] * q2[3]) + (q1[1] * q2[2]) - (q1[2] * q2[1]) + (q1[3] * q2[0]);
    qout[2] = tmpy;
    qout[1] = tmpx;
    qout[0] = tmpw;
}

/**
 * @brief Normalize a quaternion
 *
 * @param qout Pointer to the wxyz quat (float[4]) to be written.
 * @param qin Pointer to the quaterion to normalize.
 */
void mathQuatNormalize(float* qout, const float* qin)
{
    float qmag = qin[0] * qin[0] + qin[1] * qin[1] + qin[2] * qin[2] + qin[3] * qin[3];
    qmag       = rsqrtf(qmag);
    qout[0]    = qin[0] * qmag;
    qout[1]    = qin[1] * qmag;
    qout[2]    = qin[2] * qmag;
    qout[3]    = qin[3] * qmag;
}

/**
 * @brief Perform a 3D cross product
 *
 * @param p Pointer to the float[3] output of the cross product (p = a x b)
 * @param a Pointer to the float[3] of the cross product a vector.
 * @param b Pointer to the float[3] of the cross product b vector.
 */
void mathCrossProduct(float* p, const float* a, const float* b)
{
    float tx = a[1] * b[2] - a[2] * b[1];
    float ty = a[2] * b[0] - a[0] * b[2];
    p[2]     = a[0] * b[1] - a[1] * b[0];
    p[1]     = ty;
    p[0]     = tx;
}

/**
 * @brief Rotate a 3D vector by a quaternion
 *
 * @param pout Pointer to the float[3] output of the rotation
 * @param q Pointer to the wxyz quaternion (float[4]) of the rotation.
 * @param p Pointer to the float[3] of the vector to rotates.
 */
void mathRotateVectorByQuaternion(float* pout, const float* q, const float* p)
{
    // return v + 2.0 * cross(q.xyz, cross(q.xyz, v) + q.w * v);
    float iqo[3];
    mathCrossProduct(iqo, q + 1 /*.xyz*/, p);
    iqo[0] += q[0] * p[0];
    iqo[1] += q[0] * p[1];
    iqo[2] += q[0] * p[2];
    float ret[3];
    mathCrossProduct(ret, q + 1 /*.xyz*/, iqo);
    pout[0] = ret[0] * 2.0 + p[0];
    pout[1] = ret[1] * 2.0 + p[1];
    pout[2] = ret[2] * 2.0 + p[2];
}

/**
 * @brief Rotate a 3D vector by the inverse of a quaternion
 *
 * @param pout Pointer to the float[3] output of the antirotation.
 * @param q Pointer to the wxyz quaternion (float[4]) opposite of the rotation.
 * @param p Pointer to the float[3] of the vector to antirotates.
 */
void mathRotateVectorByInverseOfQuaternion(float* pout, const float* q, const float* p)
{
    // General note: Performing a transform this way can be about 20-30% slower than a well formed 3x3 matrix.
    // return v + 2.0 * cross(q.xyz, cross(q.xyz, v) + q.w * v);
    float iqo[3];
    mathCrossProduct(iqo, p, q + 1 /*.xyz*/);
    iqo[0] += q[0] * p[0];
    iqo[1] += q[0] * p[1];
    iqo[2] += q[0] * p[2];
    float ret[3];
    mathCrossProduct(ret, iqo, q + 1 /*.xyz*/);
    pout[0] = ret[0] * 2.0 + p[0];
    pout[1] = ret[1] * 2.0 + p[1];
    pout[2] = ret[2] * 2.0 + p[2];
}

/**
 * @brief Compute the difference between two quaternions
 *
 * @param qOut Pointer to the float[4] (wxyz) output of the rotation between qFrom and qTo.
 * @param qFrom is the Quaterntion that you are rotating FROM
 * @param qTo is the Quaternion that you are rotating TO
 */
void mathComputeQuaternionDeltaBetweenQuaternions(float* qOut, const float* qFrom, const float* qTo)
{
    // Diff= quatmultiply(quatconj(x),y)
    float qtmp[3];
    qtmp[0] = qFrom[0] * qTo[0] + qFrom[1] * qTo[1] + qFrom[2] * qTo[2] + qFrom[3] * qTo[3];
    qtmp[1] = qFrom[0] * qTo[1] - qFrom[1] * qTo[0] - qFrom[2] * qTo[3] + qFrom[3] * qTo[2];
    qtmp[2] = qFrom[0] * qTo[2] + qFrom[1] * qTo[3] - qFrom[2] * qTo[0] - qFrom[3] * qTo[1];
    qOut[3] = qFrom[0] * qTo[3] - qFrom[1] * qTo[2] + qFrom[2] * qTo[1] - qFrom[3] * qTo[0];
    qOut[0] = qtmp[0];
    qOut[1] = qtmp[1];
    qOut[2] = qtmp[2];
}

/**
 * @brief Compute the quaterntion rotation between two vectors, from v1 to v2
 *
 * @param qOut Pointer to the float[4] (wxyz) output of the rotation defined by v1 to v2
 * @param v1 is the vector you are rotating FROM. THIS MUST BE NORMALIZED.
 * @param v2 is the vector you are rotating TO. THIS MUST BE NORMALIZED.
 */
void mathQuatFromTwoVectors(float* qOut, const float* v1, const float* v2)
{
    float ideal_up[3]  = {v1[0], v1[1], v1[2]};
    float target_up[3] = {v2[0], v2[1], v2[2]};
    float half[3]      = {target_up[0] + ideal_up[0], target_up[1] + ideal_up[1], target_up[2] + ideal_up[2]};
    float halfnormreq  = rsqrtf(half[0] * half[0] + half[1] * half[1] + half[2] * half[2]);
    half[0] *= halfnormreq;
    half[1] *= halfnormreq;
    half[2] *= halfnormreq;

    mathCrossProduct(qOut + 1, target_up, half);
    float dotdiff = target_up[0] * half[0] + target_up[1] * half[1] + target_up[2] * half[2];
    qOut[0]       = dotdiff;
}
