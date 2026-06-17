//
// Focus on 3D Terrain Programming
//
// Abstract terrain base class. All specific implementations are derived from
// it. Contains heightmap loading/saving, fractal terrain generation, texture
// mapping, texture-map generation from tiles, and detail mapping.
//
// Original coders: Trent Polack (trent@voxelsoft.com)
//
#ifndef _TERRAIN_H_
#define _TERRAIN_H_

#include <stdlib.h> // rand, RAND_MAX
#include "image.h"

#define NUM_TILES 5

enum TileType
{
	LOWEST_TILE = 0,	// Sand, dirt, etc.
	LOW_TILE,			// Grass
	HIGH_TILE,			// Mountainside
	HIGHEST_TILE		// Tip of mountain
};

enum LightingType
{
	LIGHTING_NONE = 0,	// No lighting (full brightness)
	HEIGHT_BASED,		// Brightness from the height value
	LIGHTMAP,			// Brightness from a loaded lightmap
	SLOPE_LIGHT			// Brightness computed from terrain slope
};

struct HeightData
{
	unsigned char *data;	// The height data
	int size;				// The height size (must be a power of 2)
};

struct LightmapData
{
	unsigned char *data;	// The lightmap (brightness) data
	int size;				// The lightmap size
};

struct TextureRegion
{
	int lowHeight;			// Lowest possible height (0%)
	int optimalHeight;		// Optimal height (100%)
	int highHeight;			// Highest possible height (0%)
};

struct TextureTiles
{
	TextureRegion regions[NUM_TILES];	// Texture regions
	Image tiles[NUM_TILES];				// Texture tiles
	int numTiles;
};

class Terrain
{
protected:
	HeightData heightData;	// The height data

	float scale[3];			// Scaling vector (width, height, depth)

	// Texture information
	TextureTiles tiles;
	Image texture;
	Image detailMap;
	int repeatDetailMap;
	bool multitexture;
	bool textureMapping;
	bool detailMapping;
	bool multitexturePass;	// True only while emitting the single ARB multitexture pass

	// Lighting information
	LightingType lightingType;
	LightmapData lightmap;
	float lightColor[3];
	float minBrightness, maxBrightness;
	float lightSoftness;
	int directionX, directionZ;

	int vertsPerFrame;		// Stat variables
	int trisPerFrame;

	// Fractal terrain generation helpers
	void NormalizeTerrain(float *heightData);
	void FilterHeightBand(float *band, int stride, int count, float filter);
	void FilterHeightField(float *heightData, float filter);

	// Texture map generation helpers
	float RegionPercent(int tileType, unsigned char height);
	void GetTexCoords(Image *texture, unsigned int *x, unsigned int *y);
	unsigned char InterpolateHeight(int x, int z, float heightToTexRatio);

	// ARB multitexture rendering helpers (color map on unit 0, detail on unit 1)
	void BeginMultitexture(void);
	void EndMultitexture(void);
	void EmitTexCoord(float u, float v);

public:
	int size;	// The size of the heightmap, must be a power of two

	virtual void Render(void) = 0;

	bool LoadHeightMap(const char *filename, int size);
	bool SaveHeightMap(const char *filename);
	bool UnloadHeightMap(void);

	bool MakeTerrainFault(int size, int iterations, int minDelta, int maxDelta, float filter);
	bool MakeTerrainPlasma(int size, float roughness);

	// Texture mapping
	bool LoadTexture(const char *filename);
	void UnloadTexture(void) { texture.Unload(); }
	void DoTextureMapping(bool doIt) { textureMapping = doIt; }

	// Texture map generation from tiles
	void GenerateTextureMap(unsigned int size);
	bool LoadTile(TileType tileType, const char *filename) { return tiles.tiles[tileType].LoadData(filename); }
	void UnloadTile(TileType tileType) { tiles.tiles[tileType].Unload(); }
	void UnloadAllTiles(void)
	{
		UnloadTile(LOWEST_TILE);
		UnloadTile(LOW_TILE);
		UnloadTile(HIGH_TILE);
		UnloadTile(HIGHEST_TILE);
	}

	// Detail mapping
	bool LoadDetailMap(const char *filename);
	void UnloadDetailMap(void) { detailMap.Unload(); }
	void DoDetailMapping(bool doIt, int repeatNum = 0)
	{
		detailMapping = doIt;
		repeatDetailMap = repeatNum;
	}
	void DoMultitexturing(bool doIt) { multitexture = doIt; }

	// Lighting
	void SetLightingType(LightingType type) { lightingType = type; }
	void SetLightColor(float r, float g, float b)
	{
		lightColor[0] = r;
		lightColor[1] = g;
		lightColor[2] = b;
	}
	float *GetLightColor(void) { return lightColor; }
	void CustomizeSlopeLighting(int dirX, int dirZ, float minBright, float maxBright, float softness)
	{
		directionX = dirX;
		directionZ = dirZ;
		minBrightness = minBright;
		maxBrightness = maxBright;
		lightSoftness = softness;
	}

	bool LoadLightMap(const char *filename, int size);
	bool UnloadLightMap(void);
	void CalculateLighting(void);

	void SetBrightnessAtPoint(int x, int z, unsigned char brightness)
	{
		lightmap.data[(z * lightmap.size) + x] = brightness;
	}

	// Brightness used by the renderer, depending on the lighting technique
	unsigned char GetBrightnessAtPoint(int x, int z)
	{
		switch (lightingType) {
		case HEIGHT_BASED:
			return GetTrueHeightAtPoint(x, z);
		case LIGHTMAP:
		case SLOPE_LIGHT:
			return lightmap.data ? lightmap.data[(z * lightmap.size) + x] : 255;
		default:
			return 255;
		}
	}

	int GetNumVertsPerFrame(void) { return vertsPerFrame; }
	int GetNumTrisPerFrame(void) { return trisPerFrame; }

	// Get a random value between the two arguments
	float RangedRandom(float f1, float f2)
	{
		return (f1 + (f2 - f1) * ((float)rand()) / ((float)RAND_MAX));
	}

	// Set just the height (Y) scale
	void SetHeightScale(float s) { scale[1] = s; }

	// Set the width/height/depth scale of the terrain
	void Scale(float x, float y, float z)
	{
		scale[0] = x;
		scale[1] = y;
		scale[2] = z;
	}
	float *GetScale(void) { return scale; }

	void SetHeightAtPoint(unsigned char height, int x, int z)
	{
		heightData.data[(z * size) + x] = height;
	}

	unsigned char GetTrueHeightAtPoint(int x, int z)
	{
		return (heightData.data[(z * size) + x]);
	}

	float GetScaledHeightAtPoint(int x, int z)
	{
		return ((float)(heightData.data[(z * size) + x]) * scale[1]);
	}

	Terrain(void)
	{
		heightData.data = NULL;
		size = 0;
		scale[0] = 1.0f;
		scale[1] = 1.0f;
		scale[2] = 1.0f;
		textureMapping = false;
		detailMapping = false;
		multitexture = false;
		multitexturePass = false;
		repeatDetailMap = 0;
		lightingType = LIGHTING_NONE;
		lightmap.data = NULL;
		lightmap.size = 0;
		lightColor[0] = 1.0f;
		lightColor[1] = 1.0f;
		lightColor[2] = 1.0f;
		minBrightness = 0.0f;
		maxBrightness = 1.0f;
		lightSoftness = 1.0f;
		directionX = 0;
		directionZ = 0;
	}
	~Terrain(void) {}
};

#endif
