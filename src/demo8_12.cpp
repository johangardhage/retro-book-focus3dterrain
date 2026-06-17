//
// Focus on 3D Terrain Programming - demo 8_12
//
// Author: Johan Gardhage <johan.gardhage@gmail.com>
//
// The grand finale: a geomipmapping terrain with simulated water, a skydome,
// volumetric fog, camera-terrain collision, and a rain particle system around
// the camera. Ported from the original Win32/OpenGL demo to SDL3.
//
#include "lib/retromain.h"
#include "lib/geomipmapping.h"
#include "lib/watersim.h"
#include "lib/skydome.h"
#include "lib/particlesystem.h"

GeoMipMapping terrain;
WaterSim water;
Skydome skydome;
ParticleSystem particleEngine;

void DEMO_Startup(void)
{
	RETRO.title = "Demo 8_12: Applying a Particle Engine to the Outdoors (Rain)";
}

void DEMO_Initialize(void)
{
	// Generate the terrain
	terrain.MakeTerrainFault(513, 64, 0, 255, 0.15f);
	terrain.Scale(2.0f, 1.0f, 2.0f);

	// Set up slope lighting (dim, overcast)
	terrain.SetLightingType(SLOPE_LIGHT);
	terrain.SetLightColor(0.3f, 0.3f, 0.3f);
	terrain.CustomizeSlopeLighting(1, 1, 0.2f, 0.9f, 15);
	terrain.CalculateLighting();

	// Load the terrain tiles and generate a texture map from them
	terrain.LoadTile(LOWEST_TILE,  "assets/lowesttile.tga");
	terrain.LoadTile(LOW_TILE,     "assets/lowtile.tga");
	terrain.LoadTile(HIGH_TILE,    "assets/hightile.tga");
	terrain.LoadTile(HIGHEST_TILE, "assets/highesttile.tga");
	terrain.LoadDetailMap("assets/detailmap.tga");
	terrain.DoDetailMapping(true, 16);
	terrain.GenerateTextureMap(256);
	terrain.DoTextureMapping(true);
	terrain.DoMultitexturing(true);
	terrain.Init(17);
	terrain.SetFogDepth(150.0f);

	// Setup the per-vertex (volumetric) fog
	float fogColor[4] = { 0.9f, 0.9f, 0.9f, 1.0f };
	glFogi(GL_FOG_MODE, GL_LINEAR);
	glFogfv(GL_FOG_COLOR, fogColor);
	glFogf(GL_FOG_START, 0.0f);
	glFogf(GL_FOG_END, 150.0f);
	glFogi(GL_FOG_COORD_SRC, GL_FOG_COORD);

	// Initialize the water
	water.Init(1024.0f);
	water.LoadReflectionMap("assets/reflection_map.tga");
	water.SetColor(1.0f, 1.0f, 1.0f, 0.9f);

	// Initialize the skydome
	skydome.Init(5.0f, 5.0f, 256.0f);
	skydome.LoadTexture("assets/clouds2.tga");

	// Initialize the rain particle engine
	particleEngine.Init(2000);
	particleEngine.SetMaxLife(75);
	particleEngine.SetColor(1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
	particleEngine.SetTranslucency(1.0f, 0.3f);
	particleEngine.SetSize(0.1f, 2.0f, 0.1f, 2.0f);
	particleEngine.SetMass(1.25f);
	particleEngine.SetFriction(0.0f);
	particleEngine.SetExternalForces(0.0f, 0.0001f, 0.0f);
	particleEngine.LoadTexture("assets/flare.tga");
	particleEngine.Explode(0.12f, 777);

	// Set the camera's position
	camera.SetPosition(300.0f, 250.0f, 300.0f);
	camera.SetYaw(150.0f);
	camera.SetPitch(-20.0f);
	camera.SetMovementSpeed(4.0f);
}

void DEMO_Deinitialize(void)
{
	particleEngine.Shutdown();
	skydome.Shutdown();
	terrain.Shutdown();
	terrain.UnloadHeightMap();
	terrain.UnloadTexture();
	terrain.UnloadDetailMap();
	terrain.UnloadLightMap();
}

void DEMO_Render(double deltatime)
{
	// Collision detection and response against the (scaled) terrain mesh
	float fScale = terrain.GetScale()[0];
	if (camera.head[0] < 100) camera.head[0] = 100;
	else if (camera.head[0] > terrain.size * fScale - 100) camera.head[0] = terrain.size * fScale - 100;
	if (camera.head[2] < 100) camera.head[2] = 100;
	else if (camera.head[2] > terrain.size * fScale - 100) camera.head[2] = terrain.size * fScale - 100;

	float groundHeight = (float)terrain.GetScaledHeightAtPoint((int)(camera.head[0] / fScale), (int)(camera.head[2] / fScale));
	if (camera.head[1] < groundHeight + 8) {
		camera.head[1] = groundHeight + 8;
	}

	// Setup a viewing matrix and transformation
	camera.LookAt();
	camera.UpdateFrustum();

	// Render the skydome (centered on the camera, no depth write)
	glDepthMask(GL_FALSE);
	skydome.Set(camera.head[0], camera.head[1] - 200.0f, camera.head[2]);
	skydome.Render(0.009f, true);
	glDepthMask(GL_TRUE);

	// Update the water simulation
	water.Update(0.001f);
	water.CalcNormals();

	// Render the terrain with per-vertex fog enabled
	terrain.Update(&camera);
	glEnable(GL_FOG);
	terrain.Render();
	glDisable(GL_FOG);

	// Render the water mesh
	glPushMatrix();
	glTranslatef(0.0f, 75.0f, 0.0f);
	glDepthMask(GL_FALSE);
	water.Render(true);
	glDepthMask(GL_TRUE);
	glPopMatrix();

	// Rain: create drops around the camera, then update and render them
	particleEngine.CreateRaindrops(camera.head[0] - 150.0f, camera.head[1] - 150.0f, camera.head[2] - 150.0f,
		camera.head[0] + 150.0f, camera.head[1] + 150.0f, camera.head[2] + 150.0f, 6, 150);
	particleEngine.Update();
	particleEngine.Render();
}
