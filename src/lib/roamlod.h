//
// Focus on 3D Terrain Programming
//
// Simple ROAM (Real-time Optimally Adapting Meshes) LOD implementation: a
// fractal terrain produced by recursive triangle subdivision, drawn with a
// grid texture so the adapting mesh is visible.
//
// Original coders: Trent Polack (trent@voxelsoft.com) and Mark Duchaineau
//
#ifndef _ROAMLOD_H_
#define _ROAMLOD_H_

#include "retrocamera.h"
#include "terrain.h"

class RoamLOD : public Terrain
{
private:
	RETRO_Camera *camera;

	float *levelMDSize;		// Max midpoint displacement per level
	int maxLevel;

	unsigned int gridTexID;		// Grid texture id from glGenTextures
	float gridTexCoords[3][2];	// Texture coordinates for the three verts

	void RenderSub(int level, float *vert1, float *vert2, float *vert3);

public:
	void Init(int maxLevel, RETRO_Camera *camera);
	void Shutdown(void);
	void Render(void);

	RoamLOD(void)
	{
		camera = NULL;
		levelMDSize = NULL;
		maxLevel = 0;
		gridTexID = 0;
	}
	~RoamLOD(void) {}
};

#endif
