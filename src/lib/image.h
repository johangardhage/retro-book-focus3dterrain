//
// Focus on 3D Terrain Programming
//
// Image/texture loading (TGA), and OpenGL texture creation.
//
// Original coders: Trent Polack (trent@voxelsoft.com) and Evan Pipho
//
#ifndef _IMAGE_H_
#define _IMAGE_H_

#include <stdlib.h> // NULL

class Image
{
private:
	unsigned char *data;
	unsigned int width;
	unsigned int height;
	unsigned int bpp;
	unsigned int id;

	bool loaded;

	bool LoadUncompressedTGA(unsigned char *raw, int rawSize);
	bool LoadCompressedTGA(unsigned char *raw, int rawSize);

public:
	bool Create(unsigned int width, unsigned int height, unsigned int bpp);

	// Load only the pixel data (do not create an OpenGL texture)
	bool LoadData(const char *filename);

	// Load the data and create an OpenGL texture
	bool Load(const char *filename, float minFilter, float maxFilter, bool mipmap = false);
	void Unload(void);

	void GetColor(unsigned int x, unsigned int y, unsigned char *red, unsigned char *green, unsigned char *blue)
	{
		unsigned int bytes = bpp / 8;
		if ((x < width) && (y < height)) {
			*red = data[((y * width) + x) * bytes];
			*green = data[((y * width) + x) * bytes + 1];
			*blue = data[((y * width) + x) * bytes + 2];
		}
	}

	void SetColor(unsigned int x, unsigned int y, unsigned char red, unsigned char green, unsigned char blue)
	{
		unsigned int bytes = bpp / 8;
		if ((x < width) && (y < height)) {
			data[((y * width) + x) * bytes] = red;
			data[((y * width) + x) * bytes + 1] = green;
			data[((y * width) + x) * bytes + 2] = blue;
		}
	}

	unsigned char *GetData(void) { return data; }
	unsigned int GetWidth(void) { return width; }
	unsigned int GetHeight(void) { return height; }
	unsigned int GetBPP(void) { return bpp; }
	unsigned int GetID(void) { return id; }
	void SetID(unsigned int newID) { id = newID; }
	bool IsLoaded(void) { return loaded; }

	Image(void) { data = NULL; width = 0; height = 0; bpp = 0; id = 0; loaded = false; }
	~Image(void) {}
};

#endif
