//
// Focus on 3D Terrain Programming
//
// Abstract terrain base class. All specific implementations are derived from
// it. Contains heightmap loading/saving and fractal terrain generation.
//
// Original coders: Trent Polack (trent@voxelsoft.com)
//
#ifndef _TERRAIN_H_
#define _TERRAIN_H_

#include <stdlib.h> // rand, RAND_MAX

struct HeightData
{
	unsigned char *data;	// The height data
	int size;				// The height size (must be a power of 2)
};

class Terrain
{
protected:
	HeightData heightData;	// The height data

	float heightScale;		// Scaling variable

	int vertsPerFrame;		// Stat variables
	int trisPerFrame;

	// Fractal terrain generation helpers
	void NormalizeTerrain(float *heightData);
	void FilterHeightBand(float *band, int stride, int count, float filter);
	void FilterHeightField(float *heightData, float filter);

public:
	int size;	// The size of the heightmap, must be a power of two

	virtual void Render(void) = 0;

	bool LoadHeightMap(const char *filename, int size);
	bool SaveHeightMap(const char *filename);
	bool UnloadHeightMap(void);

	bool MakeTerrainFault(int size, int iterations, int minDelta, int maxDelta, float filter);
	bool MakeTerrainPlasma(int size, float roughness);

	int GetNumVertsPerFrame(void) { return vertsPerFrame; }
	int GetNumTrisPerFrame(void) { return trisPerFrame; }

	// Get a random value between the two arguments
	float RangedRandom(float f1, float f2)
	{
		return (f1 + (f2 - f1) * ((float)rand()) / ((float)RAND_MAX));
	}

	void SetHeightScale(float scale) { heightScale = scale; }

	void SetHeightAtPoint(unsigned char height, int x, int z)
	{
		heightData.data[(z * size) + x] = height;
	}

	unsigned char GetTrueHeightAtPoint(int x, int z)
	{
		return (heightData.data[(z * size) + x]);
	}

	float GetScaledHeightAtPoint(int x, int z)
	{
		return ((float)(heightData.data[(z * size) + x]) * heightScale);
	}

	Terrain(void) { heightData.data = NULL; size = 0; heightScale = 1.0f; }
	~Terrain(void) {}
};

#endif
