//
// Focus on 3D Terrain Programming
//
// A realistic water system (force/velocity simulation + reflection mapping).
//
// Original coders: Trent Polack (trent@voxelsoft.com)
//
#include <math.h> // sqrt
#include <stdlib.h> // rand
#include <string.h> // memset
#include "retrogl.h"
#include "image.h"
#include "watersim.h"

#define WR WATER_RESOLUTION

//
// Initialize the water mesh
//
void WaterSim::Init(float ws)
{
	worldSize = ws;

	numVertices = WR * WR;
	numIndices = (WR - 1) * (WR - 1) * 6;

	// Vertex spacing
	float dx = worldSize / (WR - 1);
	float dz = worldSize / (WR - 1);

	for (int j = 0; j < WR; j++) {
		for (int k = 0; k < WR; k++) {
			vertArray[(j * WR) + k][0] = -1.0f + dx * k;
			vertArray[(j * WR) + k][1] = 0.0f;
			vertArray[(j * WR) + k][2] = -1.0f + dz * j;
		}
	}

	// Polygon indices
	int x = 0;
	int z = WR;
	int *indexPtr = polyIndexArray;
	for (int j = 0; j < WR - 1; j++) {
		for (int k = 0; k < WR - 1; k++) {
			indexPtr[0] = x;
			indexPtr[1] = x + 1;
			indexPtr[2] = z;
			indexPtr[3] = z;
			indexPtr[4] = x + 1;
			indexPtr[5] = z + 1;

			indexPtr += 6;
			x++;
			z++;
		}
		x++;
		z++;
	}

	// The force and velocity fields start at rest
	memset(forceArray, 0, sizeof(forceArray));
	memset(velArray, 0, sizeof(velArray));

	// Start a water ripple at a random spot in the field
	vertArray[rand() % (WR * WR)][1] = 200.0f;
}

//
// Update the vertices of the water mesh
//
void WaterSim::Update(float delta)
{
	// Accumulate the forces acting on each interior vertex from its 8 neighbors
	for (int z = 1; z < WR - 1; z++) {
		for (int x = 1; x < WR - 1; x++) {
			float tempF = forceArray[(z * WR) + x];
			float vert = vertArray[(z * WR) + x][1];
			float d;

			// bottom
			d = vert - vertArray[((z - 1) * WR) + x][1];
			forceArray[((z - 1) * WR) + x] += d;
			tempF -= d;

			// left
			d = vert - vertArray[(z * WR) + (x - 1)][1];
			forceArray[(z * WR) + (x - 1)] += d;
			tempF -= d;

			// top
			d = vert - vertArray[((z + 1) * WR) + x][1];
			forceArray[((z + 1) * WR) + x] += d;
			tempF -= d;

			// right
			d = vert - vertArray[(z * WR) + (x + 1)][1];
			forceArray[(z * WR) + (x + 1)] += d;
			tempF -= d;

			// upper right
			d = (vert - vertArray[((z + 1) * WR) + (x + 1)][1]) * 4.94974747f;
			forceArray[((z + 1) * WR) + (x + 1)] += d;
			tempF -= d;

			// lower left
			d = (vert - vertArray[((z - 1) * WR) + (x - 1)][1]) * 4.94974747f;
			forceArray[((z - 1) * WR) + (x - 1)] += d;
			tempF -= d;

			// lower right
			d = (vert - vertArray[((z - 1) * WR) + (x + 1)][1]) * 4.94974747f;
			forceArray[((z - 1) * WR) + (x + 1)] += d;
			tempF -= d;

			// upper left
			d = (vert - vertArray[((z + 1) * WR) + (x - 1)][1]) * 4.94974747f;
			forceArray[((z + 1) * WR) + (x - 1)] += d;
			tempF -= d;

			forceArray[(z * WR) + x] = tempF;
		}
	}

	// Integrate velocity and update the vertex heights
	for (int x = 0; x < numVertices; x++) {
		velArray[x] += (forceArray[x] * delta);
		vertArray[x][1] += velArray[x];
		forceArray[x] = 0.0f;
	}
}

