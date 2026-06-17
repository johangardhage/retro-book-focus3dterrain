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
// ARB multitexturing: render the color map (unit 0) and detail map (unit 1) in
// a single pass, combined with a 2x RGB scale to compensate for the detail
// map's midtone average. Matches the book's hardware multitexturing path.
//
void Terrain::BeginMultitexture(void)
{
	glDisable(GL_BLEND);

	// Color texture on the first texture unit
	glActiveTextureARB(GL_TEXTURE0_ARB);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture.GetID());

	// Detail texture on the second texture unit, modulated x2 onto the color
	glActiveTextureARB(GL_TEXTURE1_ARB);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, detailMap.GetID());
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_RGB_SCALE_ARB, 2);

	multitexturePass = true;
}

void Terrain::EndMultitexture(void)
{
	multitexturePass = false;

	// Unbind the second texture unit
	glActiveTextureARB(GL_TEXTURE1_ARB);
	glDisable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Leave the first texture unit active
	glActiveTextureARB(GL_TEXTURE0_ARB);
	glDisable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
}

//
// Emit a texture coordinate to unit 0, and (during the multitexture pass) to
// unit 1 scaled up by the detail-map repeat factor.
//
void Terrain::EmitTexCoord(float u, float v)
{
	glMultiTexCoord2fARB(GL_TEXTURE0_ARB, u, v);
	if (multitexturePass) {
		glMultiTexCoord2fARB(GL_TEXTURE1_ARB, u * repeatDetailMap, v * repeatDetailMap);
	}
}

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

//
// Load a texture that will be stretched over the landscape
//
bool Terrain::LoadTexture(const char *filename)
{
	return texture.Load(filename, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, true);
}

//
// Load a detail map to add realism to the terrain
//
bool Terrain::LoadDetailMap(const char *filename)
{
	return detailMap.Load(filename, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, true);
}

//
// Get the percentage of which a texture tile should be visible at a given height
//
float Terrain::RegionPercent(int tileType, unsigned char height)
{
	// If the height is lower than the lowest tile's height, then we want full
	// brightness; otherwise the area will get darkened, showing no texture
	if (tiles.tiles[LOWEST_TILE].IsLoaded()) {
		if (tileType == LOWEST_TILE && height < tiles.regions[LOWEST_TILE].optimalHeight) {
			return 1.0f;
		}
	} else if (tiles.tiles[LOW_TILE].IsLoaded()) {
		if (tileType == LOW_TILE && height < tiles.regions[LOW_TILE].optimalHeight) {
			return 1.0f;
		}
	} else if (tiles.tiles[HIGH_TILE].IsLoaded()) {
		if (tileType == HIGH_TILE && height < tiles.regions[HIGH_TILE].optimalHeight) {
			return 1.0f;
		}
	} else if (tiles.tiles[HIGHEST_TILE].IsLoaded()) {
		if (tileType == HIGHEST_TILE && height < tiles.regions[HIGHEST_TILE].optimalHeight) {
			return 1.0f;
		}
	}

	// Height is outside the region's boundaries
	if (height < tiles.regions[tileType].lowHeight) {
		return 0.0f;
	} else if (height > tiles.regions[tileType].highHeight) {
		return 0.0f;
	}

	// Height is below the optimum height
	if (height < tiles.regions[tileType].optimalHeight) {
		float temp1 = (float)height - tiles.regions[tileType].lowHeight;
		float temp2 = (float)tiles.regions[tileType].optimalHeight - tiles.regions[tileType].lowHeight;
		return (temp1 / temp2);
	}

	// Height is exactly the optimal height
	else if (height == tiles.regions[tileType].optimalHeight) {
		return 1.0f;
	}

	// Height is above the optimal height
	else if (height > tiles.regions[tileType].optimalHeight) {
		float temp1 = (float)tiles.regions[tileType].highHeight - tiles.regions[tileType].optimalHeight;
		return ((temp1 - (height - tiles.regions[tileType].optimalHeight)) / temp1);
	}

	return 0.0f;
}

