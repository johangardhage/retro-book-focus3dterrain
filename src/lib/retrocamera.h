//
// Retro graphics library
//
// Author: Johan Gardhage <johan.gardhage@gmail.com>
//
#ifndef _RETROCAMERA_H_
#define _RETROCAMERA_H_

#include "retrogl.h"
#include <math.h> // cos, sin, fabs, sqrt, M_PI

typedef float vec3_t[3];

#define CAMERA_TURN_SPEED        2.0
#define CAMERA_PITCH_SPEED       2.0
#define CAMERA_MOUSE_SENSITIVITY 0.5

// A free-flying camera shared by the demos. The orientation math (forward and
// side vectors from yaw/pitch) matches the original demos' CCAMERA. Movement
// accumulates between updates and is applied in UpdatePosition().
struct RETRO_Camera
{
	vec3_t head;    // Position of eye
	vec3_t view;    // Forward (look) vector
	vec3_t up;      // Up vector

	float yaw;              // Heading (degrees)
	float pitch;            // Neck angle (degrees)
	float speed;            // Accumulated speed along heading
	float strafe;           // Accumulated speed sideways
	float movementSpeed;    // Movement increment per key tick

	float frustum[6][4];    // View frustum planes

	RETRO_Camera()
	{
		head[0] = 0.0f;
		head[1] = 0.0f;
		head[2] = 0.0f;
		view[0] = 0.0f;
		view[1] = 0.0f;
		view[2] = -1.0f;
		yaw = 0;
		pitch = 0;
		speed = 0;
		strafe = 0;
		movementSpeed = 1.0f;
		up[0] = 0.0f;
		up[1] = 1.0f;
		up[2] = 0.0f;
	}

	void SetPosition(float x, float y, float z)
	{
		head[0] = x;
		head[1] = y;
		head[2] = z;
	}

	void SetYaw(float degrees)
	{
		yaw = degrees;
	}

	void SetPitch(float degrees)
	{
		pitch = degrees;
	}

	void SetMovementSpeed(float s)
	{
		movementSpeed = s;
	}

	// Update the camera position and view vector
	void UpdatePosition(void)
	{
		float cosYaw = cos(yaw * M_PI / 180.0);
		float sinYaw = sin(yaw * M_PI / 180.0);
		float cosPitch = cos(pitch * M_PI / 180.0);
		float sinPitch = sin(pitch * M_PI / 180.0);

		// Forward (look) vector
		float forward[3] = { sinYaw * cosPitch, sinPitch, -cosYaw * cosPitch };

		// Side vector = forward x up
		float side[3] = {
			forward[1] * up[2] - forward[2] * up[1],
			forward[2] * up[0] - forward[0] * up[2],
			forward[0] * up[1] - forward[1] * up[0]
		};

		// Move along the heading
		if (fabs(speed) > 0) {
			head[0] += forward[0] * speed;
			head[1] += forward[1] * speed;
			head[2] += forward[2] * speed;
		}

		// Move sideways
		if (fabs(strafe) > 0) {
			head[0] += side[0] * strafe;
			head[1] += side[1] * strafe;
			head[2] += side[2] * strafe;
		}

		// Publish the view vector
		view[0] = forward[0];
		view[1] = forward[1];
		view[2] = forward[2];

		// Reset accumulators
		speed = 0;
		strafe = 0;
	}

	void Pitch(float degrees)
	{
		pitch -= degrees * CAMERA_MOUSE_SENSITIVITY;
		if (pitch > 60.0f) {
			pitch = 60.0f;
		} else if (pitch < -60.0f) {
			pitch = -60.0f;
		}
	}

	void Yaw(float degrees)
	{
		yaw += degrees * CAMERA_MOUSE_SENSITIVITY;
		if (yaw < 0.0f) {
			yaw += 360.0f;
		} else if (yaw > 360.0f) {
			yaw -= 360.0f;
		}
	}

	void PitchUp(float scale = 1.0f)
	{
		if (pitch < 60.0f) {
			pitch += CAMERA_PITCH_SPEED * scale;
		}
	}

	void PitchDown(float scale = 1.0f)
	{
		if (pitch > -60.0f) {
			pitch -= CAMERA_PITCH_SPEED * scale;
		}
	}

	void MoveForward(float scale = 1.0f)
	{
		speed += movementSpeed * scale;
	}

	void MoveBackward(float scale = 1.0f)
	{
		speed -= movementSpeed * scale;
	}

	void TurnRight(float scale = 1.0f)
	{
		yaw += CAMERA_TURN_SPEED * scale;
		if (yaw > 360.0f) {
			yaw -= 360.0f;
		}
	}

	void TurnLeft(float scale = 1.0f)
	{
		yaw -= CAMERA_TURN_SPEED * scale;
		if (yaw < 0.0f) {
			yaw += 360.0f;
		}
	}

	void StrafeRight(float scale = 1.0f)
	{
		strafe += movementSpeed * scale;
	}

	void StrafeLeft(float scale = 1.0f)
	{
		strafe -= movementSpeed * scale;
	}

	// Apply the camera's view transform to the current (modelview) matrix.
	void LookAt(void)
	{
		gluLookAt(head[0], head[1], head[2],
				head[0] + view[0],
				head[1] + view[1],
				head[2] + view[2],
				up[0], up[1], up[2]);
	}

