//
// Focus on 3D Terrain Programming - demo 5_1
//
// Author: Johan Gardhage <johan.gardhage@gmail.com>
//
// The basics of geomipmapping: the terrain is split into patches, each drawn
// at a distance-based level of detail as triangle fans. Press T to toggle
// texturing, B to toggle the detail map. Ported from the original
// Win32/OpenGL demo to SDL3.
//
#include "lib/retromain.h"
#include "lib/geomipmapping.h"

GeoMipMapping terrain;
bool textureEnabled = true;
bool detailEnabled = true;
bool wireframeEnabled = false;

void DEMO_Startup(void)
{
	RETRO.title = "Demo 5_1: The Basics of Geomipmapping";
	RETRO.usagekeys = "T = toggle texturing, B = toggle detail map, Tab = wireframe";
}

void DEMO_Initialize(void)
{
	// Generate the terrain
	terrain.MakeTerrainFault(257, 64, 0, 255, 0.05f);
	terrain.Scale(8.0f, 3.0f, 8.0f);

	// Load the terrain tiles and generate a texture map from them
	terrain.LoadTile(LOWEST_TILE,  "assets/lowesttile.tga");
	terrain.LoadTile(LOW_TILE,     "assets/lowtile.tga");
	terrain.LoadTile(HIGH_TILE,    "assets/hightile.tga");
	terrain.LoadTile(HIGHEST_TILE, "assets/highesttile.tga");
	terrain.GenerateTextureMap(256);

	// Load the detail map
	if (!terrain.LoadDetailMap("assets/detailmap.tga")) {
		RETRO_RageQuit("[ERROR] Demo5_1::DEMO_Initialize() Unable to load detail map\n");
	}

	terrain.DoTextureMapping(textureEnabled);
	terrain.DoDetailMapping(detailEnabled, 16);
	terrain.DoMultitexturing(true);

	// Set up slope lighting
	terrain.SetLightingType(SLOPE_LIGHT);
	terrain.SetLightColor(1.0f, 1.0f, 1.0f);
	terrain.CustomizeSlopeLighting(1, 1, 0.2f, 0.9f, 15);
	terrain.CalculateLighting();

	// Initialize the geomipmapping patch system
	terrain.Init(17);

	// Set the camera's position
	camera.SetPosition(64.0f, 1000.0f, 256.0f);
	camera.SetYaw(130.0f);
	camera.SetPitch(-25.0f);
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

	// Update and render the terrain
	terrain.Update(&camera, false);
	terrain.Render();
}
