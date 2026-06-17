//
// Focus on 3D Terrain Programming
//
// ROAM with a diamond backbone: a split-only ROAM that maintains a pool of
// "diamonds" (shared split vertices) linked to their parents and children,
// recycled through a free list.
//
// Original coders: Trent Polack (trent@voxelsoft.com) and Mark Duchaineau
//
#ifndef _ROAMDIAMOND_H_
#define _ROAMDIAMOND_H_

#include "retrocamera.h"
#include "terrain.h"

// Frustum bitmasks
#define CULL_ALLIN 0x3f
#define CULL_OUT   0x40

struct Diamond
{
	Diamond *parent[4];				// Diamond's parents
	Diamond *child[4];				// Diamond's children
	Diamond *prev, *next;			// Prev/next links on the free list

	float vert[3];					// Vertex position
	float boundRad;					// Squared radius of the bounding sphere
	float errorRad;					// Squared pointwise error radius

	char childIndex[2];				// Our child index within each parent
	signed char level;				// Level of resolution
	unsigned char lockCount;		// Reference count (0 == free)
};

class RoamDiamond : public Terrain
{
private:
	RETRO_Camera *camera;

	Diamond *pool;					// Diamond storage pool
	int poolSize;					// Number of diamonds in the pool

	Diamond *freeDmnd[2];			// Head/tail of the free list
	Diamond *level0Dmnd[3][3];		// Base diamonds, level 0
	Diamond *level1Dmnd[4][4];		// Base diamonds, levels -1, -2

	float *levelMDSize;				// Max midpoint displacement per level (offset)
	float *levelMDAlloc;			// Actual allocation base (for delete)
	int maxLevel;

	// Scale an (x, z) vertex from [-1,1] into map coordinates
	void ShiftCoords(float *x, float *z)
	{
		*x = (*x + 1.0f) / 2.0f;
		*z = (*z + 1.0f) / 2.0f;
		*x *= size;
		*z *= size;
	}

	void RenderChild(Diamond *dmnd, int index, int cull, float texRepeat);
	Diamond *Create(void);
	Diamond *GetChild(Diamond *dmnd, int index);
	void Lock(Diamond *dmnd);
	void Unlock(Diamond *dmnd);

public:
	void Init(int maxLevel, int poolSize, RETRO_Camera *camera);
	void Shutdown(void);
	void Render(void);

	RoamDiamond(void)
	{
		camera = NULL;
		pool = NULL;
		poolSize = 0;
		levelMDSize = NULL;
		levelMDAlloc = NULL;
		maxLevel = 0;
	}
	~RoamDiamond(void) {}
};

#endif
