//
// Focus on 3D Terrain Programming
//
// ROAM with frustum culling: recursive triangle subdivision over a real
// heightmap, with each triangle's bounding sphere tested against the view
// frustum so out-of-view branches are skipped.
//
// Original coders: Trent Polack (trent@voxelsoft.com) and Mark Duchaineau
//
#ifndef _ROAMFRUSTUM_H_
#define _ROAMFRUSTUM_H_

#include "retrocamera.h"
#include "terrain.h"

// Frustum bitmasks
#define CULL_ALLIN 0x3f
#define CULL_OUT   0x40

class RoamFrustum : public Terrain
{
private:
	RETRO_Camera *camera;

	float *levelMDSize;		// Max midpoint displacement per level
	int maxLevel;

	void RenderSub(int level, float *vert1, float *vert2, float *vert3, int cull, float texRepeat);

public:
	void Init(int maxLevel, RETRO_Camera *camera);
	void Shutdown(void);
	void Render(void);

	RoamFrustum(void)
	{
		camera = NULL;
		levelMDSize = NULL;
		maxLevel = 0;
	}
	~RoamFrustum(void) {}
};

#endif
