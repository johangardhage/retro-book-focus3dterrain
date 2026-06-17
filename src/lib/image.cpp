//
// Focus on 3D Terrain Programming
//
// Image/texture loading (TGA), and OpenGL texture creation. Supports
// uncompressed and RLE-compressed 24/32-bit Targa files.
//
// Original coders: Trent Polack (trent@voxelsoft.com) and Evan Pipho
//
#include <stdio.h> // FILE
#include <string.h> // memcpy, memcmp
#include "retrogl.h"
#include "image.h"

static unsigned char uncompressedTGAHeader[12] = { 0, 0, 2,  0, 0, 0, 0, 0, 0, 0, 0, 0 };
static unsigned char compressedTGAHeader[12]   = { 0, 0, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

//
// Create space for use with a new texture
//
bool Image::Create(unsigned int newWidth, unsigned int newHeight, unsigned int newBPP)
{
	width = newWidth;
	height = newHeight;
	bpp = newBPP;

	data = new unsigned char[width * height * (bpp / 8)];
	if (!data) {
		printf("[ERROR] Image::Create() Out of memory for image allocation\n");
		return false;
	}

	loaded = true;
	return true;
}

//
// Load only the data for a new image (do not create an OpenGL texture)
//
bool Image::LoadData(const char *filename)
{
	// Open the file for reading (in binary mode)
	FILE *file = fopen(filename, "rb");
	if (file == NULL) {
		printf("[ERROR] Image::LoadData() Could not open file %s\n", filename);
		return false;
	}

	// Get file length
	fseek(file, 0, SEEK_END);
	int rawSize = ftell(file);
	fseek(file, 0, SEEK_SET);

	// Read the entire file into a temporary buffer
	unsigned char *raw = new unsigned char[rawSize];
	if (fread(raw, sizeof(unsigned char), rawSize, file) != (unsigned)rawSize) {
		printf("[ERROR] Image::LoadData() %s is corrupted\n", filename);
		delete[] raw;
		fclose(file);
		return false;
	}
	fclose(file);

	bool result = false;

	// Detect the file format from the header
	if (memcmp(raw, uncompressedTGAHeader, 12) == 0) {
		result = LoadUncompressedTGA(raw, rawSize);
	} else if (memcmp(raw, compressedTGAHeader, 12) == 0) {
		result = LoadCompressedTGA(raw, rawSize);
	} else {
		printf("[ERROR] Image::LoadData() %s is not a supported image type\n", filename);
	}

	delete[] raw;

	if (result) {
		loaded = true;
	}
	return result;
}

//
// Completely setup a new texture: load the data and create an OpenGL texture
//
bool Image::Load(const char *filename, float minFilter, float maxFilter, bool mipmap)
{
	// Load the file's data in
	if (!LoadData(filename)) {
		return false;
	}

	// Set the image's OpenGL BPP type
	int type = (bpp == 24) ? GL_RGB : GL_RGBA;

	// Build the texture for use with OpenGL
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, maxFilter);

	if (!mipmap) {
		glTexImage2D(GL_TEXTURE_2D, 0, type, width, height, 0, type, GL_UNSIGNED_BYTE, data);
	} else {
		gluBuild2DMipmaps(GL_TEXTURE_2D, type, width, height, type, GL_UNSIGNED_BYTE, data);
	}

	loaded = true;
	return true;
}

//
// Unload the texture that is currently loaded
//
void Image::Unload(void)
{
	if (loaded) {
		delete[] data;
		data = NULL;
		width = 0;
		height = 0;
		bpp = 0;
		loaded = false;
	}
}

//
// Load an uncompressed Targa from the raw file buffer
//
bool Image::LoadUncompressedTGA(unsigned char *raw, int rawSize)
{
	// The 6-byte info header follows the 12-byte id header
	unsigned char *header = raw + 12;

	unsigned int newWidth = header[1] * 256 + header[0];
	unsigned int newHeight = header[3] * 256 + header[2];
	unsigned int newBPP = header[4];

	if ((newWidth <= 0) || (newHeight <= 0) || ((newBPP != 24) && (newBPP != 32))) {
		return false;
	}

	Create(newWidth, newHeight, newBPP);

	unsigned int bytesPerPixel = bpp / 8;
	unsigned int imageSize = bytesPerPixel * width * height;

	// Pixel data starts after the 18-byte header
	if (rawSize < (int)(18 + imageSize)) {
		return false;
	}
	memcpy(data, raw + 18, imageSize);

	// Swap the R and B bytes (Targa stores BGR)
	for (unsigned int i = 0; i < imageSize; i += bytesPerPixel) {
		unsigned char temp = data[i];
		data[i] = data[i + 2];
		data[i + 2] = temp;
	}

	return true;
}

//
// Load an RLE-compressed Targa from the raw file buffer
//
bool Image::LoadCompressedTGA(unsigned char *raw, int rawSize)
{
	// The 6-byte info header follows the 12-byte id header
	unsigned char *header = raw + 12;

	unsigned int newWidth = header[1] * 256 + header[0];
	unsigned int newHeight = header[3] * 256 + header[2];
	unsigned int newBPP = header[4];

	if ((newWidth <= 0) || (newHeight <= 0) || ((newBPP != 24) && (newBPP != 32))) {
		return false;
	}

	Create(newWidth, newHeight, newBPP);

	unsigned int bytesPerPixel = bpp / 8;
	unsigned int pixelCount = width * height;
	unsigned int currentPixel = 0;
	unsigned int currentByte = 0;

	// Pixel data starts after the 18-byte header
	unsigned char *file = raw + 18;
	unsigned char *end = raw + rawSize;

	do {
		if (file >= end) {
			return false;
		}

		unsigned char chunkHeader = *file;
		file++;

		if (chunkHeader < 128) {
			// RAW chunk: chunkHeader+1 literal pixels
			chunkHeader++;
			for (int i = 0; i < chunkHeader; i++) {
				if (file + bytesPerPixel > end || currentPixel >= pixelCount) {
					return false;
				}
				// Swap R and B while copying
				data[currentByte] = file[2];
				data[currentByte + 1] = file[1];
				data[currentByte + 2] = file[0];
				if (bytesPerPixel == 4) {
					data[currentByte + 3] = file[3];
				}
				file += bytesPerPixel;
				currentByte += bytesPerPixel;
				currentPixel++;
			}
		} else {
			// RLE chunk: one pixel repeated chunkHeader-127 times
			chunkHeader -= 127;
			if (file + bytesPerPixel > end) {
				return false;
			}
			unsigned char color[4];
			memcpy(color, file, bytesPerPixel);
			file += bytesPerPixel;

			for (int i = 0; i < chunkHeader; i++) {
				if (currentPixel >= pixelCount) {
					return false;
				}
				data[currentByte] = color[2];
				data[currentByte + 1] = color[1];
				data[currentByte + 2] = color[0];
				if (bytesPerPixel == 4) {
					data[currentByte + 3] = color[3];
				}
				currentByte += bytesPerPixel;
				currentPixel++;
			}
		}
	} while (currentPixel < pixelCount);

	return true;
}
