//
// Focus on 3D Terrain Programming
//
// Geomipmapping terrain implementation.
//
// Original coders: Trent Polack (trent@voxelsoft.com)
//
// Ported from the original. The OpenGL render uses ARB multitexturing to
// combine the color and detail maps in a single pass (with a two-pass color +
// detail fallback when multitexturing is disabled), matching the book.
//
#include <math.h> // sqrt, fabs
#include "retrogl.h"
#include "geomipmapping.h"

#define SQR(n) ((n) * (n))

//
// Initiate the geomipmapping system (patchSize is in vertices, e.g. 17)
//
bool GeoMipMapping::Init(int patchSizeIn)
{
	if (size == 0) {
		return false;
	}

	if (patches) {
		Shutdown();
	}

	// Initiate the patch information
	patchSize = patchSizeIn;
	numPatchesPerSide = size / patchSize;
	patches = new GeoMMPatch[SQR(numPatchesPerSide)];

	// Figure out the maximum level of detail for a patch
	int divisor = patchSize - 1;
	int lod = 0;
	while (divisor > 2) {
		divisor = divisor >> 1;
		lod++;
	}
	maxLOD = lod;

	// Initialize the patch values
	for (int z = 0; z < numPatchesPerSide; z++) {
		for (int x = 0; x < numPatchesPerSide; x++) {
			patches[GetPatchNumber(x, z)].LOD = maxLOD;
			patches[GetPatchNumber(x, z)].visible = true;
		}
	}

	return true;
}

//
// Shutdown the geomipmapping system
//
void GeoMipMapping::Shutdown(void)
{
	if (patches) {
		delete[] patches;
		patches = NULL;
	}

	patchSize = 0;
	numPatchesPerSide = 0;
	maxLOD = 0;
}

//
// Update the geomipmapping system (compute per-patch visibility, distance, LOD)
//
void GeoMipMapping::Update(RETRO_Camera *camera, bool cullPatches)
{
	for (int z = 0; z < numPatchesPerSide; z++) {
		for (int x = 0; x < numPatchesPerSide; x++) {
			int patch = GetPatchNumber(x, z);

			// Compute patch center (used for distance/culling)
			float fX = (x * patchSize) + (patchSize / 2.0f);
			float fZ = (z * patchSize) + (patchSize / 2.0f);
			float fY = GetScaledHeightAtPoint((int)fX, (int)fZ);

			// Only scale X and Z; the Y value is already scaled
			fX *= scale[0];
			fZ *= scale[2];

			// Frustum-cull the patch if requested
			if (cullPatches) {
				patches[patch].visible = camera->CubeFrustumTest(fX, fY, fZ, patchSize * scale[0]);
			} else {
				patches[patch].visible = true;
			}

			// Only finish updating if the patch is visible
			if (patches[patch].visible) {
				patches[patch].distance = sqrt(SQR(fX - camera->head[0]) +
					SQR(fY - camera->head[1]) +
					SQR(fZ - camera->head[2]));

				// Simple distance-based LOD selection
				if (patches[patch].distance < 500) {
					patches[patch].LOD = 0;
				} else if (patches[patch].distance < 1000) {
					patches[patch].LOD = 1;
				} else if (patches[patch].distance < 2500) {
					patches[patch].LOD = 2;
				} else {
					patches[patch].LOD = 3;
				}
			}
		}
	}
}

//
// Render the geomipmapping system
//
void GeoMipMapping::Render(void)
{
	// Reset the counting variables
	patchesPerFrame = 0;
	vertsPerFrame = 0;
	trisPerFrame = 0;

	// Enable back-face culling
	glEnable(GL_CULL_FACE);

	bool doTexture = textureMapping && texture.IsLoaded();
	bool doDetail = detailMapping && detailMap.IsLoaded();

	// No texturing: render once with vertex colors only
	if (!doTexture && !doDetail) {
		glDisable(GL_TEXTURE_2D);
		RenderAllPatches(1.0f);
		return;
	}

	// Single-pass ARB multitexturing: color map * detail map at once
	if (multitexture && doTexture && doDetail) {
		BeginMultitexture();
		RenderAllPatches(1.0f);
		EndMultitexture();
		return;
	}

	// Color texture pass
	if (doTexture) {
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, texture.GetID());
		RenderAllPatches(1.0f);
	}

	// Detail map pass (multiplies onto the color pass)
	if (doDetail) {
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, detailMap.GetID());

		if (doTexture) {
			glEnable(GL_BLEND);
			glBlendFunc(GL_ZERO, GL_SRC_COLOR);
		}

		RenderAllPatches((float)repeatDetailMap);

		glDisable(GL_BLEND);
	}
}

//
// Render every visible patch with the given texture-coordinate repeat factor
//
void GeoMipMapping::RenderAllPatches(float texRepeat)
{
	for (int z = 0; z < numPatchesPerSide; z++) {
		for (int x = 0; x < numPatchesPerSide; x++) {
			if (patches[GetPatchNumber(x, z)].visible) {
				RenderPatch(x, z, texRepeat);
				patchesPerFrame++;
			}
		}
	}
}

