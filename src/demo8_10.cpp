//
// Focus on 3D Terrain Programming - demo 8_10
//
// Author: Johan Gardhage <johan.gardhage@gmail.com>
//
// Making more realistic particles: textured, camera-facing billboard
// particles (a flare texture) instead of points. Press E for a new explosion.
// Ported from the original Win32/OpenGL demo to SDL3.
//
#include <time.h> // time
#include "lib/retromain.h"
#include "lib/particlesystem.h"

ParticleSystem particleEngine;

void DEMO_Startup(void)
{
	RETRO.title = "Demo 8_10: Making More Realistic Particles";
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
	particleEngine.SetMaxLife(200);
	particleEngine.SetEmissionPosition(0, 0, -50.0f);
	particleEngine.SetColor(0.1f, 1.0f, 0.25f, 0.1f, 1.0f, 0.25f);
	particleEngine.SetTranslucency(1.0f, 0.0f);
	particleEngine.SetSize(0.5f, 0.5f, 0.5f, 0.5f);
	particleEngine.SetMass(1.25f);
	particleEngine.SetFriction(0.01f);
	particleEngine.SetExternalForces(0.0f, -0.001f, 0.0f);
	particleEngine.LoadTexture("assets/flare.tga");
	particleEngine.Explode(0.15f, 1000);
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
