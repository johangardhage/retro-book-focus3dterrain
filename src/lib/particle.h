//
// Focus on 3D Terrain Programming
//
// A simple particle engine: point particles with life, velocity, mass,
// friction, and external forces (gravity).
//
// Original coders: Trent Polack (trent@voxelsoft.com)
//
#ifndef _PARTICLE_H_
#define _PARTICLE_H_

#include <stdlib.h> // NULL

struct Particle
{
	float life;

	float position[3];
	float velocity[3];

	float mass;
	float size;

	float color[3];
	float translucency;

	float friction;
};

class ParticleEngine
{
private:
	Particle *particles;
	int numParticles;

	int numParticlesOnScreen;

	// External forces (gravity, etc.)
	float forces[3];

	// Base particle attributes (for newly created particles)
	float life;
	float position[3];
	float mass;
	float size;
	float color[3];
	float friction;

	void CreateParticle(float velX, float velY, float velZ);

public:
	bool Init(int numParticles);
	void Shutdown(void);

	void Update(void);
	void Render(void);

	void Explode(float magnitude, int numParticles);

	void SetLife(float l) { life = l; }
	void SetEmissionPosition(float x, float y, float z) { position[0] = x; position[1] = y; position[2] = z; }
	void SetMass(float m) { mass = m; }
	void SetSize(float pixelSize) { size = pixelSize; }
	void SetColor(float r, float g, float b) { color[0] = r; color[1] = g; color[2] = b; }
	void SetFriction(float f) { friction = f; }
	void SetExternalForces(float x, float y, float z) { forces[0] = x; forces[1] = y; forces[2] = z; }

	int GetNumParticlesOnScreen(void) { return numParticlesOnScreen; }

	ParticleEngine(void)
	{
		particles = NULL;
		numParticles = 0;
		numParticlesOnScreen = 0;
		life = 0.0f;
		mass = 0.0f;
		size = 1.0f;
		friction = 0.0f;
		forces[0] = forces[1] = forces[2] = 0.0f;
		position[0] = position[1] = position[2] = 0.0f;
		color[0] = color[1] = color[2] = 1.0f;
	}
	~ParticleEngine(void) {}
};

#endif
