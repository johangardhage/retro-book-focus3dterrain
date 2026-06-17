//
// Focus on 3D Terrain Programming - demo 4_3
//
// Author: Johan Gardhage <johan.gardhage@gmail.com>
//
// Slope lighting. The terrain is shaded by the slope between neighbouring
// height samples, producing soft directional shadows. Press T to toggle
// texturing, B to toggle the detail map. Ported from the original demo to SDL3.
//
#include "lib/retromain.h"
#include "lib/bruteforce.h"

BruteForce terrain;
bool textureEnabled = true;
bool detailEnabled = true;
bool wireframeEnabled = false;

void DEMO_Startup(void)
{
	RETRO.title = "Demo 4_3: Slope Lighting";
	RETRO.usagekeys = "T = toggle texturing, B = toggle detail map, Tab = wireframe";
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

	// Load the detail map
	if (!terrain.LoadDetailMap("assets/detailmap.tga")) {
		RETRO_RageQuit("[ERROR] Demo4_3::DEMO_Initialize() Unable to load detail map\n");
	}

	terrain.DoTextureMapping(textureEnabled);
	terrain.DoDetailMapping(detailEnabled, 8);
	terrain.DoMultitexturing(true);

	// Set up slope lighting
	terrain.SetLightingType(SLOPE_LIGHT);
	terrain.SetLightColor(1.0f, 1.0f, 1.0f);
	terrain.CustomizeSlopeLighting(1, 1, 0.2f, 0.9f, 15);
	terrain.CalculateLighting();

	// Set the camera's position
	camera.SetPosition(64.0f, 128.0f, 256.0f);
	camera.SetPitch(-25.0f);
	camera.SetMovementSpeed(2.0f);
}

void DEMO_Deinitialize(void)
{
	terrain.UnloadHeightMap();
	terrain.UnloadTexture();
	terrain.UnloadDetailMap();
	terrain.UnloadLightMap();
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
	if (RETRO_KeyPressed(SDL_SCANCODE_B)) {
		detailEnabled = !detailEnabled;
		terrain.DoDetailMapping(detailEnabled, 8);
	}
}

void DEMO_Render(double deltatime)
{
	// Setup a viewing matrix and transformation
	camera.LookAt();

	// Render the terrain (scaled up)
	glPushMatrix();
	glScalef(2.0f, 2.0f, 2.0f);
	terrain.Render();
	glPopMatrix();
}
