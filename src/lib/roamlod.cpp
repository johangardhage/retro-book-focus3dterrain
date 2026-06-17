//
// Focus on 3D Terrain Programming
//
// Simple ROAM LOD implementation (recursive triangle subdivision).
//
// Original coders: Trent Polack (trent@voxelsoft.com) and Mark Duchaineau
//
#include <math.h> // sqrt, floor
#include <string.h> // memcpy
#include "retrogl.h"
#include "roamlod.h"
#include "randtab.h"

#define SQR(n) ((n) * (n))

//
// Initialize the ROAM engine (maxLevel is the deepest recursion level)
//
void RoamLOD::Init(int level, RETRO_Camera *cam)
{
	char gridData[128 * 128 * 4];	// Texture for showing the mesh

	maxLevel = level;
	levelMDSize = new float[maxLevel + 1];

	// Generate the table of displacement sizes versus levels (a quick hack to
	// avoid cracks, since this implementation has no crack-prevention steps)
	for (int i = 0; i <= maxLevel; i++) {
		levelMDSize[i] = 0.3f / ((float)sqrt((float)(1 << i)));
	}

	// Generate the grid texture
	for (int y = 0; y < 128; y++) {
		for (int x = 0; x < 128; x++) {
			// Bump-shaped function f that is zero on each edge
			float a0 = (float)y / 127.0f;
			float a1 = (float)(127 - x) / 127.0f;
			float a2 = (float)(x - y) / 127.0f;

			float f = a0 * a1 * a2;
			f = (float)sqrt(f);

			int k = (int)floor(1400.0f * f);
			if (k < 0) k = 0;
			if (k > 255) k = 255;

			gridData[4 * (y * 128 + x) + 0] = 30 + (SQR(k)) / 290;
			gridData[4 * (y * 128 + x) + 1] = k;
			gridData[4 * (y * 128 + x) + 2] = (k < 128 ? 0 : (k - 128) * 2);
			gridData[4 * (y * 128 + x) + 3] = 0;
		}
	}

	// Texture coordinates for the three triangle vertices (all triangles share
	// them when showing the grid); sucked in by half a texel for bilinear
	float a0 = 0.5f / 128.0f;
	float a1 = 1.0f - a0;
	gridTexCoords[0][0] = a0; gridTexCoords[0][1] = a0;
	gridTexCoords[1][0] = a1; gridTexCoords[1][1] = a0;
	gridTexCoords[2][0] = a1; gridTexCoords[2][1] = a1;

	// Set up the grid texture
	glGenTextures(1, &gridTexID);
	glBindTexture(GL_TEXTURE_2D, gridTexID);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 128, 128, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 128, 128, GL_RGBA, GL_UNSIGNED_BYTE, gridData);

	camera = cam;
}

//
// Shutdown the ROAM engine
//
void RoamLOD::Shutdown(void)
{
	delete[] levelMDSize;
	levelMDSize = NULL;
}

//
// Render the ROAM engine
//
void RoamLOD::Render(void)
{
	float verts[4][3];

	vertsPerFrame = 0;
	trisPerFrame = 0;

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, gridTexID);

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	// Four corners of the base square
	for (int i = 0; i < 4; i++) {
		verts[i][0] = ((i & 1) ? 1.0f : -1.0f);
		verts[i][1] = 0.0f;
		verts[i][2] = ((i & 2) ? 1.0f : -1.0f);
	}

	// Recurse on the two base triangles
	glBegin(GL_TRIANGLES);
	RenderSub(0, verts[0], verts[1], verts[3]);
	RenderSub(0, verts[3], verts[2], verts[0]);
	glEnd();

	glDisable(GL_TEXTURE_2D);
}

//
// Render a subdivision (child) triangle
//
void RoamLOD::RenderSub(int level, float *vert1, float *vert2, float *vert3)
{
	float newVert[3];

	// Max midpoint-displacement size (crack-fix hack)
	float md = levelMDSize[level];

	// Split point of the base edge
	newVert[0] = (vert1[0] + vert3[0]) / 2.0f;
	newVert[1] = 0.0f;
	newVert[2] = (vert1[2] + vert3[2]) / 2.0f;

	// Random perturbation of the center via a hash of the x,y bytes
	unsigned char *pC = (unsigned char *)newVert;
	unsigned int s = 0;
	for (int i = 0; i < 8; i++) {
		s += randtab[(i << 8) | pC[i]];
	}

	// Stuff random hash bits into a float (IEEE trick, via memcpy)
	float randHash;
	unsigned int bits = 0x40000000 + (s & 0x007fffff);
	memcpy(&randHash, &bits, sizeof(randHash));
	randHash -= 3.0f;

	// The random height value for the vertex
	newVert[1] = ((vert1[1] + vert3[1]) / 2.0f) + randHash * md;

	// Distance from the camera
	float dist = SQR(newVert[0] - camera->head[0]) +
		SQR(newVert[1] - camera->head[1]) +
		SQR(newVert[2] - camera->head[2]);

	if (level < maxLevel && SQR(md) > dist * 0.00001f) {
		// Render the children; this node is fully covered by them
		RenderSub(level + 1, vert1, newVert, vert2);
		RenderSub(level + 1, vert2, newVert, vert3);
		return;
	}

	// Send the vertices to the rendering API
	glTexCoord2fv(gridTexCoords[0]); glVertex3fv(vert1);
	glTexCoord2fv(gridTexCoords[1]); glVertex3fv(vert2);
	glTexCoord2fv(gridTexCoords[2]); glVertex3fv(vert3);

	vertsPerFrame += 3;
	trisPerFrame++;
}
