//
// Focus on 3D Terrain Programming
//
// Geomipmapping terrain implementation: the terrain is split into square
// patches, each rendered with a distance-based level of detail (as triangle
// fans), with optional view-frustum culling of patches.
//
// Original coders: Trent Polack (trent@voxelsoft.com)
//
#ifndef _GEOMIPMAPPING_H_
#define _GEOMIPMAPPING_H_

#include "retrocamera.h"
#include "terrain.h"

struct GeoMMPatch
{
	float distance;		// Distance from the camera
	int LOD;			// Level of detail
	bool visible;		// Is the patch in the view frustum?
};

struct GeoMMNeighbor
{
	bool left;
	bool up;
	bool right;
	bool down;
};

class GeoMipMapping : public Terrain
{
private:
	GeoMMPatch *patches;
	int patchSize;
	int numPatchesPerSide;
	int maxLOD;
	int patchesPerFrame;

	void RenderVertex(float x, float z, float u, float v);
	void RenderFan(float cX, float cZ, float size, GeoMMNeighbor neighbor, float texRepeat);
	void RenderPatch(int PX, int PZ, float texRepeat);
	void RenderAllPatches(float texRepeat);

public:
	bool Init(int patchSize);
	void Shutdown(void);
	void Update(RETRO_Camera *camera, bool cullPatches = false);
	void Render(void);

	int GetNumPatchesPerFrame(void) { return patchesPerFrame; }
	int GetPatchNumber(int PX, int PZ) { return ((PZ * numPatchesPerSide) + PX); }

	GeoMipMapping(void)
	{
		patches = NULL;
		patchSize = 0;
		numPatchesPerSide = 0;
		maxLOD = 0;
		patchesPerFrame = 0;
	}
	~GeoMipMapping(void) {}
};

#endif
