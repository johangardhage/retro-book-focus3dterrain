//
// Focus on 3D Terrain Programming - demo 7_3
//
// Author: Johan Gardhage <johan.gardhage@gmail.com>
//
// ROAM with a diamond backbone: a split-only ROAM that maintains a pool of
// "diamonds" (shared split vertices) linked to parents and children. Press T
// to toggle texturing, B to toggle the detail map. Ported from the original
// Win32/OpenGL demo to SDL3.
//
#include "lib/retromain.h"
#include "lib/roamdiamond.h"

RoamDiamond terrain;
bool textureEnabled = true;
bool detailEnabled = true;
bool wireframeEnabled = false;

void DEMO_Startup(void)
{
	RETRO.title = "Demo 7_3: Adding the Diamond Backbone Structure";
	RETRO.usagekeys = "T = toggle texturing, B = toggle detail map, Tab = wireframe";
}

void DEMO_Initialize(void)
{
	// Generate the terrain
	terrain.MakeTerrainPlasma(2048, 1.0f);

	// Load the terrain tiles and generate a texture map from them
	terrain.LoadTile(LOWEST_TILE,  "assets/lowesttile.tga");
	terrain.LoadTile(LOW_TILE,     "assets/lowtile.tga");
	terrain.LoadTile(HIGH_TILE,    "assets/hightile.tga");
	terrain.LoadTile(HIGHEST_TILE, "assets/highesttile.tga");
	terrain.GenerateTextureMap(512);

	// Load the detail map
	if (!terrain.LoadDetailMap("assets/detailmap.tga")) {
		RETRO_RageQuit("[ERROR] Demo7_3::DEMO_Initialize() Unable to load detail map\n");
	}

	terrain.DoTextureMapping(textureEnabled);
	terrain.DoDetailMapping(detailEnabled, 16);
	terrain.DoMultitexturing(true);

	// Set up slope lighting
	terrain.SetLightingType(SLOPE_LIGHT);
	terrain.SetLightColor(1.0f, 1.0f, 1.0f);
	terrain.CustomizeSlopeLighting(1, 1, 0.2f, 0.9f, 7);
	terrain.CalculateLighting();

	// Initialize the ROAM system
	terrain.Init(15, 32768, &camera);

	// Set the camera's position
	camera.SetPosition(128.0f, 512.0f, 512.0f);
	camera.SetYaw(100.0f);
	camera.SetPitch(-40.0f);
	camera.SetMovementSpeed(10.0f);
}

void DEMO_Deinitialize(void)
{
	terrain.Shutdown();
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
		terrain.DoDetailMapping(detailEnabled, 16);
	}
}

void DEMO_Render(double deltatime)
{
	// Setup a viewing matrix and transformation
	camera.LookAt();

	// Cull against the frustum and render the terrain
	camera.UpdateFrustum();
	terrain.Render();
}
