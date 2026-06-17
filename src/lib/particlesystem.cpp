//
// Focus on 3D Terrain Programming
//
// A textured billboard particle system with start/end interpolation.
//
// Original coders: Trent Polack (trent@voxelsoft.com)
//
#include <math.h> // cos, sin
#include <stdlib.h> // rand, RAND_MAX
#include "retrogl.h"
#include "image.h"
#include "particlesystem.h"

#define PI 3.1415926535897932f
#define RANDOM_FLOAT ((float)rand() / (float)RAND_MAX)
#define DEG_TO_RAD(a) ((a) * PI / 180.0f)

bool ParticleSystem::Init(int num)
{
	numParticles = num;
	particles = new SysParticle[numParticles];
	if (particles == NULL) {
		return false;
	}

	for (int i = 0; i < numParticles; i++) {
		particles[i].life = 0.0f;
	}

	return true;
}

void ParticleSystem::Shutdown(void)
{
	delete[] particles;
	particles = NULL;
}

void ParticleSystem::LoadTexture(const char *filename)
{
	Image tex;
	tex.Load(filename, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, true);
	texID = tex.GetID();
}

//
// Create a new particle (reuses the first dead slot), setting up the start
// values and the per-frame interpolation counters toward the end values
//
void ParticleSystem::CreateParticle(float velX, float velY, float velZ)
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

	SysParticle *p = &particles[choice];

	p->life = RANDOM_FLOAT * maxLife;

	p->position[0] = position[0];
	p->position[1] = position[1];
	p->position[2] = position[2];

	p->velocity[0] = velX;
	p->velocity[1] = velY;
	p->velocity[2] = velZ;

	for (int j = 0; j < 3; j++) {
		p->color[j] = startColor[j];
		p->colorCounter[j] = (endColor[j] - startColor[j]) / p->life;
		p->size[j] = startSize[j];
		p->sizeCounter[j] = (endSize[j] - startSize[j]) / p->life;
	}

	p->translucency = startTranslucency;
	p->translucencyCounter = (endTranslucency - startTranslucency) / p->life;

	p->mass = mass;
	p->friction = friction;
}

//
// Update the particle system
//
void ParticleSystem::Update(void)
{
	for (int i = 0; i < numParticles; i++) {
		SysParticle *p = &particles[i];

		p->life -= 1;

		if (p->life > 0.0f) {
			// Move by the momentum (velocity * mass)
			p->position[0] += p->velocity[0] * p->mass;
			p->position[1] += p->velocity[1] * p->mass;
			p->position[2] += p->velocity[2] * p->mass;

			// Interpolate the per-particle members toward their end values
			for (int j = 0; j < 3; j++) {
				p->color[j] += p->colorCounter[j];
				p->size[j] += p->sizeCounter[j];
			}
			p->translucency += p->translucencyCounter;

			// Apply friction and external forces
			p->velocity[0] *= 1 - p->friction;
			p->velocity[1] *= 1 - p->friction;
			p->velocity[2] *= 1 - p->friction;
			p->velocity[0] += forces[0];
			p->velocity[1] += forces[1];
			p->velocity[2] += forces[2];
		}
	}
}

//
// Render the particle system as camera-facing textured billboards
//
void ParticleSystem::Render(void)
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glDisable(GL_DEPTH_TEST);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texID);

	// Extract the right and up vectors from the modelview matrix (billboarding)
	float m[16];
	glGetFloatv(GL_MODELVIEW_MATRIX, m);
	float right[3] = { m[0], m[4], m[8] };
	float up[3] = { m[1], m[5], m[9] };

	numParticlesOnScreen = 0;

	for (int i = 0; i < numParticles; i++) {
		SysParticle *p = &particles[i];
		if (p->life > 0.0f) {
			float sx = p->size[0];
			float sy = p->size[2];
			float sz = (sx + sy) / 2;
			float *pos = p->position;

			glBegin(GL_TRIANGLE_STRIP);
				glColor4f(p->color[0], p->color[1], p->color[2], p->translucency);

				// Top right: (right + up)
				glTexCoord2f(1, 1);
				glVertex3f((right[0] + up[0]) * sx + pos[0], (right[1] + up[1]) * sy + pos[1], (right[2] + up[2]) * sz + pos[2]);

				// Top left: (up - right)
				glTexCoord2f(0, 1);
				glVertex3f((up[0] - right[0]) * sx + pos[0], (up[1] - right[1]) * sy + pos[1], (up[2] - right[2]) * sz + pos[2]);

				// Bottom right: (right - up)
				glTexCoord2f(1, 0);
				glVertex3f((right[0] - up[0]) * sx + pos[0], (right[1] - up[1]) * sy + pos[1], (right[2] - up[2]) * sz + pos[2]);

				// Bottom left: -(right + up)
				glTexCoord2f(0, 0);
				glVertex3f(-(right[0] + up[0]) * sx + pos[0], -(right[1] + up[1]) * sy + pos[1], -(right[2] + up[2]) * sz + pos[2]);
			glEnd();

			numParticlesOnScreen++;
		}
	}

	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
}

//
// Make an explosion of particles
//
void ParticleSystem::Explode(float magnitude, int num)
{
	while (--num > 0) {
		float yaw = RANDOM_FLOAT * PI * 2.0f;
		float pitch = DEG_TO_RAD(RANDOM_FLOAT * (rand() % 360));

		CreateParticle((cosf(pitch)) * (magnitude * RANDOM_FLOAT),
			(sinf(pitch) * cosf(yaw)) * (magnitude * RANDOM_FLOAT),
			(sinf(pitch) * sinf(yaw)) * (magnitude * RANDOM_FLOAT));
	}
}

//
// Make a series of raindrops falling within the given box-shaped area
//
void ParticleSystem::CreateRaindrops(float minX, float minY, float minZ,
	float maxX, float maxY, float maxZ, int dropSpeed, int numDrops)
{
	// Store the current emission position
	float saved[3] = { position[0], position[1], position[2] };

	for (int i = 0; i < numDrops; i++) {
		SetEmissionPosition(RangedRandom(minX, maxX), RangedRandom(minY, maxY), RangedRandom(minZ, maxZ));
		CreateParticle(0.0f, (float)-dropSpeed, 0.0f);
	}

	// Restore the old emission position
	position[0] = saved[0];
	position[1] = saved[1];
	position[2] = saved[2];
}
