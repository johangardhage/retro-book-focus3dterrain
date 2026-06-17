//
// Focus on 3D Terrain Programming - demo 8_6
//
// Author: Johan Gardhage <johan.gardhage@gmail.com>
//
// Camera-terrain collision detection and simple response: the camera is kept
// above the terrain surface as it moves. Ported from the original Win32/OpenGL
// demo to SDL3.
//
#include "lib/retromain.h"
#include "lib/roamsplitmerge.h"
#include "lib/watersim.h"
#include "lib/skydome.h"

RoamSplitMerge terrain;
WaterSim water;
Skydome skydome;
bool textureEnabled = true;
bool detailEnabled = true;
bool wireframeEnabled = false;

void DEMO_Startup(void)
{
	RETRO.title = "Demo 8_6: Camera-Terrain Collision Detection and Simple Response";
	RETRO.usagekeys = "T = toggle texturing, B = toggle detail map, Tab = wireframe";
}

void DEMO_Initialize(void)
{
	// Generate the terrain
	terrain.MakeTerrainPlasma(1024, 1.0f);

	// Load the terrain tiles and generate a texture map from them
	terrain.LoadTile(LOWEST_TILE,  "assets/lowesttile.tga");
	terrain.LoadTile(LOW_TILE,     "assets/lowtile.tga");
	terrain.LoadTile(HIGH_TILE,    "assets/hightile.tga");
	terrain.LoadTile(HIGHEST_TILE, "assets/highesttile.tga");
	terrain.GenerateTextureMap(512);

	// Load the detail map
	if (!terrain.LoadDetailMap("assets/detailmap.tga")) {
		RETRO_RageQuit("[ERROR] Demo8_6::DEMO_Initialize() Unable to load detail map\n");
	}

	terrain.DoTextureMapping(textureEnabled);
	terrain.DoDetailMapping(detailEnabled, 16);
	terrain.DoMultitexturing(false);

	// Set up slope lighting
	terrain.SetLightingType(SLOPE_LIGHT);
	terrain.SetLightColor(1.0f, 1.0f, 1.0f);
	terrain.CustomizeSlopeLighting(1, 1, 0.2f, 0.9f, 7);
	terrain.CalculateLighting();

	// Initialize the ROAM system
	terrain.Init(15, 65536, &camera);
	terrain.SetMaxTrisPerFrame(5000);

	// Initialize the water
	water.Init(1024.0f);
	water.LoadReflectionMap("assets/reflection_map.tga");
	water.SetColor(1.0f, 1.0f, 1.0f, 0.9f);

	// Initialize the skydome with a procedurally generated cloud texture
	skydome.Init(5.0f, 5.0f, 512.0f);
	skydome.GenCloudTexture(256, 0.5f, 25, 1.0f, 0.25f, 0.25f);

	// Set the camera's position
	camera.SetPosition(374.0f, 350.0f, 428.0f);
	camera.SetYaw(125.0f);
	camera.SetPitch(-42.0f);
	camera.SetMovementSpeed(4.0f);
}

void DEMO_Deinitialize(void)
{
	skydome.Shutdown();
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
	// Collision detection and response against the terrain mesh
	if (camera.head[0] < 0) camera.head[0] = 0;
	else if (camera.head[0] > terrain.size - 1) camera.head[0] = terrain.size - 1;
	if (camera.head[2] < 0) camera.head[2] = 0;
	else if (camera.head[2] > terrain.size - 1) camera.head[2] = terrain.size - 1;

	float groundHeight = (float)terrain.GetTrueHeightAtPoint((int)camera.head[0], (int)camera.head[2]);
	if (camera.head[1] < groundHeight + 5) {
		camera.head[1] = groundHeight + 5;
	}

	// Setup a viewing matrix and transformation
	camera.LookAt();

	glFrontFace(GL_CW);
	glEnable(GL_CULL_FACE);

	// Update (process the split/merge queues) and render the terrain
	camera.UpdateFrustum();
	terrain.Update();

	// Update the water simulation
	water.Update(0.001f);
	water.CalcNormals();

	// Render the skydome centered on the camera
	skydome.Set(camera.head[0], camera.head[1] - 400.0f, camera.head[2]);
	skydome.Render(0.009f, true);

	terrain.Render();

	// Render the water mesh
	glPushMatrix();
	glTranslatef(0.0f, 75.0f, 0.0f);
	glDepthMask(GL_FALSE);
	water.Render(true);
	glDepthMask(GL_TRUE);
	glPopMatrix();
}
