//
// Focus on 3D Terrain Programming - demo 6_3
//
// Author: Johan Gardhage <johan.gardhage@gmail.com>
//
// Adding frustum culling: quadtree nodes outside the camera's view frustum
// are skipped during refinement. Press T to toggle texturing, B to toggle the
// detail map. Ported from the original Win32/OpenGL demo to SDL3.
//
#include "lib/retromain.h"
#include "lib/quadtree.h"

QuadTree terrain;
bool textureEnabled = true;
bool detailEnabled = true;
bool wireframeEnabled = false;

void DEMO_Startup(void)
{
	RETRO.title = "Demo 6_3: Adding Frustum Culling";
	RETRO.usagekeys = "T = toggle texturing, B = toggle detail map, Tab = wireframe";
}

void DEMO_Initialize(void)
{
	// Generate the terrain
	terrain.MakeTerrainFault(513, 64, 0, 255, 0.25f);
	terrain.Scale(8.0f, 5.0f, 8.0f);

	// Load the terrain tiles and generate a texture map from them
	terrain.LoadTile(LOWEST_TILE,  "assets/lowesttile.tga");
	terrain.LoadTile(LOW_TILE,     "assets/lowtile.tga");
	terrain.LoadTile(HIGH_TILE,    "assets/hightile.tga");
	terrain.LoadTile(HIGHEST_TILE, "assets/highesttile.tga");
	terrain.GenerateTextureMap(256);

	// Load the detail map
	if (!terrain.LoadDetailMap("assets/detailmap.tga")) {
		RETRO_RageQuit("[ERROR] Demo6_3::DEMO_Initialize() Unable to load detail map\n");
	}

	terrain.DoTextureMapping(textureEnabled);
	terrain.DoDetailMapping(detailEnabled, 16);
	terrain.DoMultitexturing(true);

	// Set up slope lighting
	terrain.SetLightingType(SLOPE_LIGHT);
	terrain.SetLightColor(1.0f, 1.0f, 1.0f);
	terrain.CustomizeSlopeLighting(1, 1, 0.1f, 0.9f, 5);
	terrain.CalculateLighting();

	// Initialize the quadtree engine (with roughness propagation)
	terrain.SetDetailLevel(50.0f);
	terrain.SetMinResolution(10.0f);
	terrain.SetUseRoughness(true);
	terrain.Init();

	// Set the camera's position
	camera.SetPosition(64.0f, 1024.0f, 256.0f);
	camera.SetYaw(120.0f);
	camera.SetPitch(-25.0f);
	camera.SetMovementSpeed(20.0f);
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

	// Update and render the terrain (cull nodes outside the frustum)
	camera.UpdateFrustum();
	terrain.Update(&camera, true);
	terrain.Render();
}
