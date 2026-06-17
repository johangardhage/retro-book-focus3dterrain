//
// Focus on 3D Terrain Programming - demo 8_3
//
// Author: Johan Gardhage <johan.gardhage@gmail.com>
//
// Skybox: a split/merge ROAM terrain with simulated water, surrounded by a
// textured skybox centered on the camera. Ported from the original
// Win32/OpenGL demo to SDL3.
//
#include "lib/retromain.h"
#include "lib/roamsplitmerge.h"
#include "lib/watersim.h"
#include "lib/skybox.h"

RoamSplitMerge terrain;
WaterSim water;
Skybox skybox;
bool textureEnabled = true;
bool detailEnabled = true;
bool wireframeEnabled = false;

void DEMO_Startup(void)
{
	RETRO.title = "Demo 8_3: Skybox Fun!";
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
		RETRO_RageQuit("[ERROR] Demo8_3::DEMO_Initialize() Unable to load detail map\n");
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

	// Load the skybox textures
	skybox.LoadTexture(SBX_FRONT,  "assets/skybox_front.tga");
	skybox.LoadTexture(SBX_BACK,   "assets/skybox_back.tga");
	skybox.LoadTexture(SBX_RIGHT,  "assets/skybox_right.tga");
	skybox.LoadTexture(SBX_LEFT,   "assets/skybox_left.tga");
	skybox.LoadTexture(SBX_TOP,    "assets/skybox_top.tga");
	skybox.LoadTexture(SBX_BOTTOM, "assets/skybox_bottom.tga");

	// Set the camera's position
	camera.SetPosition(374.0f, 380.0f, 428.0f);
	camera.SetYaw(125.0f);
	camera.SetPitch(-30.0f);
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

	glFrontFace(GL_CW);
	glEnable(GL_CULL_FACE);

	// Update (process the split/merge queues) and render the terrain
	camera.UpdateFrustum();
	terrain.Update();

	// Update the water simulation
	water.Update(0.001f);
	water.CalcNormals();

	// Render the skybox centered on the camera
	skybox.Set(camera.head[0], camera.head[1], camera.head[2], 1024.0f);
	skybox.Render();

	terrain.Render();

	// Render the water mesh
	glPushMatrix();
	glTranslatef(0.0f, 75.0f, 0.0f);
	glDepthMask(GL_FALSE);
	water.Render(true);
	glDepthMask(GL_TRUE);
	glPopMatrix();
}
