//
// Focus on 3D Terrain Programming
//
// Abstract terrain base class: heightmap loading/saving and fractal terrain
// generation (fault formation and midpoint displacement).
//
// Original coders: Trent Polack (trent@voxelsoft.com). Fractal generation
// thanks to Jason Shankel.
//
#include <stdio.h> // FILE
#include <math.h> // pow
#include "retrogl.h"
#include "terrain.h"

//
// Load a grayscale RAW height map
//
bool Terrain::LoadHeightMap(const char *filename, int mapSize)
{
	// Check to see if the data has been set
	if (heightData.data) {
		UnloadHeightMap();
	}

	// Open the RAW height map dataset
	FILE *file = fopen(filename, "rb");
	if (file == NULL) {
		printf("[ERROR] Terrain::LoadHeightMap() Could not load %s\n", filename);
		return false;
	}

	// Allocate the memory for our height data
	heightData.data = new unsigned char[mapSize * mapSize];

	// Read the heightmap into context
	fread(heightData.data, 1, mapSize * mapSize, file);

	// Close the file
	fclose(file);

	// Set the size data
	size = mapSize;

	return true;
}

//
// Save a grayscale RAW height map
//
bool Terrain::SaveHeightMap(const char *filename)
{
	// Open a file to write to
	FILE *file = fopen(filename, "wb");
	if (file == NULL) {
		printf("[ERROR] Terrain::SaveHeightMap() Could not create %s\n", filename);
		return false;
	}

	// Check to see if our height map actually has data in it
	if (heightData.data == NULL) {
		printf("[ERROR] Terrain::SaveHeightMap() The height data buffer for %s is empty\n", filename);
		fclose(file);
		return false;
	}

	// Write the data to the file
	fwrite(heightData.data, 1, size * size, file);

	// Close the file
	fclose(file);

	return true;
}

//
// Unload the class's height map (if there is one)
//
bool Terrain::UnloadHeightMap(void)
{
	// Check to see if the data has been set
	if (heightData.data) {
		// Delete the data
		delete[] heightData.data;
		heightData.data = NULL;

		// Reset the map dimensions also
		size = 0;
	}

	return true;
}

//
// Scale the terrain height values to a range of 0-255
//
void Terrain::NormalizeTerrain(float *heightData)
{
	float min = heightData[0];
	float max = heightData[0];

	// Find the min/max values of the height buffer
	for (int i = 1; i < size * size; i++) {
		if (heightData[i] > max) {
			max = heightData[i];
		} else if (heightData[i] < min) {
			min = heightData[i];
		}
	}

	// Find the range of the altitude
	if (max <= min) {
		return;
	}

	float height = max - min;

	// Scale the values to a range of 0-255
	for (int i = 0; i < size * size; i++) {
		heightData[i] = ((heightData[i] - min) / height) * 255.0f;
	}
}

//
// Apply the erosion filter to an individual band of height values
//
void Terrain::FilterHeightBand(float *band, int stride, int count, float filter)
{
	float v = band[0];
	int j = stride;

	// Go through the height band and apply the erosion filter
	for (int i = 0; i < count - 1; i++) {
		band[j] = filter * v + (1 - filter) * band[j];

		v = band[j];
		j += stride;
	}
}

//
// Apply the erosion filter to an entire buffer of height values
//
void Terrain::FilterHeightField(float *heightData, float filter)
{
	// Erode left to right
	for (int i = 0; i < size; i++) {
		FilterHeightBand(&heightData[size * i], 1, size, filter);
	}

	// Erode right to left
	for (int i = 0; i < size; i++) {
		FilterHeightBand(&heightData[size * i + size - 1], -1, size, filter);
	}

	// Erode top to bottom
	for (int i = 0; i < size; i++) {
		FilterHeightBand(&heightData[i], size, size, filter);
	}

	// Erode from bottom to top
	for (int i = 0; i < size; i++) {
		FilterHeightBand(&heightData[size * (size - 1) + i], -size, size, filter);
	}
}

