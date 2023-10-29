#include "matrixMath.h"

#include <float.h>
#include <esp_log.h>
#include <string.h>
#include <math.h>

#include "quaternions.h"

/**
 * @brief Overwrites the target with the identity matrix
 *
 * @param out The matrix to set to the identity matrix
 */
void identityMatrix(float out[4][4])
{
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            out[i][j] = (i == j) ? 1.0 : 0.0;
        }
    }
}

/**
 * @brief Set the target matrix to all 0s
 *
 * @param out The matrix to zero out
 */
void zeroMatrix(float out[4][4])
{
    memset(out, 0, sizeof(float[4][4]));
}

void scaleMatrix(float mat[4][4], const float scale[3])
{
    mat[0][0] *= scale[0];
    mat[1][1] *= scale[1];
    mat[2][2] *= scale[2];
}

void unscaleMatrix(float mat[4][4], const float scale[3])
{
    mat[0][0] /= scale[0];
    mat[1][1] /= scale[1];
    mat[2][2] /= scale[2];
}

/*
// TODO ideally we would use this instead of euler angles below, but this is broken?
void rotateMatrix(float mat[4][4], const float q[4])
{
    float tmp[4][4];

    float qx2 = q[0] * q[0];
    float qy2 = q[1] * q[1];
    float qz2 = q[2] * q[2];

    tmp[0][0] = 1 - 2 * qy2 - 2 * qz2;
    tmp[0][1] = 2 * q[0] * q[1] - 2 * q[2] * q[3];
    tmp[0][2] = 2*q[0] * q[2] + 2 * q[1] * q[3];
    tmp[0][3] = 0;

    tmp[1][0] = 2 * q[0] * q[1] + 2 * q[2] * q[3];
    tmp[1][1] = 1 - 2 * qx2 - 2 * qz2;
    tmp[1][2] = 2 * q[1] * q[2] - 2 * q[0] * q[3];
    tmp[1][3] = 0;

    tmp[2][0] = 2 * q[0] * q[2] - 2 * q[1] * q[3];
    tmp[2][1] = 2 * q[1] * q[2] + 2 * q[0] * q[3];
    tmp[2][2] = 1 - 2 * qx2 - 2 * qy2;
    tmp[2][3] = 0;

    tmp[3][0] = 0;
    tmp[3][1] = 0;
    tmp[3][2] = 0;
    tmp[3][3] = 1;

    multiplyMatrixMatrix(mat, tmp, mat);
}*/

void rotateMatrix(float mat[4][4], const float quat[4])
{
    mathRotateVectorByQuaternion(mat[1], quat, mat[1]);
    mathRotateVectorByQuaternion(mat[0], quat, mat[0]);
    mathRotateVectorByQuaternion(mat[2], quat, mat[2]);
}

void unrotateMatrix(float mat[4][4], const float quat[4])
{
    mathRotateVectorByInverseOfQuaternion(mat[2], quat, mat[2]);
    mathRotateVectorByInverseOfQuaternion(mat[0], quat, mat[0]);
    mathRotateVectorByInverseOfQuaternion(mat[1], quat, mat[1]);
}

void translateMatrix(float mat[4][4], const float translate[3])
{
    mat[0][3] += translate[0];
    mat[1][3] += translate[1];
    mat[2][3] += translate[2];
}

void untranslateMatrix(float mat[4][4], const float translate[3])
{
    mat[0][3] -= translate[0];
    mat[1][3] -= translate[1];
    mat[2][3] -= translate[2];
}

void multiplyMatrixVector(float out[4], const float mat[4][4], const float vec[4])
{
    float tmp[3];
    tmp[0] = vec[0] * mat[0][0] + vec[1] * mat[0][1] + vec[2] * mat[0][2] + vec[3] * mat[0][3];
    tmp[1] = vec[0] * mat[1][0] + vec[1] * mat[1][1] + vec[2] * mat[1][2] + vec[3] * mat[1][3];
    tmp[2] = vec[0] * mat[2][0] + vec[1] * mat[2][1] + vec[2] * mat[2][2] + vec[3] * mat[2][3];
    out[3] = vec[0] * mat[3][0] + vec[1] * mat[3][1] + vec[2] * mat[3][2] + vec[3] * mat[3][3];
    out[2] = tmp[2];
    out[1] = tmp[1];
    out[0] = tmp[0];
}

void multiplyMatrixMatrix(float out[4][4], const float a[4][4], const float b[4][4])
{
    float tmp[4][4];
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            tmp[i][j] = a[i][0] * b[j][0] + a[i][1] * b[j][1] + a[i][2] * b[j][2] + a[i][3] * b[j][3];
        }
    }

    memcpy(out, tmp, sizeof(float[4][4]));
}

/**
 * @brief Invert a 4x4 matrix in place
 *
 * @param m The matrix to invert
 */
void invertMatrix(float m[4][4])
{
    float tmp;
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < i; j++)
        {
            tmp = m[i][j];
            m[i][j] = m[j][i];
            m[j][i] = tmp;
        }
    }
}

/**
 * @brief Invert a matrix and write to another matrix
 *
 * @param out The matrix to write the inverse to
 * @param in The matrix to be inverted
 */
void invertMatrixTo(float out[4][4], const float in[4][4])
{
    float tmp[4][4];
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            tmp[i][j] = in[j][i];
        }
    }
    memcpy(out, tmp, sizeof(float[4][4]));
}

/**
 * @brief Set up a transform matrix with the given parameters
 *
 * @param[out] transform The output 4x4 transform matrix
 * @param translate The x, y, and z translation to apply
 * @param rotate The orientation quaternion to apply
 * @param scale The scale transformation to apply
 *
 */
