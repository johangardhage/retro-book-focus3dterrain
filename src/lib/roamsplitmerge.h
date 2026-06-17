//
// Focus on 3D Terrain Programming
//
// Full ROAM with split/merge priority queues: a frame-coherent ROAM that keeps
// dual priority queues (split and merge) over a diamond pool and incrementally
// refines/coarsens the mesh each frame toward a target triangle count.
//
// Original coders: Trent Polack (trent@voxelsoft.com) and Mark Duchaineau
//
#ifndef _ROAMSPLITMERGE_H_
#define _ROAMSPLITMERGE_H_

#include "retrocamera.h"
#include "terrain.h"

#define IQMAX        4096	// Number of buckets in the priority queue
#define TRI_IMAX     65536	// Number of triangle-chunk slots

// Frustum bitmasks
#define CULL_ALLIN 0x3f
#define CULL_OUT   0x40

// Misc. diamond flags
#define ROAM_SPLIT   0x01
#define ROAM_TRI0    0x04
#define ROAM_TRI1    0x08
#define ROAM_CLIPPED 0x10
#define ROAM_SPLITQ  0x40
#define ROAM_MERGEQ  0x80
#define ROAM_ALLQ    0xc0
#define ROAM_UNQ     0x00

// Parent/child split flags
#define SPLIT_K0 0x01
#define SPLIT_K  0x0f
#define SPLIT_P0 0x10
#define SPLIT_P  0x30

struct SMDiamond
{
	SMDiamond *parent[4];				// Diamond's parents
	SMDiamond *child[4];				// Diamond's children
	SMDiamond *prev, *next;				// Prev/next links on a queue or the free list

	float vert[3];						// Vertex position
	float boundRad;						// Squared bounding-sphere radius
	float errorRad;						// Squared pointwise error radius

	short queueIndex;
	unsigned short triIndex[2];

	char childIndex[2];					// Our child index within each parent
	signed char level;					// Level of resolution
	unsigned char lockCount;			// Reference count (0 == free)
	unsigned char frameCount;
	unsigned char cull;
	unsigned char flags;
	unsigned char splitFlags;
};

class RoamSplitMerge : public Terrain
{
private:
	RETRO_Camera *camera;

	SMDiamond *pool;					// Diamond storage pool
	int poolSize;

	SMDiamond *freeDmnd[2];				// Head/tail of the free list
	SMDiamond *level0Dmnd[4][4];		// Base diamonds, level 0
	SMDiamond *level1Dmnd[4][4];		// Base diamonds, levels -1, -2

	SMDiamond *splitQueue[IQMAX];		// Split priority queue
	SMDiamond *mergeQueue[IQMAX];		// Merge priority queue
	int pqMin, pqMax;					// Min/max occupied bucket

	int *dmndIS;						// Packed diamond index and side (per tri chunk)

	int freeElements;					// Number of diamonds on the free list
	int frameCount;
	int queueCoarse;					// Coarseness limit on priority index
	int maxTriChunks;
	int freeTriCount;
	int maxTris;						// Target triangle count
	int freeTri;						// First free tri chunk

	int log2Table[256];					// Float->int log2 correction table

	float *vertTexBuffer;				// Interleaved tex/vertex render buffer

	float *levelMDSize;					// Max midpoint displacement per level (offset)
	float *levelMDAlloc;				// Actual allocation base (for delete)
	int maxLevel;

	void ShiftCoords(float *x, float *z)
	{
		*x = (*x + 1.0f) / 2.0f;
		*z = (*z + 1.0f) / 2.0f;
		*x *= size;
		*z *= size;
	}

	SMDiamond *Create(void);
	SMDiamond *GetChild(SMDiamond *dmnd, int index);
	void Lock(SMDiamond *dmnd);
	void Unlock(SMDiamond *dmnd);

	void AllocateTri(SMDiamond *dmnd, int j);
	void FreeTri(SMDiamond *dmnd, int j);
	void AddTri(SMDiamond *dmnd, int j);
	void RemoveTri(SMDiamond *dmnd, int j);

	void UpdateChildCull(SMDiamond *dmnd);
	void UpdateCull(SMDiamond *dmnd);
	void UpdatePriority(SMDiamond *dmnd);
	void Enqueue(SMDiamond *dmnd, int queueFlags, int newIndex);
	void Split(SMDiamond *dmnd);
	void Merge(SMDiamond *dmnd);

public:
	void Init(int maxLevel, int poolSize, RETRO_Camera *camera);
	void Shutdown(void);
	void Update(void);
	void Render(void);

	void SetMaxTrisPerFrame(int numTris) { maxTris = numTris; }

	RoamSplitMerge(void)
	{
		camera = NULL;
		pool = NULL;
		poolSize = 0;
		dmndIS = NULL;
		vertTexBuffer = NULL;
		levelMDSize = NULL;
		levelMDAlloc = NULL;
		maxLevel = 0;
	}
	~RoamSplitMerge(void) {}
};

#endif
