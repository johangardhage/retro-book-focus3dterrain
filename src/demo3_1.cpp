//
// Focus on 3D Terrain Programming - demo 3_1
//
// Author: Johan Gardhage <johan.gardhage@gmail.com>
//
// Simple texture mapping. A single color texture is stretched over the
// brute-force terrain. Press T to toggle texturing. Ported from the original
// Win32/OpenGL demo to SDL3.
//
#include "lib/retromain.h"
#include "lib/bruteforce.h"

BruteForce terrain;
bool textureEnabled = true;
bool wireframeEnabled = false;

void DEMO_Startup(void)
{
	RETRO.title = "Demo 3_1: Simple Texture Mapping";
	RETRO.usagekeys = "T = toggle texturing, Tab = wireframe";
}

void DEMO_Initialize(void)
{
	// Generate the terrain and load the color texture
	terrain.MakeTerrainPlasma(128, 1.0f);
	terrain.SetHeightScale(0.25f);
	if (!terrain.LoadTexture("assets/grass_1.tga")) {
		RETRO_RageQuit("[ERROR] Demo3_1::DEMO_Initialize() Unable to load texture\n");
	}
	terrain.DoTextureMapping(textureEnabled);

	// Set the camera's position
	camera.SetPosition(64.0f, 128.0f, 256.0f);
	camera.SetPitch(-25.0f);
	camera.SetMovementSpeed(2.0f);
}

void DEMO_Deinitialize(void)
{
	terrain.UnloadHeightMap();
	terrain.UnloadTexture();
}

void DEMO_Input(double deltatime)
{
	if (RETRO_KeyPressed(SDL_SCANCODE_TAB)) {
		wireframeEnabled = !wireframeEnabled;
		glPolygonMode(GL_FRONT_AND_BACK, wireframeEnabled ? GL_LINE : GL_FILL);
	}
	if (RETRO_KeyPressed(SDL_SCANCODE_T)) {
		textureEnabled = !textureEnabled;
		terrain.DoTextureMapping(textureEnabled);
	}
}

void DEMO_Render(double deltatime)
{
	// Setup a viewing matrix and transformation
	camera.LookAt();

	// Render the terrain
	terrain.Render();
}
