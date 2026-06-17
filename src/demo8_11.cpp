//
// Focus on 3D Terrain Programming - demo 8_11
//
// Author: Johan Gardhage <johan.gardhage@gmail.com>
//
// Adding data interpolation to a particle engine: each particle's size, color
// and translucency are interpolated from a start to an end value over its
// life. Press E for a new explosion. Ported from the original Win32/OpenGL
// demo to SDL3.
//
#include <time.h> // time
#include "lib/retromain.h"
#include "lib/particlesystem.h"

ParticleSystem particleEngine;

void DEMO_Startup(void)
{
	RETRO.title = "Demo 8_11: Adding Time-Based Movement and Data Interpolation to a Particle Engine";
	RETRO.usagekeys = "E = create a particle explosion";
	RETRO.usecamera = false;
	RETRO.znear = 1.0;
	RETRO.zfar = 1000.0;
}

void DEMO_Initialize(void)
{
	srand(time(NULL));

	// Initialize the particle system
	particleEngine.Init(2500);
	particleEngine.SetMaxLife(120);
	particleEngine.SetEmissionPosition(0, 0, -50.0f);
	particleEngine.SetColor(1.0f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f);
	particleEngine.SetTranslucency(1.0f, 0.0f);
	particleEngine.SetSize(0.5f, 0.5f, 4.0f, 4.0f);
	particleEngine.SetMass(1.25f);
	particleEngine.SetFriction(0.0f);
	particleEngine.SetExternalForces(0.0f, 0.0001f, 0.0f);
	particleEngine.LoadTexture("assets/flare.tga");
	particleEngine.Explode(0.12f, 777);
}

void DEMO_Deinitialize(void)
{
	particleEngine.Shutdown();
}

void DEMO_Input(double deltatime)
{
	if (RETRO_KeyPressed(SDL_SCANCODE_E)) {
		particleEngine.Explode(0.15f, 1000);
	}
}

void DEMO_Render(double deltatime)
{
	// Update the particles
	particleEngine.Update();

	// Additive blending, no depth test, for the particles
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glDisable(GL_DEPTH_TEST);

	particleEngine.Render();

	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
}
