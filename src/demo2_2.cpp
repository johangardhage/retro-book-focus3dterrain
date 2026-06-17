//
// Focus on 3D Terrain Programming - demo 2_2
//
// Author: Johan Gardhage <johan.gardhage@gmail.com>
//
// Fractal terrain generation. The terrain is generated procedurally using the
// fault formation or midpoint displacement (plasma) algorithm and rendered
// brute force, with a 2D "minimap" of the height data in the corner.
//
// The original used a Win32 menu/dialog to regenerate the terrain; here that
// is replaced with keyboard controls (F = fault formation, P = plasma).
//
#include "lib/retromain.h"
#include "lib/bruteforce.h"

#define TERRAIN_SIZE	128
#define FAULT_FILTER	0.4f
#define PLASMA_ROUGHNESS	1.0f

BruteForce terrain;
bool wireframeEnabled = false;

void DEMO_Startup(void)
{
	RETRO.title = "Demo 2_2: Fractal Terrain Generation";
	RETRO.usagekeys = "F = fault formation, P = midpoint displacement (plasma), Tab = wireframe";
}

void DEMO_Initialize(void)
{
	// Generate the initial terrain (midpoint displacement)
	terrain.MakeTerrainPlasma(TERRAIN_SIZE, PLASMA_ROUGHNESS);
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

void DEMO_Input(double deltatime)
{
	if (RETRO_KeyPressed(SDL_SCANCODE_TAB)) {
		wireframeEnabled = !wireframeEnabled;
		glPolygonMode(GL_FRONT_AND_BACK, wireframeEnabled ? GL_LINE : GL_FILL);
	}
	if (RETRO_KeyPressed(SDL_SCANCODE_F)) {
		// Generate a new height map using fault formation
		terrain.MakeTerrainFault(TERRAIN_SIZE, 64, 0, 255, FAULT_FILTER);
		terrain.SetHeightScale(0.25f);
	}
	if (RETRO_KeyPressed(SDL_SCANCODE_P)) {
		// Generate a new height map using midpoint displacement
		terrain.MakeTerrainPlasma(TERRAIN_SIZE, PLASMA_ROUGHNESS);
		terrain.SetHeightScale(0.25f);
	}
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
