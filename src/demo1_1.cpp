//
// Focus on 3D Terrain Programming - demo 1_1
//
// Author: Johan Gardhage <johan.gardhage@gmail.com>
//
// A ROAM (Real-time Optimally Adapting Meshes) fractal terrain. Ported from
// the original Win32/OpenGL demo to SDL3 for windowing and input, keeping the
// OpenGL rendering.
//
#include "lib/retromain.h"
#include "lib/roam.h"

ROAM roam;

void DEMO_Startup(void)
{
	RETRO.title = "Demo 1_1: ROAM Terrain";
	RETRO.znear = 0.01;
	RETRO.zfar = 100.0;
}

void DEMO_Initialize(void)
{
	// Set the camera's position
	camera.SetPosition(0.0f, 2.0f, -2.0f);
	camera.SetYaw(175.0f);
	camera.SetPitch(-38.5f);
	camera.SetMovementSpeed(0.05f);

	// Initialize the ROAM system (needs a valid GL context)
	roam.Initialize();
}

void DEMO_Deinitialize(void)
{
	roam.Shutdown();
}

void DEMO_Render(double deltatime)
{
	// Setup a viewing matrix and transformation
	camera.LookAt();

	// Render the terrain
	roam.Update(camera.head);
	roam.Render();
}
