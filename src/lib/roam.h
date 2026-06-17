//
// Focus on 3D Terrain Programming - demo 1_1
//
// Real-time Optimally Adapting Meshes.
//
// Original coders: Trent Polack (trent@voxelsoft.com) and Mark Duchaineau
//
#ifndef _ROAM_H_
#define _ROAM_H_

#define LEVEL_MAX 30

class ROAM
{
private:
	float cameraEye[3];			// Camera eye position

	float *level2dzsize;		// Max midpoint displacement per level

	unsigned int gridTexID;		// Id from glGenTextures
	float gridtex_t[3][2];		// Texture coordinates for three verts

	int vertsPerFrame;			// Stat variables
	int trisPerFrame;

	void RenderSub(int level, float *vert1, float *vert2, float *vert3);

public:
	void Initialize(void);
	void Shutdown(void);
	void Update(float *eye);
	void Render(void);

	int GetNumVertsPerFrame(void) { return vertsPerFrame; }
	int GetNumTrisPerFrame(void) { return trisPerFrame; }

	ROAM() {}
	~ROAM() {}
};

#endif