//
// Get the (wrapped) texture coordinates for a tile of the given size
//
void Terrain::GetTexCoords(Image *texture, unsigned int *x, unsigned int *y)
{
	unsigned int width = texture->GetWidth();
	unsigned int height = texture->GetHeight();
	int repeatX = -1;
	int repeatY = -1;
	int i = 0;

	// Figure out how many times the tile has repeated (on the X axis)
	while (repeatX == -1) {
		i++;
		if (*x < (width * i)) {
			repeatX = i - 1;
		}
	}

	i = 0;

	// Figure out how many times the tile has repeated (on the Y axis)
	while (repeatY == -1) {
		i++;
		if (*y < (height * i)) {
			repeatY = i - 1;
		}
	}

	// Update the given texture coordinates
	*x = *x - (width * repeatX);
	*y = *y - (height * repeatY);
}

//
// Interpolate heights so that the generated texture map does not look blocky
//
unsigned char Terrain::InterpolateHeight(int x, int z, float heightToTexRatio)
{
	unsigned char low, highX, highZ;
	float interpolation;
	float scaledX = x * heightToTexRatio;
	float scaledZ = z * heightToTexRatio;

	// Set the middle boundary
	low = GetTrueHeightAtPoint((int)scaledX, (int)scaledZ);

	// Interpolate along the X axis
	if ((scaledX + 1) > size) {
		return low;
	} else {
		highX = GetTrueHeightAtPoint((int)scaledX + 1, (int)scaledZ);
	}
	interpolation = (scaledX - (int)scaledX);
	float interpX = ((highX - low) * interpolation) + low;

	// Interpolate along the Z axis
	if ((scaledZ + 1) > size) {
		return low;
	} else {
		highZ = GetTrueHeightAtPoint((int)scaledX, (int)scaledZ + 1);
	}
	interpolation = (scaledZ - (int)scaledZ);
	float interpZ = ((highZ - low) * interpolation) + low;

	// Average of the two interpolated values
	return ((unsigned char)((interpX + interpZ) / 2));
}

//
// Generate a texture map from the loaded tiles
//
void Terrain::GenerateTextureMap(unsigned int textureSize)
{
	unsigned char red, green, blue;

	// Find out the number of tiles that we have
	tiles.numTiles = 0;
	for (int i = 0; i < NUM_TILES; i++) {
		if (tiles.tiles[i].IsLoaded()) {
			tiles.numTiles++;
		}
	}

	// Calculate the texture regions
	int lastHeight = -1;
	for (int i = 0; i < NUM_TILES; i++) {
		if (tiles.tiles[i].IsLoaded()) {
			// Calculate the three height boundaries
			tiles.regions[i].lowHeight = lastHeight + 1;
			lastHeight += 255 / tiles.numTiles;

			tiles.regions[i].optimalHeight = lastHeight;
			tiles.regions[i].highHeight = (lastHeight - tiles.regions[i].lowHeight) + lastHeight;
		}
	}

	// Create room for a new texture
	texture.Create(textureSize, textureSize, 24);

	// Get the height-map-to-texture-map ratio
	float mapRatio = (float)size / textureSize;

	// Create the texture data
	for (unsigned int z = 0; z < textureSize; z++) {
		for (unsigned int x = 0; x < textureSize; x++) {
			float totalRed = 0.0f;
			float totalGreen = 0.0f;
			float totalBlue = 0.0f;

			// Loop through the tiles
			for (int i = 0; i < NUM_TILES; i++) {
				if (tiles.tiles[i].IsLoaded()) {
					unsigned int texX = x;
					unsigned int texZ = z;

					// Get texture coordinates
					GetTexCoords(&tiles.tiles[i], &texX, &texZ);

					// Get the current color in the tile at those coordinates
					tiles.tiles[i].GetColor(texX, texZ, &red, &green, &blue);

					// Get the blending percentage for this tile at this height
					float blend = RegionPercent(i, InterpolateHeight(x, z, mapRatio));

					// Accumulate the RGB values
					totalRed += red * blend;
					totalGreen += green * blend;
					totalBlue += blue * blend;
				}
			}

			// Set the final color in the generated texture
			texture.SetColor(x, z, (unsigned char)totalRed, (unsigned char)totalGreen, (unsigned char)totalBlue);
		}
	}

	// Build the OpenGL texture
	unsigned int tempID;
	glGenTextures(1, &tempID);
	glBindTexture(GL_TEXTURE_2D, tempID);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, textureSize, textureSize, 0, GL_RGB, GL_UNSIGNED_BYTE, texture.GetData());

	texture.SetID(tempID);
}
