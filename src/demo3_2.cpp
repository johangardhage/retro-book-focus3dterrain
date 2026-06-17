//
// Focus on 3D Terrain Programming - demo 3_2
//
// Author: Johan Gardhage <johan.gardhage@gmail.com>
//
// Texture map generation. Four terrain tiles are blended together based on
// height to generate a texture map for the brute-force terrain. Press T to
// toggle texturing. Ported from the original Win32/OpenGL demo to SDL3.
//
#include "lib/retromain.h"
#include "lib/bruteforce.h"

BruteForce terrain;
bool textureEnabled = true;
bool wireframeEnabled = false;

void DEMO_Startup(void)
{
	RETRO.title = "Demo 3_2: Texture Map Generation";
	RETRO.usagekeys = "T = toggle texturing, Tab = wireframe";
}

void DEMO_Initialize(void)
{
	// Generate the terrain
	terrain.MakeTerrainPlasma(128, 1.0f);
	terrain.SetHeightScale(0.25f);

	// Load the terrain tiles and generate a texture map from them
	terrain.LoadTile(LOWEST_TILE,  "assets/lowesttile.tga");
	terrain.LoadTile(LOW_TILE,     "assets/lowtile.tga");
	terrain.LoadTile(HIGH_TILE,    "assets/hightile.tga");
	terrain.LoadTile(HIGHEST_TILE, "assets/highesttile.tga");
	terrain.GenerateTextureMap(256);
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