void createTransformMatrix(float transform[4][4], const float translate[3], const float rotate[4], const float scale[3])
{
    ESP_LOGI("Model", "createTransformMatrix(translate=[%+03.2f, %+03.2f, %+03.2f], orient=[%+03.2f %+03.2f %+03.2f %+03.2f], scale=[%+03.2f %+03.2f %+03.2f])",
             translate[0], translate[1], translate[2], rotate[0], rotate[1], rotate[2], rotate[3], scale[0], scale[1], scale[2]);
    // Get rotation matrix first
    identityMatrix(transform);

    // Initialize it with the identity matrix, scaled
    scaleMatrix(transform, scale);

    // Add translate (will be unaffected by rotation)
    translateMatrix(transform, translate);

    // w = 1 for a point, 0 for a vector
    // so this makes sense
    // rotate in yxz order because that's how the other code does it
    // Rotate each row by the quaternion, I'm pretty sure this will apply the rotation
    rotateMatrix(transform, rotate);
}


/**
 * @brief Set up a transform matrix with the given parameters
 *
 * @param[out] transform The output 4x4 transform matrix
 * @param translate The x, y, and z translation to apply
 * @param rotate The orientation quaternion to apply
 * @param scale The scale transformation to apply
 *
 */
void createViewMatrix(float transform[4][4], const float translate[3], const float rotate[4], const float scale[3])
{
    // TODO this is not how a view matrix works
    ESP_LOGI("Model", "createViewMatrix(translate=[%+03.2f, %+03.2f, %+03.2f], orient=[%+03.2f %+03.2f %+03.2f %+03.2f], scale=[%+03.2f %+03.2f %+03.2f])",
             translate[0], translate[1], translate[2], rotate[0], rotate[1], rotate[2], rotate[3], scale[0], scale[1], scale[2]);
    // Get rotation matrix first
    identityMatrix(transform);

    // Initialize it with the identity matrix, scaled
    scaleMatrix(transform, scale);

    // Add translate (will be unaffected by rotation)
    translateMatrix(transform, translate);

    // w = 1 for a point, 0 for a vector
    // so this makes sense
    // rotate in reverse-yxz order because that's how the other code did it
    // Rotate each row by the quaternion, I'm pretty sure this will apply the rotation
    unrotateMatrix(transform, rotate);
}

void createPerspectiveMatrix(float mat[4][4], float fovy, float aspect, float zNear, float zFar)
{
    // TODO is there a more efficient tanf? Does it matter? Probably not, this will happen like once.
	float f = 1.0 / tanf(fovy * 3.141592653589 / 360.0);
	mat[0][0] = f / aspect;
    mat[0][1] = 0;
    mat[0][2] = 0;
    mat[0][3] = 0;

	mat[1][0] = 0;
    mat[1][1] = f;
    mat[1][2] = 0;
    mat[1][3] = 0;

	mat[2][0] = 0;
    mat[2][1] = 0;
	mat[2][2] = (zFar + zNear) / (zNear - zFar);
	mat[2][3] = 2 * zFar * zNear  / (zNear - zFar);

	mat[3][0] = 0;
    mat[3][1] = 0;
    mat[3][2] = -1;
    mat[3][3] = 0;
}

/**
 * @brief Transform the given view matrix so that it points from \c eye towards \c lookAt, with up
 * in the direction of the \c up vector
 *
 * @param[in,out] mat The existing view matrix to transform
 * @param[in] eye The location of the camera's eye
 * @param[in] lookAt A point toward which the eye is pointing
 * @param[in] up A vector representing the direction from the bottom to the top of the screen
 */
void lookAtMatrix(float mat[4][4], const float eye[3], const float lookAt[3], const float up[3])
{
    float tmp[4][4];

    // Normalize the forward / "at" vector
    float fwd[3] = {
        lookAt[0] - eye[0],
        lookAt[1] - eye[1],
        lookAt[2] - eye[2]
    };
    const float fwdDiv = rsqrtf(fwd[0] * fwd[0] + fwd[1] * fwd[1] + fwd[2] * fwd[2]);

    fwd[0] *= fwdDiv;
    fwd[1] *= fwdDiv;
    fwd[2] *= fwdDiv;

    // Normalize the "up" vector
    float upDiv = rsqrtf(up[0] * up[0] + up[1] * up[1] + up[2] * up[2]);
    float upNorm[3] = { up[0] * upDiv, up[1] * upDiv, up[2] * upDiv };

    // TODO not sure if these are the correct names
    float skew[3];
    float upOut[3];

    // Multiply the matrices and construct the look matrix with the result
    mathCrossProduct(skew, fwd, upNorm);
    mathCrossProduct(upOut, skew, fwd);

    tmp[0][0] = skew[0];
    tmp[0][1] = skew[1];
    tmp[0][2] = skew[2];
    tmp[0][3] = 0;

    tmp[1][0] = up[0];
    tmp[1][1] = up[1];
    tmp[1][2] = up[2];
    tmp[1][3] = 0;

    tmp[2][0] = -fwd[0];
    tmp[2][1] = -fwd[1];
    tmp[2][2] = -fwd[2];
    tmp[2][3] = 0;

    tmp[3][0] = 0;
    tmp[3][1] = 0;
    tmp[3][2] = 0;
    tmp[3][3] = 1;

    // Transform the given matrix by the result
    multiplyMatrixMatrix(mat, tmp, mat);

    // Translate the matrix by the inverse of the eye vector
    untranslateMatrix(mat, eye);
}