//
// Create a height data set using the "Fault Formation" algorithm
//
bool Terrain::MakeTerrainFault(int mapSize, int iterations, int minDelta, int maxDelta, float filter)
{
	if (heightData.data) {
		UnloadHeightMap();
	}

	size = mapSize;

	// Allocate the memory for our height data
	heightData.data = new unsigned char[size * size];
	float *tempBuffer = new float[size * size];

	// Clear the temp buffer
	for (int i = 0; i < size * size; i++) {
		tempBuffer[i] = 0;
	}

	for (int currentIteration = 0; currentIteration < iterations; currentIteration++) {
		// Calculate the height range (linear interpolation from maxDelta to
		// minDelta) for this fault-pass
		int height = maxDelta - ((maxDelta - minDelta) * currentIteration) / iterations;

		// Pick two points at random from the entire height map
		int randX1 = rand() % size;
		int randZ1 = rand() % size;

		// Check to make sure that the points are not the same
		int randX2, randZ2;
		do {
			randX2 = rand() % size;
			randZ2 = rand() % size;
		} while (randX2 == randX1 && randZ2 == randZ1);

		// dirX1, dirZ1 is a vector going the same direction as the line
		int dirX1 = randX2 - randX1;
		int dirZ1 = randZ2 - randZ1;

		for (int z = 0; z < size; z++) {
			for (int x = 0; x < size; x++) {
				// dirX2, dirZ2 is a vector from randX1, randZ1 to the current point
				int dirX2 = x - randX1;
				int dirZ2 = z - randZ1;

				// If the result of the cross product is "up" (above 0),
				// then raise this point by height
				if ((dirX2 * dirZ1 - dirX1 * dirZ2) > 0) {
					tempBuffer[(z * size) + x] += (float)height;
				}
			}
		}

		// Erode terrain
		FilterHeightField(tempBuffer, filter);
	}

	// Normalize the terrain for our purposes
	NormalizeTerrain(tempBuffer);

	// Transfer the terrain into our class's unsigned char height buffer
	for (int z = 0; z < size; z++) {
		for (int x = 0; x < size; x++) {
			SetHeightAtPoint((unsigned char)tempBuffer[(z * size) + x], x, z);
		}
	}

	// Delete temporary buffer
	delete[] tempBuffer;

	return true;
}

//
// Create a height data set using the "Midpoint Displacement" algorithm.
// Note: this algorithm can only generate (n^2) x (n^2) maps.
//
bool Terrain::MakeTerrainPlasma(int mapSize, float roughness)
{
	int rectSize = mapSize;

	if (heightData.data) {
		UnloadHeightMap();
	}

	if (roughness < 0) {
		roughness *= -1;
	}

	float height = (float)rectSize / 2;
	float heightReducer = (float)pow(2, -1 * roughness);

	size = mapSize;

	// Allocate the memory for our height data
	heightData.data = new unsigned char[size * size];
	float *tempBuffer = new float[size * size];

	// Set the first value in the height field
	tempBuffer[0] = 0.0f;

	// Begin the displacement process
	while (rectSize > 0) {
		// Diamond step: find the values at the center of the rectangles by
		// averaging the values at the corners and adding a random offset
		for (int i = 0; i < size; i += rectSize) {
			for (int j = 0; j < size; j += rectSize) {
				int ni = (i + rectSize) % size;
				int nj = (j + rectSize) % size;

				int mi = (i + rectSize / 2);
				int mj = (j + rectSize / 2);

				tempBuffer[mi + mj * size] = (float)((tempBuffer[i + j * size] + tempBuffer[ni + j * size] + tempBuffer[i + nj * size] + tempBuffer[ni + nj * size]) / 4 + RangedRandom(-height / 2, height / 2));
			}
		}

		// Square step: find the values on the left and top sides of each
		// rectangle. The height map wraps, so we're never left hanging.
		for (int i = 0; i < size; i += rectSize) {
			for (int j = 0; j < size; j += rectSize) {
				int ni = (i + rectSize) % size;
				int nj = (j + rectSize) % size;

				int mi = (i + rectSize / 2);
				int mj = (j + rectSize / 2);

				int pmi = (i - rectSize / 2 + size) % size;
				int pmj = (j - rectSize / 2 + size) % size;

				// Calculate the square value for the top side of the rectangle
				tempBuffer[mi + j * size] = (float)((tempBuffer[i + j * size] +
					tempBuffer[ni + j * size] +
					tempBuffer[mi + pmj * size] +
					tempBuffer[mi + mj * size]) / 4 +
					RangedRandom(-height / 2, height / 2));

				// Calculate the square value for the left side of the rectangle
				tempBuffer[i + mj * size] = (float)((tempBuffer[i + j * size] +
					tempBuffer[i + nj * size] +
					tempBuffer[pmi + mj * size] +
					tempBuffer[mi + mj * size]) / 4 +
					RangedRandom(-height / 2, height / 2));
			}
		}

		// Reduce the rectangle size by two to prepare for the next pass
		rectSize /= 2;

		// Reduce the height by the height reducer
		height *= heightReducer;
	}

	// Normalize the terrain for our purposes
	NormalizeTerrain(tempBuffer);

	// Transfer the terrain into our class's unsigned char height buffer
	for (int z = 0; z < size; z++) {
		for (int x = 0; x < size; x++) {
			SetHeightAtPoint((unsigned char)tempBuffer[(z * size) + x], x, z);
		}
	}

	// Delete temporary buffer
	delete[] tempBuffer;

	return true;
}
