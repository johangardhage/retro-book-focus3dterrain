//
// Focus on 3D Terrain Programming
//
// A realistic water system: a grid mesh animated with a simple force/velocity
// water simulation, rendered with a sphere-mapped reflection map.
//
// Original coders: Trent Polack (trent@voxelsoft.com)
//
#ifndef _WATERSIM_H_
#define _WATERSIM_H_

#define WATER_RESOLUTION 60

class WaterSim
{
private:
	float vertArray[WATER_RESOLUTION * WATER_RESOLUTION][3];
	float normalArray[WATER_RESOLUTION * WATER_RESOLUTION][3];
	float forceArray[WATER_RESOLUTION * WATER_RESOLUTION];
	float velArray[WATER_RESOLUTION * WATER_RESOLUTION];
	int polyIndexArray[(WATER_RESOLUTION - 1) * (WATER_RESOLUTION - 1) * 6];

	int numVertices;
	int numIndices;

	float worldSize;

	float color[3];
	float transparency;

	unsigned int refmapID;

public:
	void Init(float worldSize);
	void Update(float delta);
	void CalcNormals(void);
	void Render(bool useCVA);
	void LoadReflectionMap(const char *filename);

	void SetColor(float red, float green, float blue, float alpha)
	{
		color[0] = red;
		color[1] = green;
		color[2] = blue;
		transparency = alpha;
	}

	int GetNumVertices(void) { return numVertices; }
	int GetNumTriangles(void) { return numVertices * 3; }

	WaterSim(void)
	{
		color[0] = color[1] = color[2] = 1.0f;
		transparency = 1.0f;
		refmapID = 0;
		numVertices = 0;
		numIndices = 0;
		worldSize = 0.0f;
	}
	~WaterSim(void) {}
};

#endif
