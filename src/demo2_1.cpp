//
// Focus on 3D Terrain Programming - demo 2_1
//
// Author: Johan Gardhage <johan.gardhage@gmail.com>
//
// Simple brute force terrain implementation. A grayscale RAW heightmap is
// loaded and rendered as triangle strips, with a 2D "minimap" of the height
// data in the corner. Ported from the original Win32/OpenGL demo to SDL3.
//
#include "lib/retromain.h"
#include "lib/bruteforce.h"

BruteForce terrain;

void DEMO_Startup(void)
{
	RETRO.title = "Demo 2_1: Simple Brute Force Implementation";
}

void DEMO_Initialize(void)
{
	// Load the height map and set up the terrain
	if (!terrain.LoadHeightMap("assets/height128.raw", 128)) {
		RETRO_RageQuit("[ERROR] Demo2_1::DEMO_Initialize() Unable to load height map\n");
	}
	terrain.SetHeightScale(0.25f);

	// Set the camera's position
	camera.SetPosition(64.0f, 128.0f, 256.0f);
	camera.SetPitch(-25.0f);
	camera.SetMovementSpeed(2.0f);
}

void DEMO_Deinitialize(void)
{
	terrain.UnloadHeightMap();
}

//
// Render the heightmap as a 2D "minimap" of grayscale points in the corner
//
void RenderMinimap(BruteForce *terrain, int width, int height)
{
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, width, 0, height, -1, 1);
	glMatrixMode(GL_MODELVIEW);

	glBegin(GL_POINTS);
	for (int z = 0; z < terrain->size; z++) {
		for (int x = 0; x < terrain->size; x++) {
			unsigned char color = terrain->GetTrueHeightAtPoint(x, z);
			glColor3ub(color, color, color);
			glVertex2d(x, height - z);
		}
	}
	glEnd();

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
}

void DEMO_Render(double deltatime)
{
	// Render the heightmap minimap in the corner (identity modelview)
	RenderMinimap(&terrain, RETRO.width, RETRO.height);

	// Setup a viewing matrix and transformation
	glLoadIdentity();
	camera.LookAt();

	// Render the terrain
	terrain.Render();
}
