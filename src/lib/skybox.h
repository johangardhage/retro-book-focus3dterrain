//
// Focus on 3D Terrain Programming
//
// A skybox: a textured cube centered on the camera.
//
// Original coders: Trent Polack (trent@voxelsoft.com)
//
#ifndef _SKYBOX_H_
#define _SKYBOX_H_

#include "image.h"

#define SBX_NUMSIDES 6
#define SBX_FRONT  0
#define SBX_BACK   1
#define SBX_RIGHT  2
#define SBX_LEFT   3
#define SBX_TOP    4
#define SBX_BOTTOM 5

class Skybox
{
private:
	Image textures[SBX_NUMSIDES];

	float vmin[3], vmax[3], center[3];

public:
	bool LoadTexture(int side, const char *filename);
	void Render(void);

	void Set(float cx, float cy, float cz, float size)
	{
		float half = size / 2;

		center[0] = cx;
		center[1] = cy;
		center[2] = cz;

		vmin[0] = vmin[1] = vmin[2] = -half;
		vmax[0] = vmax[1] = vmax[2] = half;
	}

	Skybox(void) {}
	~Skybox(void) {}
};

#endif
