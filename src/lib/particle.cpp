//
// Focus on 3D Terrain Programming
//
// A simple particle engine.
//
// Original coders: Trent Polack (trent@voxelsoft.com)
//
#include <math.h> // cos, sin
#include <stdlib.h> // rand, RAND_MAX
#include "retrogl.h"
#include "particle.h"

#define PI 3.1415926535897932f
#define RANDOM_FLOAT ((float)rand() / (float)RAND_MAX)
#define DEG_TO_RAD(a) ((a) * PI / 180.0f)

//
// Initialize the particle engine
//
bool ParticleEngine::Init(int num)
{
	numParticles = num;
	particles = new Particle[numParticles];
	if (particles == NULL) {
		return false;
	}

	// All particles start dead
	for (int i = 0; i < numParticles; i++) {
		particles[i].life = 0.0f;
	}

	return true;
}

//
// Shutdown the system
//
void ParticleEngine::Shutdown(void)
{
	delete[] particles;
	particles = NULL;
}

//
// Create a new particle with the given velocity (reuses the first dead slot)
//
void ParticleEngine::CreateParticle(float velX, float velY, float velZ)
{
	int choice = -1;

	for (int i = 0; i < numParticles; i++) {
		if (particles[i].life <= 0.0f) {
			choice = i;
			break;
		}
	}

	if (choice == -1) {
		return;
	}

	particles[choice].life = life;

	particles[choice].position[0] = position[0];
	particles[choice].position[1] = position[1];
	particles[choice].position[2] = position[2];

	particles[choice].velocity[0] = velX;
	particles[choice].velocity[1] = velY;
	particles[choice].velocity[2] = velZ;

	particles[choice].color[0] = color[0];
	particles[choice].color[1] = color[1];
	particles[choice].color[2] = color[2];
	particles[choice].translucency = 1.0f;

	particles[choice].size = size;
	particles[choice].mass = mass;

	particles[choice].friction = friction;
}

//
// Update the particle engine
//
void ParticleEngine::Update(void)
{
	for (int i = 0; i < numParticles; i++) {
		// Age the particle
		particles[i].life -= 1;

		if (particles[i].life > 0.0f) {
			// Update the position by the momentum (velocity * mass)
			particles[i].position[0] += particles[i].velocity[0] * particles[i].mass;
			particles[i].position[1] += particles[i].velocity[1] * particles[i].mass;
			particles[i].position[2] += particles[i].velocity[2] * particles[i].mass;

			// Fade out with age
			particles[i].translucency = particles[i].life / life;

			// Apply friction and external forces
			particles[i].velocity[0] *= 1 - particles[i].friction;
			particles[i].velocity[1] *= 1 - particles[i].friction;
			particles[i].velocity[2] *= 1 - particles[i].friction;
			particles[i].velocity[0] += forces[0];
			particles[i].velocity[1] += forces[1];
			particles[i].velocity[2] += forces[2];
		}
	}
}

//
// Render the particle engine (as points)
//
void ParticleEngine::Render(void)
{
	numParticlesOnScreen = 0;

	for (int i = 0; i < numParticles; i++) {
		if (particles[i].life > 0.0f) {
			glPointSize(particles[i].size);

			glBegin(GL_POINTS);
				glColor4f(particles[i].color[0], particles[i].color[1], particles[i].color[2], particles[i].translucency);
				glVertex3f(particles[i].position[0], particles[i].position[1], particles[i].position[2]);
			glEnd();

			numParticlesOnScreen++;
		}
	}
}

//
// Make an explosion of particles
//
void ParticleEngine::Explode(float magnitude, int num)
{
	while (--num > 0) {
		float yaw = RANDOM_FLOAT * PI * 2.0f;
		float pitch = DEG_TO_RAD(RANDOM_FLOAT * (rand() % 360));

		CreateParticle((cosf(pitch)) * (magnitude * RANDOM_FLOAT),
			(sinf(pitch) * cosf(yaw)) * (magnitude * RANDOM_FLOAT),
			(sinf(pitch) * sinf(yaw)) * (magnitude * RANDOM_FLOAT));
	}
}
