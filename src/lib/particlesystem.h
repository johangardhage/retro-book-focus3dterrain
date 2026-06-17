//
// Focus on 3D Terrain Programming
//
// A textured, camera-facing (billboard) particle system with start/end
// interpolation of size, color and translucency over each particle's life.
//
// Original coders: Trent Polack (trent@voxelsoft.com)
//
#ifndef _PARTICLESYSTEM_H_
#define _PARTICLESYSTEM_H_

#include <stdlib.h> // NULL

struct SysParticle
{
	float life;

	float position[3];
	float velocity[3];

	float size[3];
	float sizeCounter[3];
	float mass;

	float color[3];
	float colorCounter[3];
	float translucency;
	float translucencyCounter;

	float friction;
};

class ParticleSystem
{
private:
	SysParticle *particles;
	int numParticles;

	int numParticlesOnScreen;

	float forces[3];	// Gravity, etc.

	// Base attributes for newly created particles
	float maxLife;
	float position[3];
	float startSize[3], endSize[3];
	float mass;
	float startColor[3], endColor[3];
	float startTranslucency, endTranslucency;
	float friction;

	unsigned int texID;

	void CreateParticle(float velX, float velY, float velZ);

	float RangedRandom(float f1, float f2)
	{
		return (f1 + (f2 - f1) * ((float)rand() / (float)RAND_MAX));
	}

public:
	bool Init(int numParticles);
	void Shutdown(void);

	void Update(void);
	void Render(void);

	void Explode(float magnitude, int numParticles);
	void CreateRaindrops(float minX, float minY, float minZ,
		float maxX, float maxY, float maxZ, int dropSpeed, int numDrops);

	void LoadTexture(const char *filename);

	void SetMaxLife(float l) { maxLife = l; }
	void SetEmissionPosition(float x, float y, float z) { position[0] = x; position[1] = y; position[2] = z; }
	void SetMass(float m) { mass = m; }
	void SetSize(float startW, float startH, float endW, float endH)
	{
		startSize[0] = startW; startSize[1] = 0.0f; startSize[2] = startH;
		endSize[0] = endW; endSize[1] = 0.0f; endSize[2] = endH;
	}
	void SetColor(float sr, float sg, float sb, float er, float eg, float eb)
	{
		startColor[0] = sr; startColor[1] = sg; startColor[2] = sb;
		endColor[0] = er; endColor[1] = eg; endColor[2] = eb;
	}
	void SetTranslucency(float start, float end) { startTranslucency = start; endTranslucency = end; }
	void SetFriction(float f) { friction = f; }
	void SetExternalForces(float x, float y, float z) { forces[0] = x; forces[1] = y; forces[2] = z; }

	int GetNumParticlesOnScreen(void) { return numParticlesOnScreen; }

	ParticleSystem(void)
	{
		particles = NULL;
		numParticles = 0;
		numParticlesOnScreen = 0;
		maxLife = 1.0f;
		mass = 1.0f;
		friction = 0.0f;
		startTranslucency = 1.0f;
		endTranslucency = 0.0f;
		texID = 0;
		forces[0] = forces[1] = forces[2] = 0.0f;
		position[0] = position[1] = position[2] = 0.0f;
		startSize[0] = startSize[1] = startSize[2] = 1.0f;
		endSize[0] = endSize[1] = endSize[2] = 1.0f;
		startColor[0] = startColor[1] = startColor[2] = 1.0f;
		endColor[0] = endColor[1] = endColor[2] = 1.0f;
	}
	~ParticleSystem(void) {}
};

#endif