//
// Calculate the mesh's vertex normals
//
void WaterSim::CalcNormals(void)
{
	for (int i = 0; i < WR; i++) {
		for (int j = 0; j < WR; j++) {
			float (*vert)[3] = &vertArray[(i * WR) + j];
			float *normal = normalArray[(i * WR) + j];

			normal[0] = 0.0f;
			normal[1] = 1.0f;
			normal[2] = 0.0f;

			// above
			if (i != 0) {
				if (j != 0) {
					normal[0] += -vert[-WR - 1][1];
					normal[2] += -vert[-WR - 1][1];
				} else {
					normal[0] += -vert[-WR][1];
					normal[2] += -vert[-WR][1];
				}

				normal[0] += -vert[-WR][1] * 2.0f;

				if (j != (WR - 1)) {
					normal[0] += -vert[-WR + 1][1];
					normal[2] += vert[-WR + 1][1];
				} else {
					normal[0] += -vert[-WR][1];
					normal[2] += vert[-WR][1];
				}
			} else {
				normal[0] += -vert[0][1];
				normal[0] += -vert[0][1] * 2.0f;
				normal[0] += -vert[0][1];

				normal[2] += -vert[0][1];
				normal[2] += vert[0][1];
			}

			// current line
			if (j != 0) {
				normal[2] += -vert[-1][1] * 2.0f;
			} else {
				normal[2] += -vert[0][1] * 2.0f;
			}

			if (j != (WR - 1)) {
				normal[2] += vert[1][1] * 2.0f;
			} else {
				normal[2] += vert[0][1] * 2.0f;
			}

			// below
			if (i != (WR - 1)) {
				if (j != 0) {
					normal[0] += vert[WR - 1][1];
					normal[2] += -vert[WR - 1][1];
				} else {
					normal[0] += vert[WR][1];
					normal[2] += -vert[WR][1];
				}

				normal[0] += vert[WR][1] * 2.0f;

				if (j != WR - 1) {
					normal[0] += vert[WR + 1][1];
					normal[2] += vert[WR + 1][1];
				} else {
					normal[0] += vert[WR][1];
					normal[2] += vert[WR][1];
				}
			} else {
				normal[0] += vert[0][1];
				normal[0] += vert[0][1] * 2.0f;
				normal[0] += vert[0][1];

				normal[2] += -vert[0][1];
				normal[2] += vert[0][1];
			}

			// normalize
			float tmp = 1.0f / (float)sqrt(normal[0] * normal[0] + normal[2] * normal[2] + 1.0f);
			normal[0] *= tmp;
			normal[1] *= tmp;
			normal[2] *= tmp;
		}
	}
}

//
// Render the water mesh, using sphere-mapped reflection
//
void WaterSim::Render(bool useCVA)
{
	(void)useCVA;	// Compiled-vertex-array locking is not used in this port

	glBindTexture(GL_TEXTURE_2D, refmapID);
	glEnable(GL_TEXTURE_2D);

	// Auto-generate texture coordinates via sphere mapping
	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
	glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	glColor4f(color[0], color[1], color[2], transparency);

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 3 * sizeof(float), vertArray);

	glEnableClientState(GL_NORMAL_ARRAY);
	glNormalPointer(GL_FLOAT, 3 * sizeof(float), normalArray);

	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);
	glEnable(GL_TEXTURE_GEN_R);

	glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_INT, polyIndexArray);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);

	glDisable(GL_BLEND);

	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);
	glDisable(GL_TEXTURE_GEN_R);

	glDisable(GL_TEXTURE_2D);
}

//
// Load the water's reflection map
//
void WaterSim::LoadReflectionMap(const char *filename)
{
	Image image;
	image.Load(filename, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, true);
	refmapID = image.GetID();
}
