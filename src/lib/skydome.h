//
// Focus on 3D Terrain Programming
//
// A skydome (half-sphere) system, rendered as a textured triangle strip that
// can slowly rotate to simulate moving clouds.
//
// Original coders: Trent Polack (trent@voxelsoft.com)
//
#ifndef _SKYDOME_H_
#define _SKYDOME_H_

#include "image.h"

class Skydome
{
private:
	float *vertices;
	float *texCoords;

	float center[3];

	int numVertices;

	unsigned int texID;

	// Fractal cloud-texture generation helpers
	float CosineInterpolation(float num1, float num2, float x);
	float RangedRandom(int x, int y);
	float RangedSmoothRandom(int x, int y);
	float Noise(float x, float y);
	float FBM(float x, float y, float octaves, float amplitude, float frequency, float h);
	void NormalizeFractal(float *data, int size);
	void BlurBand(float *band, int stride, int count, float filter);
	void Blur(float *data, int size, float filter);

public:
	void Init(float theta, float phi, float radius);
	void Shutdown(void);
	void Render(float delta, bool rotate);
	void LoadTexture(const char *filename);
	void GenCloudTexture(int size, float blur, float octaves, float amplitude, float frequency, float h);

	void Set(float cx, float cy, float cz)
	{
		center[0] = cx;
		center[1] = cy;
		center[2] = cz;
	}

	int GetNumVertices(void) { return numVertices; }
	int GetNumTriangles(void) { return numVertices - 1; }

	Skydome(void)
	{
		vertices = NULL;
		texCoords = NULL;
		numVertices = 0;
		texID = 0;
	}
	~Skydome(void) {}
};

#endif
