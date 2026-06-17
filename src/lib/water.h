//
// Focus on 3D Terrain Programming
//
// A simple water system: a single animated, blended, textured quad.
//
// Original coders: Trent Polack (trent@voxelsoft.com)
//
#ifndef _WATER_H_
#define _WATER_H_

class Water
{
private:
	float color[3];
	float transparency;
	unsigned int texID;

public:
	void Render(float worldSize);
	void LoadTextureMaps(const char *filename);

	void SetColor(float red, float green, float blue, float alpha)
	{
		color[0] = red;
		color[1] = green;
		color[2] = blue;
		transparency = alpha;
	}

	Water(void)
	{
		color[0] = color[1] = color[2] = 1.0f;
		transparency = 1.0f;
		texID = 0;
	}
	~Water(void) {}
};

#endif