//
// Render a single vertex (shaded color + texture coordinate + position)
//
void GeoMipMapping::RenderVertex(float x, float z, float u, float v)
{
	unsigned char color = GetBrightnessAtPoint((int)x, (int)z);

	glColor3ub((unsigned char)(color * lightColor[0]),
		(unsigned char)(color * lightColor[1]),
		(unsigned char)(color * lightColor[2]));

	EmitTexCoord(u, v);

	glVertex3f(x * scale[0], GetScaledHeightAtPoint((int)x, (int)z), z * scale[2]);

	vertsPerFrame++;
}

//
// Render a patch of terrain as a grid of triangle fans
//
void GeoMipMapping::RenderPatch(int PX, int PZ, float texRepeat)
{
	GeoMMNeighbor patchNeighbor;
	GeoMMNeighbor fanNeighbor;
	int patch = GetPatchNumber(PX, PZ);

	// Determine whether each neighbouring patch is of equal-or-greater detail
	// (or off the edge of the map). If so, we can render the shared mid-edge
	// vertex without causing cracks.
	patchNeighbor.left = (PX == 0) ||
		(patches[GetPatchNumber(PX - 1, PZ)].LOD <= patches[patch].LOD);
	patchNeighbor.up = (PZ >= numPatchesPerSide - 1) ||
		(patches[GetPatchNumber(PX, PZ + 1)].LOD <= patches[patch].LOD);
	patchNeighbor.right = (PX >= numPatchesPerSide - 1) ||
		(patches[GetPatchNumber(PX + 1, PZ)].LOD <= patches[patch].LOD);
	patchNeighbor.down = (PZ == 0) ||
		(patches[GetPatchNumber(PX, PZ - 1)].LOD <= patches[patch].LOD);

	// Determine the distance between the center of each triangle fan
	float fSize = (float)patchSize;
	int divisor = patchSize - 1;
	int lod = patches[patch].LOD;
	while (lod-- >= 0) {
		divisor = divisor >> 1;
	}
	fSize /= divisor;

	// Half the size between fan centers (the size between each vertex)
	float fHalfSize = fSize / 2.0f;

	for (float z = fHalfSize; ((int)(z + fHalfSize)) < patchSize + 1; z += fSize) {
		for (float x = fHalfSize; ((int)(x + fHalfSize)) < patchSize + 1; x += fSize) {
			// Adjust edge fans to match the neighbouring patch detail
			fanNeighbor.left = (x == fHalfSize) ? patchNeighbor.left : true;
			fanNeighbor.down = (z == fHalfSize) ? patchNeighbor.down : true;
			fanNeighbor.right = (x >= (patchSize - fHalfSize)) ? patchNeighbor.right : true;
			fanNeighbor.up = (z >= (patchSize - fHalfSize)) ? patchNeighbor.up : true;

			RenderFan((PX * patchSize) + x, (PZ * patchSize) + z, fSize, fanNeighbor, texRepeat);
		}
	}
}

//
// Render a single triangle fan, skipping mid-edge vertices where a lower-detail
// neighbour requires it (to avoid cracks)
//
void GeoMipMapping::RenderFan(float cX, float cZ, float fSize, GeoMMNeighbor neighbor, float texRepeat)
{
	float fHalfSize = fSize / 2.0f;

	// Texture coordinates for the fan's extents
	float texLeft = (fabs(cX - fHalfSize) / size) * texRepeat;
	float texBottom = (fabs(cZ - fHalfSize) / size) * texRepeat;
	float texRight = (fabs(cX + fHalfSize) / size) * texRepeat;
	float texTop = (fabs(cZ + fHalfSize) / size) * texRepeat;

	float midX = (texLeft + texRight) / 2;
	float midZ = (texBottom + texTop) / 2;

	glBegin(GL_TRIANGLE_FAN);

	// Center vertex
	RenderVertex(cX, cZ, midX, midZ);

	// Lower-left vertex
	RenderVertex(cX - fHalfSize, cZ - fHalfSize, texLeft, texBottom);

	// Mid-left vertex (only if the left neighbour is not lower detail)
	if (neighbor.left) {
		RenderVertex(cX - fHalfSize, cZ, texLeft, midZ);
		trisPerFrame++;
	}

	// Upper-left vertex
	RenderVertex(cX - fHalfSize, cZ + fHalfSize, texLeft, texTop);
	trisPerFrame++;

	// Upper-mid vertex
	if (neighbor.up) {
		RenderVertex(cX, cZ + fHalfSize, midX, texTop);
		trisPerFrame++;
	}

	// Upper-right vertex
	RenderVertex(cX + fHalfSize, cZ + fHalfSize, texRight, texTop);
	trisPerFrame++;

	// Mid-right vertex
	if (neighbor.right) {
		RenderVertex(cX + fHalfSize, cZ, texRight, midZ);
		trisPerFrame++;
	}

	// Lower-right vertex
	RenderVertex(cX + fHalfSize, cZ - fHalfSize, texRight, texBottom);
	trisPerFrame++;

	// Lower-mid vertex
	if (neighbor.down) {
		RenderVertex(cX, cZ - fHalfSize, midX, texBottom);
		trisPerFrame++;
	}

	// Close the fan back at the lower-left vertex
	RenderVertex(cX - fHalfSize, cZ - fHalfSize, texLeft, texBottom);
	trisPerFrame++;

	glEnd();
}