	// Update the planes that make up the viewing frustum, from the current
	// OpenGL projection and modelview matrices.
	void UpdateFrustum(void)
	{
		float proj[16];		// Projection matrix
		float modl[16];		// Modelview matrix
		float clip[16];		// Combined matrix

		glGetFloatv(GL_PROJECTION_MATRIX, proj);
		glGetFloatv(GL_MODELVIEW_MATRIX, modl);

		// Combine the two matrices (multiply projection by modelview)
		clip[0]  = modl[0] * proj[0] + modl[1] * proj[4] + modl[2] * proj[8] + modl[3] * proj[12];
		clip[1]  = modl[0] * proj[1] + modl[1] * proj[5] + modl[2] * proj[9] + modl[3] * proj[13];
		clip[2]  = modl[0] * proj[2] + modl[1] * proj[6] + modl[2] * proj[10] + modl[3] * proj[14];
		clip[3]  = modl[0] * proj[3] + modl[1] * proj[7] + modl[2] * proj[11] + modl[3] * proj[15];

		clip[4]  = modl[4] * proj[0] + modl[5] * proj[4] + modl[6] * proj[8] + modl[7] * proj[12];
		clip[5]  = modl[4] * proj[1] + modl[5] * proj[5] + modl[6] * proj[9] + modl[7] * proj[13];
		clip[6]  = modl[4] * proj[2] + modl[5] * proj[6] + modl[6] * proj[10] + modl[7] * proj[14];
		clip[7]  = modl[4] * proj[3] + modl[5] * proj[7] + modl[6] * proj[11] + modl[7] * proj[15];

		clip[8]  = modl[8] * proj[0] + modl[9] * proj[4] + modl[10] * proj[8] + modl[11] * proj[12];
		clip[9]  = modl[8] * proj[1] + modl[9] * proj[5] + modl[10] * proj[9] + modl[11] * proj[13];
		clip[10] = modl[8] * proj[2] + modl[9] * proj[6] + modl[10] * proj[10] + modl[11] * proj[14];
		clip[11] = modl[8] * proj[3] + modl[9] * proj[7] + modl[10] * proj[11] + modl[11] * proj[15];

		clip[12] = modl[12] * proj[0] + modl[13] * proj[4] + modl[14] * proj[8] + modl[15] * proj[12];
		clip[13] = modl[12] * proj[1] + modl[13] * proj[5] + modl[14] * proj[9] + modl[15] * proj[13];
		clip[14] = modl[12] * proj[2] + modl[13] * proj[6] + modl[14] * proj[10] + modl[15] * proj[14];
		clip[15] = modl[12] * proj[3] + modl[13] * proj[7] + modl[14] * proj[11] + modl[15] * proj[15];

		// Extract and normalize the six frustum planes.
		ExtractFrustumPlane(0, clip, 0, -1.0f); // Right
		ExtractFrustumPlane(1, clip, 0,  1.0f); // Left
		ExtractFrustumPlane(2, clip, 1,  1.0f); // Bottom
		ExtractFrustumPlane(3, clip, 1, -1.0f); // Top
		ExtractFrustumPlane(4, clip, 2, -1.0f); // Far
		ExtractFrustumPlane(5, clip, 2,  1.0f); // Near
	}

	// Test an axis-aligned cube against the viewing frustum
	bool CubeFrustumTest(float x, float y, float z, float size)
	{
		for (int i = 0; i < 6; i++) {
			if (frustum[i][0] * (x - size) + frustum[i][1] * (y - size) + frustum[i][2] * (z - size) + frustum[i][3] > 0) continue;
			if (frustum[i][0] * (x + size) + frustum[i][1] * (y - size) + frustum[i][2] * (z - size) + frustum[i][3] > 0) continue;
			if (frustum[i][0] * (x - size) + frustum[i][1] * (y + size) + frustum[i][2] * (z - size) + frustum[i][3] > 0) continue;
			if (frustum[i][0] * (x + size) + frustum[i][1] * (y + size) + frustum[i][2] * (z - size) + frustum[i][3] > 0) continue;
			if (frustum[i][0] * (x - size) + frustum[i][1] * (y - size) + frustum[i][2] * (z + size) + frustum[i][3] > 0) continue;
			if (frustum[i][0] * (x + size) + frustum[i][1] * (y - size) + frustum[i][2] * (z + size) + frustum[i][3] > 0) continue;
			if (frustum[i][0] * (x - size) + frustum[i][1] * (y + size) + frustum[i][2] * (z + size) + frustum[i][3] > 0) continue;
			if (frustum[i][0] * (x + size) + frustum[i][1] * (y + size) + frustum[i][2] * (z + size) + frustum[i][3] > 0) continue;

			// The cube is not in the frustum
			return false;
		}

		return true;
	}

	float (*GetViewFrustum(void))[4] { return frustum; }

	void ExtractFrustumPlane(int plane, const float clip[16], int axis, float sign)
	{
		frustum[plane][0] = clip[3] + sign * clip[axis];
		frustum[plane][1] = clip[7] + sign * clip[4 + axis];
		frustum[plane][2] = clip[11] + sign * clip[8 + axis];
		frustum[plane][3] = clip[15] + sign * clip[12 + axis];

		float norm = sqrt(frustum[plane][0] * frustum[plane][0] +
				frustum[plane][1] * frustum[plane][1] +
				frustum[plane][2] * frustum[plane][2]);
		frustum[plane][0] /= norm;
		frustum[plane][1] /= norm;
		frustum[plane][2] /= norm;
		frustum[plane][3] /= norm;
	}
};

#endif
