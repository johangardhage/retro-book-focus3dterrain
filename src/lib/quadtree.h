//
// Focus on 3D Terrain Programming
//
// Quadtree terrain implementation: a top-down quadtree LOD scheme (after
// Roettger). Supports roughness propagation (more triangles in rough areas)
// and optional view-frustum culling of nodes.
//
// Original coders: Trent Polack (trent@voxelsoft.com)
//
#ifndef _QUADTREE_H_
#define _QUADTREE_H_

#include "retrocamera.h"
#include "terrain.h"

// Quadtree child node ids
#define QT_LR_NODE 0
#define QT_LL_NODE 1
#define QT_UL_NODE 2
#define QT_UR_NODE 3

// Fan bit codes
#define QT_COMPLETE_FAN 0
#define QT_LL_UR        5
#define QT_LR_UL        10
#define QT_NO_FAN       15

class QuadTree : public Terrain
{
private:
	unsigned char *quadMatrix;	// The quadtree matrix (the engine's core)
	RETRO_Camera *camera;				// Camera used during Update

	float detailLevel;			// Desired resolution / detail level
	float minResolution;		// Minimum global resolution

	bool useRoughness;			// Seed the matrix with terrain roughness?
	bool cullNodes;				// Frustum-cull nodes during refinement?

	void PropagateRoughness(void);
	void RefineNode(float x, float z, int edgeLength);
	void RenderNode(float x, float z, int edgeLength, float texRepeat);
	void RenderVertex(float x, float z, float u, float v);

	int GetMatrixIndex(int x, int z) { return ((z * size) + x); }

public:
	bool Init(void);
	void Shutdown(void);
	void Update(RETRO_Camera *cam, bool cull = false);
	void Render(void);

	void SetDetailLevel(float detail) { detailLevel = detail; }
	void SetMinResolution(float res) { minResolution = res; }
	void SetUseRoughness(bool use) { useRoughness = use; }

	unsigned char GetQuadMatrixData(int x, int z) { return quadMatrix[(z * size) + x]; }

	QuadTree(void)
	{
		quadMatrix = NULL;
		camera = NULL;
		detailLevel = 50.0f;
		minResolution = 10.0f;
		useRoughness = true;
		cullNodes = false;
	}
	~QuadTree(void) {}
};

#endif
