//
// Focus on 3D Terrain Programming - demo 7_1
//
// Author: Johan Gardhage <johan.gardhage@gmail.com>
//
// Simple ROAM LOD: a fractal terrain produced by recursive triangle
// subdivision, drawn with a grid texture. Ported from the original
// Win32/OpenGL demo to SDL3.
//
#include "lib/retromain.h"
#include "lib/roamlod.h"

RoamLOD roam;

void DEMO_Startup(void)
{
	RETRO.title = "Demo 7_1: Simple ROAM LOD Implementation";
	RETRO.znear = 0.01;
	RETRO.zfar = 100.0;
}

void DEMO_Initialize(void)
{
	// Initial camera (matches the original demo)
	camera.SetPosition(0.0f, 2.0f, -2.0f);
	camera.SetYaw(175.0f);
	camera.SetPitch(-40.0f);
	camera.SetMovementSpeed(0.1f);

	// Initialize the ROAM system (needs a valid GL context)
	roam.Init(15, &camera);
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
	roam.Render();
}
