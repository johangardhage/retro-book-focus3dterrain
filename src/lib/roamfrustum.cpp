//
// Focus on 3D Terrain Programming
//
// ROAM with frustum culling.
//
// Original coders: Trent Polack (trent@voxelsoft.com) and Mark Duchaineau
//
// Ported from the original. The OpenGL render uses ARB multitexturing to
// combine the color and detail maps in a single pass (with a two-pass color +
// detail fallback when multitexturing is disabled), matching the book.
//
#include <math.h> // sqrt
#include "retrogl.h"
#include "roamfrustum.h"

#define SQR(n) ((n) * (n))

//
// Initialize the ROAM engine (maxLevel is the deepest recursion level)
//
void RoamFrustum::Init(int level, RETRO_Camera *cam)
{
	maxLevel = level;
	levelMDSize = new float[maxLevel + 1];

	// Generate the table of displacement sizes versus levels
	for (int i = 0; i <= maxLevel; i++) {
		levelMDSize[i] = 255.0f / ((float)sqrt((float)(1 << i)));
	}

	camera = cam;
}

//
// Shutdown the ROAM engine
//
void RoamFrustum::Shutdown(void)
{
	delete[] levelMDSize;
	levelMDSize = NULL;
}

//
// Render the ROAM engine
//
void RoamFrustum::Render(void)
{
	float verts[4][3];

	vertsPerFrame = 0;
	trisPerFrame = 0;

	// Four corners of the base square (over the whole heightmap)
	for (int i = 0; i < 4; i++) {
		verts[i][0] = ((i & 1) ? (float)(size - 1) : 0.0f);
		verts[i][1] = 0.0f;
		verts[i][2] = ((i & 2) ? (float)(size - 1) : 0.0f);
	}

	bool doTexture = textureMapping && texture.IsLoaded();
	bool doDetail = detailMapping && detailMap.IsLoaded();

	// No texturing: render once with vertex colors only
	if (!doTexture && !doDetail) {
		glDisable(GL_TEXTURE_2D);
		glBegin(GL_TRIANGLES);
		RenderSub(0, verts[0], verts[1], verts[3], 0, 1.0f);
		RenderSub(0, verts[3], verts[2], verts[0], 0, 1.0f);
		glEnd();
		return;
	}

	// Single-pass ARB multitexturing: color map * detail map at once
	if (multitexture && doTexture && doDetail) {
		BeginMultitexture();
		glBegin(GL_TRIANGLES);
		RenderSub(0, verts[0], verts[1], verts[3], 0, 1.0f);
		RenderSub(0, verts[3], verts[2], verts[0], 0, 1.0f);
		glEnd();
		EndMultitexture();
		return;
	}

	// Color texture pass
	if (doTexture) {
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, texture.GetID());
		glBegin(GL_TRIANGLES);
		RenderSub(0, verts[0], verts[1], verts[3], 0, 1.0f);
		RenderSub(0, verts[3], verts[2], verts[0], 0, 1.0f);
		glEnd();
	}

	// Detail map pass (multiplies onto the color pass)
	if (doDetail) {
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, detailMap.GetID());

		if (doTexture) {
			glEnable(GL_BLEND);
			glBlendFunc(GL_ZERO, GL_SRC_COLOR);
		}

		glBegin(GL_TRIANGLES);
		RenderSub(0, verts[0], verts[1], verts[3], 0, (float)repeatDetailMap);
		RenderSub(0, verts[3], verts[2], verts[0], 0, (float)repeatDetailMap);
		glEnd();

		glDisable(GL_BLEND);
	}
}

//
// Render a subdivision (child) triangle, culling against the view frustum
//
void RoamFrustum::RenderSub(int level, float *vert1, float *vert2, float *vert3, int cull, float texRepeat)
{
	float newVert[3];

	float md = levelMDSize[level];

	// Split point of the base edge, with its real terrain height
	newVert[0] = (vert1[0] + vert3[0]) / 2.0f;
	newVert[2] = (vert1[2] + vert3[2]) / 2.0f;
	newVert[1] = GetTrueHeightAtPoint((int)newVert[0], (int)newVert[2]);

	// Bounding sphere of the triangle (max squared distance of any corner to
	// the split vertex)
	float sqrBound = SQR(vert1[0] - newVert[0]) + SQR(vert1[1] - newVert[1]) + SQR(vert1[2] - newVert[2]);
	float temp = SQR(vert2[0] - newVert[0]) + SQR(vert2[1] - newVert[1]) + SQR(vert2[2] - newVert[2]);
	if (temp > sqrBound) sqrBound = temp;
	temp = SQR(vert3[0] - newVert[0]) + SQR(vert3[1] - newVert[1]) + SQR(vert3[2] - newVert[2]);
	if (temp > sqrBound) sqrBound = temp;

	// Test the bounding sphere against the view frustum
	if (cull != CULL_ALLIN) {
		float (*frustum)[4] = camera->GetViewFrustum();

		for (int j = 0, m = 1; j < 6; j++, m <<= 1) {
			if (!(cull & m)) {
				float r = frustum[j][0] * newVert[0] +
					frustum[j][1] * newVert[1] +
					frustum[j][2] * newVert[2] +
					frustum[j][3];

				if (SQR(r) > sqrBound) {
					// The sphere is entirely on one side of this plane
					if (r < 0.0f) {
						return;	// Fully outside the frustum
					}
					cull |= m;	// Fully inside this plane
				}
			}
		}
	}

	// Distance from the camera
	float dist = SQR(newVert[0] - camera->head[0]) +
		SQR(newVert[1] - camera->head[1]) +
		SQR(newVert[2] - camera->head[2]);

	if (level < maxLevel && SQR(md) > dist * 0.00001f) {
		RenderSub(level + 1, vert1, newVert, vert2, cull, texRepeat);
		RenderSub(level + 1, vert2, newVert, vert3, cull, texRepeat);
		return;
	}

	// Send the three vertices (color -> texcoord -> position)
	unsigned char shade = GetBrightnessAtPoint((int)vert1[0], (int)vert1[2]);
	glColor3ub(shade, shade, shade);
	EmitTexCoord(vert1[0] / size * texRepeat, vert1[2] / size * texRepeat);
	glVertex3fv(vert1);

	shade = GetBrightnessAtPoint((int)vert2[0], (int)vert2[2]);
	glColor3ub(shade, shade, shade);
	EmitTexCoord(vert2[0] / size * texRepeat, vert2[2] / size * texRepeat);
	glVertex3fv(vert2);

	shade = GetBrightnessAtPoint((int)vert3[0], (int)vert3[2]);
	glColor3ub(shade, shade, shade);
	EmitTexCoord(vert3[0] / size * texRepeat, vert3[2] / size * texRepeat);
	glVertex3fv(vert3);

	vertsPerFrame += 3;
	trisPerFrame++;
}
