//
// Focus on 3D Terrain Programming - demo 1_1
//
// Real-time Optimally Adapting Meshes.
//
// Original coders: Trent Polack (trent@voxelsoft.com) and Mark Duchaineau
//
// Ported from the original Win32/OpenGL demo. The OpenGL rendering is kept
// essentially verbatim; only the Win32-specific host code and the CVECTOR
// camera type were replaced (the camera eye is now a plain float[3]).
//
#include <math.h> // sqrt, floor
#include <string.h> // memcpy
#include "retrogl.h"
#include "roam.h"
#include "randtab.h"

#define SQR(n) ((n) * (n))

//
// Initialize the ROAM engine
//
void ROAM::Initialize(void)
{
	char gridTexData[128 * 128 * 4];	// Texture for showing mesh

	level2dzsize = new float[LEVEL_MAX + 1];

	// Generate table of displacement sizes versus levels
	for (int level = 0; level <= LEVEL_MAX; level++) {
		level2dzsize[level] = 3.0f / ((float)sqrt((float)(1 << level)));
	}

	// Generate grid texture
	for (int y = 0; y < 128; y++) {
		for (int x = 0; x < 128; x++) {
			// Create bump-shaped function f that is zero on each edge
			float a0 = (float)y / 127.0f;
			float a1 = (float)(127 - x) / 127.0f;
			float a2 = (float)(x - y) / 127.0f;

			float f = a0 * a1 * a2;
			f = (float)sqrt(f);

			// Quantize the function value and make into color,
			// store in rgb components of texture entry
			int k = (int)floor(1400.0f * f);

			if (k < 0)
				k = 0;
			if (k > 255)
				k = 255;

			gridTexData[4 * (y * 128 + x) + 0] = 30 + (SQR(k)) / 290;
			gridTexData[4 * (y * 128 + x) + 1] = k;
			gridTexData[4 * (y * 128 + x) + 2] = (k < 128 ? 0 : (k - 128) * 2);
			gridTexData[4 * (y * 128 + x) + 3] = 0;
		}
	}

	// Make texture coordinates for three triangle vertices.
	// All triangles use same tex coords when showing grid.
	// Suck in by half a texel to be correct for bilinear textures.
	float a0 = 0.5f / 128.0f;
	float a1 = 1.0f - a0;

	gridtex_t[0][0] = a0; gridtex_t[0][1] = a0;
	gridtex_t[1][0] = a1; gridtex_t[1][1] = a0;
	gridtex_t[2][0] = a1; gridtex_t[2][1] = a1;

	// Set up the gridview texture
	glGenTextures(1, &gridTexID);
	glBindTexture(GL_TEXTURE_2D, gridTexID);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 128, 128, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 128, 128, GL_RGBA, GL_UNSIGNED_BYTE, gridTexData);
}

//
// Shutdown the ROAM engine
//
void ROAM::Shutdown(void)
{
	delete[] level2dzsize;
}

//
// Update the ROAM engine with the camera's eye position
//
void ROAM::Update(float *eye)
{
	cameraEye[0] = eye[0];
	cameraEye[1] = eye[1];
	cameraEye[2] = eye[2];
}

//
// Render the ROAM engine
//
void ROAM::Render(void)
{
	float verts[4][3];

	// Reset the debug counters
	vertsPerFrame = 0;
	trisPerFrame = 0;

	// Turn on textured drawing (with the grid texture for now)
	glBindTexture(GL_TEXTURE_2D, gridTexID);
	glEnable(GL_TEXTURE_2D);

	// Render the roam mesh.
	// Compute four corners of the base square.
	for (int i = 0; i < 4; i++) {
		verts[i][0] = ((i & 1) ? 1.0f : -1.0f);
		verts[i][1] = 0.0f;
		verts[i][2] = ((i & 2) ? 1.0f : -1.0f);
	}

	glColor3f(1.0f, 1.0f, 1.0f);

	// Recurse on the two base triangles
	RenderSub(0, verts[0], verts[1], verts[3]);
	RenderSub(0, verts[3], verts[2], verts[0]);

	// End texturing
	glDisable(GL_TEXTURE_2D);
}

//
// Render a subdivision (child) triangle
//
void ROAM::RenderSub(int level, float *vert1, float *vert2, float *vert3)
{
	float newVert[3];	// New (split) vertex

	// Squared length of edge (vert1-vert3)
	float sqrEdge = SQR((vert3[0] - vert1[0])) +
		SQR((vert3[1] - vert1[1])) +
		SQR((vert3[2] - vert1[2]));

	// Compute split point of base edge
	newVert[0] = (vert1[0] + vert3[0]) / 2.0f;
	newVert[1] = 0.0f;
	newVert[2] = (vert1[2] + vert3[2]) / 2.0f;

	// Determine random perturbation of center z using hash of x,y.
	// Random number lookup per byte of (x, z) data, all added.
	unsigned char *pC = (unsigned char *)newVert;
	unsigned int uiS = 0;
	for (int i = 0; i < 8; i++) {
		uiS += randtab[(i << 8) | pC[i]];
	}

	// Stuff random hash value bits from uiS into float (float viewed
	// as an int, IEEE float tricks here...). Done via memcpy to avoid
	// strict-aliasing issues.
	float randHeight;
	unsigned int bits = 0x40000000 + (uiS & 0x007fffff);
	memcpy(&randHeight, &bits, sizeof(randHeight));
	randHeight -= 3.0f;
	randHeight = 0.10f * randHeight * level2dzsize[level];

	// The random height value for the vertex
	newVert[1] = ((vert1[1] + vert3[1]) / 2.0f) + randHeight;

	// Distance calculation
	float distance = SQR((newVert[0] - cameraEye[0])) +
		SQR((newVert[1] - cameraEye[1])) +
		SQR((newVert[2] - cameraEye[2]));

	if (level < LEVEL_MAX && sqrEdge > distance * 0.005f) {
		// Render the children
		RenderSub(level + 1, vert1, newVert, vert2);
		RenderSub(level + 1, vert2, newVert, vert3);

		// The current node doesn't need to be rendered,
		// since both of its children are
		return;
	}

	// Send the vertices to the rendering API
	glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2fv(gridtex_t[0]); glVertex3fv(vert1);
		glTexCoord2fv(gridtex_t[1]); glVertex3fv(vert2);
		glTexCoord2fv(gridtex_t[2]); glVertex3fv(vert3);
	glEnd();

	vertsPerFrame += 3;
	trisPerFrame++;
}
