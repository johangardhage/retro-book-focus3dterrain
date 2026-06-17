//
// Focus on 3D Terrain Programming
//
// Quadtree terrain implementation (after Roettger / Chris Cookson).
//
// Original coders: Trent Polack (trent@voxelsoft.com)
//
// Ported from the original. The OpenGL render uses ARB multitexturing to
// combine the color and detail maps in a single pass (with a two-pass color +
// detail fallback when multitexturing is disabled), matching the book.
//
#include <math.h> // ceil, fabs
#include <stdlib.h> // abs
#include "retrogl.h"
#include "quadtree.h"

#define SQR(n) ((n) * (n))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

// Fan-code lookup tables (which fans to render, and where to start)
static char fanCode[] = { 10, 8, 8, 12, 8, 0, 12, 14, 8, 12, 0, 14, 12, 14, 14, 0 };
static char fanStart[] = { 3, 3, 0, 3, 1, 0, 0, 3, 2, 2, 0, 2, 1, 1, 0, 0 };

//
// Initialize the quadtree engine
//
bool QuadTree::Init(void)
{
	if (size == 0) {
		return false;
	}

	if (quadMatrix) {
		Shutdown();
	}

	// Create memory for the quadtree matrix
	quadMatrix = new unsigned char[SQR(size)];

	// Initialize the quadtree matrix to 1 (all nodes present)
	for (int z = 0; z < size; z++) {
		for (int x = 0; x < size; x++) {
			quadMatrix[GetMatrixIndex(x, z)] = 1;
		}
	}

	// Propagate the roughness so rougher areas get more triangles
	if (useRoughness) {
		PropagateRoughness();
	}

	return true;
}

//
// Shutdown the quadtree engine
//
void QuadTree::Shutdown(void)
{
	if (quadMatrix) {
		delete[] quadMatrix;
		quadMatrix = NULL;
	}
}

//
// Update the quadtree engine (build the mesh via top-down traversal)
//
void QuadTree::Update(RETRO_Camera *cam, bool cull)
{
	camera = cam;
	cullNodes = cull;

	float center = (size - 1) / 2.0f;
	RefineNode(center, center, size);
}

//
// Render the quadtree engine
//
void QuadTree::Render(void)
{
	vertsPerFrame = 0;
	trisPerFrame = 0;

	float center = (size - 1) / 2.0f;

	glDisable(GL_CULL_FACE);

	bool doTexture = textureMapping && texture.IsLoaded();
	bool doDetail = detailMapping && detailMap.IsLoaded();

	// No texturing: render once with vertex colors only
	if (!doTexture && !doDetail) {
		glDisable(GL_TEXTURE_2D);
		RenderNode(center, center, size, 1.0f);
		return;
	}

	// Single-pass ARB multitexturing: color map * detail map at once
	if (multitexture && doTexture && doDetail) {
		BeginMultitexture();
		RenderNode(center, center, size, 1.0f);
		EndMultitexture();
		return;
	}

	// Color texture pass
	if (doTexture) {
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, texture.GetID());
		RenderNode(center, center, size, 1.0f);
	}

	// Detail map pass (multiplies onto the color pass)
	if (doDetail) {
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, detailMap.GetID());

		if (doTexture) {
			glEnable(GL_BLEND);
			glBlendFunc(GL_ZERO, GL_SRC_COLOR);
		}

		RenderNode(center, center, size, (float)repeatDetailMap);

		glDisable(GL_BLEND);
	}
}

//
// Propagate the roughness of the height map into the quadtree matrix, so that
// rougher areas of the terrain get refined into more triangles.
//
void QuadTree::PropagateRoughness(void)
{
	int edgeLength = 3;

	// Start at the lowest level of detail and traverse up to the highest node
	while (edgeLength <= size) {
		int edgeOffset = (edgeLength - 1) >> 1;
		int childOffset = (edgeLength - 1) >> 2;

		for (int z = edgeOffset; z < size; z += (edgeLength - 1)) {
			for (int x = edgeOffset; x < size; x += (edgeLength - 1)) {
				// Compute the local d2 (second derivative / roughness) values
				// upper-mid
				int localD2 = (int)ceil(abs(((GetTrueHeightAtPoint(x - edgeOffset, z + edgeOffset) +
					GetTrueHeightAtPoint(x + edgeOffset, z + edgeOffset)) >> 1) -
					GetTrueHeightAtPoint(x, z + edgeOffset)));

				// right-mid
				int dH = (int)ceil(abs(((GetTrueHeightAtPoint(x + edgeOffset, z + edgeOffset) +
					GetTrueHeightAtPoint(x + edgeOffset, z - edgeOffset)) >> 1) -
					GetTrueHeightAtPoint(x + edgeOffset, z)));
				localD2 = MAX(localD2, dH);

				// bottom-mid
				dH = (int)ceil(abs(((GetTrueHeightAtPoint(x - edgeOffset, z - edgeOffset) +
					GetTrueHeightAtPoint(x + edgeOffset, z - edgeOffset)) >> 1) -
					GetTrueHeightAtPoint(x, z - edgeOffset)));
				localD2 = MAX(localD2, dH);

				// left-mid
				dH = (int)ceil(abs(((GetTrueHeightAtPoint(x - edgeOffset, z + edgeOffset) +
					GetTrueHeightAtPoint(x - edgeOffset, z - edgeOffset)) >> 1) -
					GetTrueHeightAtPoint(x - edgeOffset, z)));
				localD2 = MAX(localD2, dH);

				// bottom-left to top-right diagonal
				dH = (int)ceil(abs(((GetTrueHeightAtPoint(x - edgeOffset, z - edgeOffset) +
					GetTrueHeightAtPoint(x + edgeOffset, z + edgeOffset)) >> 1) -
					GetTrueHeightAtPoint(x, z)));
				localD2 = MAX(localD2, dH);

				// bottom-right to top-left diagonal
				dH = (int)ceil(abs(((GetTrueHeightAtPoint(x + edgeOffset, z - edgeOffset) +
					GetTrueHeightAtPoint(x - edgeOffset, z + edgeOffset)) >> 1) -
					GetTrueHeightAtPoint(x, z)));
				localD2 = MAX(localD2, dH);

				// Make localD2 a value between 0-255
				localD2 = (int)ceil((localD2 * 3.0f) / edgeLength);

				int d2;
				int localH;

				// Smallest possible node
				if (edgeLength == 3) {
					d2 = localD2;

					// Maximum height of the nine samples
					localH = GetTrueHeightAtPoint(x + edgeOffset, z + edgeOffset);
					localH = MAX(localH, GetTrueHeightAtPoint(x + edgeOffset, z));
					localH = MAX(localH, GetTrueHeightAtPoint(x + edgeOffset, z - edgeOffset));
					localH = MAX(localH, GetTrueHeightAtPoint(x, z - edgeOffset));
					localH = MAX(localH, GetTrueHeightAtPoint(x - edgeOffset, z - edgeOffset));
					localH = MAX(localH, GetTrueHeightAtPoint(x - edgeOffset, z));
					localH = MAX(localH, GetTrueHeightAtPoint(x - edgeOffset, z + edgeOffset));
					localH = MAX(localH, GetTrueHeightAtPoint(x, z + edgeOffset));
					localH = MAX(localH, GetTrueHeightAtPoint(x, z));

					quadMatrix[GetMatrixIndex(x + 1, z)] = localH;
				} else {
					float kUpperBound = 1.0f * minResolution / (2.0f * (minResolution - 1.0f));

					// Use d2 values from farther up the quadtree
					d2 = (int)ceil(MAX(kUpperBound * (float)GetQuadMatrixData(x, z), (float)localD2));
					d2 = (int)ceil(MAX(kUpperBound * (float)GetQuadMatrixData(x - edgeOffset, z), (float)d2));
					d2 = (int)ceil(MAX(kUpperBound * (float)GetQuadMatrixData(x + edgeOffset, z), (float)d2));
					d2 = (int)ceil(MAX(kUpperBound * (float)GetQuadMatrixData(x, z + edgeOffset), (float)d2));
					d2 = (int)ceil(MAX(kUpperBound * (float)GetQuadMatrixData(x, z - edgeOffset), (float)d2));

					// Maximum local height of the four child nodes
					localH = GetTrueHeightAtPoint(x + childOffset, z + childOffset);
					localH = MAX(localH, GetTrueHeightAtPoint(x + childOffset, z - childOffset));
					localH = MAX(localH, GetTrueHeightAtPoint(x - childOffset, z - childOffset));
					localH = MAX(localH, GetTrueHeightAtPoint(x - childOffset, z + childOffset));

					quadMatrix[GetMatrixIndex(x + 1, z)] = localH;
				}

				// Store the d2 values into the quadtree matrix
				quadMatrix[GetMatrixIndex(x, z)] = d2;
				quadMatrix[GetMatrixIndex(x - 1, z)] = d2;

				// Propagate the value up the quadtree
				quadMatrix[GetMatrixIndex(x - edgeOffset, z - edgeOffset)] = MAX(GetQuadMatrixData(x - edgeOffset, z - edgeOffset), d2);
				quadMatrix[GetMatrixIndex(x - edgeOffset, z + edgeOffset)] = MAX(GetQuadMatrixData(x - edgeOffset, z + edgeOffset), d2);
				quadMatrix[GetMatrixIndex(x + edgeOffset, z + edgeOffset)] = MAX(GetQuadMatrixData(x + edgeOffset, z + edgeOffset), d2);
				quadMatrix[GetMatrixIndex(x + edgeOffset, z - edgeOffset)] = MAX(GetQuadMatrixData(x + edgeOffset, z - edgeOffset), d2);
			}
		}

		// Move up to the next quadtree level (lower level of detail)
		edgeLength = (edgeLength << 1) - 1;
	}
}

//
// Refine a quadtree node (update the quadtree matrix), optionally culling nodes
// that fall outside the view frustum.
//
void QuadTree::RefineNode(float x, float z, int edgeLength)
{
	int iX = (int)x;
	int iZ = (int)z;

	// Frustum-cull this node (and its children) if requested
	if (cullNodes) {
		if (!camera->CubeFrustumTest(x * scale[0], GetScaledHeightAtPoint(iX, iZ), z * scale[2], edgeLength * scale[0])) {
			quadMatrix[GetMatrixIndex(iX, iZ)] = 0;
			return;
		}
	}

	// Distance from the camera (L1 norm). The (iX+1) cell holds the node's max
	// height (from roughness propagation); the (iX-1) cell holds its roughness.
	float viewDistance = (float)(fabs(camera->head[0] - (x * scale[0])) +
		fabs(camera->head[1] - GetQuadMatrixData(iX + 1, iZ)) +
		fabs(camera->head[2] - (z * scale[2])));

	// Compute the 'f' value (Roettger)
	float f = viewDistance / ((float)edgeLength * minResolution *
		MAX(detailLevel * (float)GetQuadMatrixData(iX - 1, iZ) / 3, 1.0f));

	int blend = (f < 1.0f) ? 255 : 0;

	// Store the blend factor in the quadtree matrix
	quadMatrix[GetMatrixIndex(iX, iZ)] = blend;

	if (blend) {
		// Recurse down farther into the quadtree
		if (edgeLength > 3) {
			float childOffset = (float)((edgeLength - 1) >> 2);
			int childEdgeLength = (edgeLength + 1) >> 1;

			RefineNode(x - childOffset, z - childOffset, childEdgeLength);
			RefineNode(x + childOffset, z - childOffset, childEdgeLength);
			RefineNode(x - childOffset, z + childOffset, childEdgeLength);
			RefineNode(x + childOffset, z + childOffset, childEdgeLength);
		}
	}
}

//
// Render a single vertex (shaded color + texture coordinate + position)
//
void QuadTree::RenderVertex(float x, float z, float u, float v)
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
// Render quadtree nodes (recursively), as triangle fans
//
void QuadTree::RenderNode(float x, float z, int edgeLength, float texRepeat)
{
	int iX = (int)x;
	int iZ = (int)z;

	// If this node is disabled (culled or not subdivided), skip it
	if (GetQuadMatrixData(iX, iZ) == 0) {
		return;
	}

	// Edge offsets
	float fEdgeOffset = (edgeLength - 1) / 2.0f;
	int adjOffset = edgeLength - 1;

	// Texture coordinates for this node's extents
	float texLeft = (fabs(x - fEdgeOffset) / size) * texRepeat;
	float texBottom = (fabs(z - fEdgeOffset) / size) * texRepeat;
	float texRight = (fabs(x + fEdgeOffset) / size) * texRepeat;
	float texTop = (fabs(z + fEdgeOffset) / size) * texRepeat;
	float midX = (texLeft + texRight) / 2.0f;
	float midZ = (texBottom + texTop) / 2.0f;

	// Smallest node: render a single triangle fan
	if (edgeLength <= 3) {
		glBegin(GL_TRIANGLE_FAN);
		RenderVertex(x, z, midX, midZ);
		RenderVertex(x - fEdgeOffset, z - fEdgeOffset, texLeft, texBottom);

		if (((iZ - adjOffset) < 0) || GetQuadMatrixData(iX, iZ - adjOffset) != 0) {
			RenderVertex(x, z - fEdgeOffset, midX, texBottom);
			trisPerFrame++;
		}
		RenderVertex(x + fEdgeOffset, z - fEdgeOffset, texRight, texBottom);
		trisPerFrame++;

		if (((iX + adjOffset) >= size) || GetQuadMatrixData(iX + adjOffset, iZ) != 0) {
			RenderVertex(x + fEdgeOffset, z, texRight, midZ);
			trisPerFrame++;
		}
		RenderVertex(x + fEdgeOffset, z + fEdgeOffset, texRight, texTop);
		trisPerFrame++;

		if (((iZ + adjOffset) >= size) || GetQuadMatrixData(iX, iZ + adjOffset) != 0) {
			RenderVertex(x, z + fEdgeOffset, midX, texTop);
			trisPerFrame++;
		}
		RenderVertex(x - fEdgeOffset, z + fEdgeOffset, texLeft, texTop);
		trisPerFrame++;

		if (((iX - adjOffset) < 0) || GetQuadMatrixData(iX - adjOffset, iZ) != 0) {
			RenderVertex(x - fEdgeOffset, z, texLeft, midZ);
			trisPerFrame++;
		}
		RenderVertex(x - fEdgeOffset, z - fEdgeOffset, texLeft, texBottom);
		trisPerFrame++;
		glEnd();
		return;
	}

	// Larger node: figure out which child fans to render
	int childOffset = (edgeLength - 1) / 4;
	float fChildOffset = (float)childOffset;
	int childEdgeLength = (edgeLength + 1) / 2;

	// Bit code for the fan arrangement (which children are active)
	int code = (GetQuadMatrixData(iX + childOffset, iZ + childOffset) != 0) * 8;
	code |= (GetQuadMatrixData(iX - childOffset, iZ + childOffset) != 0) * 4;
	code |= (GetQuadMatrixData(iX - childOffset, iZ - childOffset) != 0) * 2;
	code |= (GetQuadMatrixData(iX + childOffset, iZ - childOffset) != 0);

	// Four children active: no fan here, recurse into all four
	if (code == QT_NO_FAN) {
		RenderNode(x - fChildOffset, z - fChildOffset, childEdgeLength, texRepeat);
		RenderNode(x + fChildOffset, z - fChildOffset, childEdgeLength, texRepeat);
		RenderNode(x - fChildOffset, z + fChildOffset, childEdgeLength, texRepeat);
		RenderNode(x + fChildOffset, z + fChildOffset, childEdgeLength, texRepeat);
		return;
	}

	// Lower-left and upper-right fans
	if (code == QT_LL_UR) {
		glBegin(GL_TRIANGLE_FAN);
		RenderVertex(x, z, midX, midZ);
		RenderVertex(x + fEdgeOffset, z, texRight, midZ);
		RenderVertex(x + fEdgeOffset, z + fEdgeOffset, texRight, texTop);
		trisPerFrame++;
		RenderVertex(x, z + fEdgeOffset, midX, texTop);
		trisPerFrame++;
		glEnd();

		glBegin(GL_TRIANGLE_FAN);
		RenderVertex(x, z, midX, midZ);
		RenderVertex(x - fEdgeOffset, z, texLeft, midZ);
		RenderVertex(x - fEdgeOffset, z - fEdgeOffset, texLeft, texBottom);
		trisPerFrame++;
		RenderVertex(x, z - fEdgeOffset, midX, texBottom);
		trisPerFrame++;
		glEnd();

		RenderNode(x - fChildOffset, z + fChildOffset, childEdgeLength, texRepeat);
		RenderNode(x + fChildOffset, z - fChildOffset, childEdgeLength, texRepeat);
		return;
	}

	// Lower-right and upper-left fans
	if (code == QT_LR_UL) {
		glBegin(GL_TRIANGLE_FAN);
		RenderVertex(x, z, midX, midZ);
		RenderVertex(x, z + fEdgeOffset, midX, texTop);
		RenderVertex(x - fEdgeOffset, z + fEdgeOffset, texLeft, texTop);
		trisPerFrame++;
		RenderVertex(x - fEdgeOffset, z, texLeft, midZ);
		trisPerFrame++;
		glEnd();

		glBegin(GL_TRIANGLE_FAN);
		RenderVertex(x, z, midX, midZ);
		RenderVertex(x, z - fEdgeOffset, midX, texBottom);
		RenderVertex(x + fEdgeOffset, z - fEdgeOffset, texRight, texBottom);
		trisPerFrame++;
		RenderVertex(x + fEdgeOffset, z, texRight, midZ);
		trisPerFrame++;
		glEnd();

		RenderNode(x + fChildOffset, z + fChildOffset, childEdgeLength, texRepeat);
		RenderNode(x - fChildOffset, z - fChildOffset, childEdgeLength, texRepeat);
		return;
	}

	// Complete fan (leaf node with no active children)
	if (code == QT_COMPLETE_FAN) {
		glBegin(GL_TRIANGLE_FAN);
		RenderVertex(x, z, midX, midZ);
		RenderVertex(x - fEdgeOffset, z - fEdgeOffset, texLeft, texBottom);

		if (((iZ - adjOffset) < 0) || GetQuadMatrixData(iX, iZ - adjOffset) != 0) {
			RenderVertex(x, z - fEdgeOffset, midX, texBottom);
			trisPerFrame++;
		}
		RenderVertex(x + fEdgeOffset, z - fEdgeOffset, texRight, texBottom);
		trisPerFrame++;

		if (((iX + adjOffset) >= size) || GetQuadMatrixData(iX + adjOffset, iZ) != 0) {
			RenderVertex(x + fEdgeOffset, z, texRight, midZ);
			trisPerFrame++;
		}
		RenderVertex(x + fEdgeOffset, z + fEdgeOffset, texRight, texTop);
		trisPerFrame++;

		if (((iZ + adjOffset) >= size) || GetQuadMatrixData(iX, iZ + adjOffset) != 0) {
			RenderVertex(x, z + fEdgeOffset, midX, texTop);
			trisPerFrame++;
		}
		RenderVertex(x - fEdgeOffset, z + fEdgeOffset, texLeft, texTop);
		trisPerFrame++;

		if (((iX - adjOffset) < 0) || GetQuadMatrixData(iX - adjOffset, iZ) != 0) {
			RenderVertex(x - fEdgeOffset, z, texLeft, midZ);
			trisPerFrame++;
		}
		RenderVertex(x - fEdgeOffset, z - fEdgeOffset, texLeft, texBottom);
		trisPerFrame++;
		glEnd();
		return;
	}

	// Partial fans: figure out which fans to render and which children to recurse
	int start = fanStart[code];

	int fanLength = 0;
	while (!(((long)fanCode[code]) & (1 << fanLength)) && fanLength < 8) {
		fanLength++;
	}

	glBegin(GL_TRIANGLE_FAN);
	RenderVertex(x, z, midX, midZ);

	for (int pos = fanLength; pos > 0; pos--) {
		switch (start) {
		case QT_LR_NODE:
			if (((iZ - adjOffset) < 0) || GetQuadMatrixData(iX, iZ - adjOffset) != 0 || pos == fanLength) {
				RenderVertex(x, z - fEdgeOffset, midX, texBottom);
				trisPerFrame++;
			}
			RenderVertex(x + fEdgeOffset, z - fEdgeOffset, texRight, texBottom);
			trisPerFrame++;
			if (pos == 1) {
				RenderVertex(x + fEdgeOffset, z, texRight, midZ);
				trisPerFrame++;
			}
			break;

		case QT_LL_NODE:
			if (((iX - adjOffset) < 0) || GetQuadMatrixData(iX - adjOffset, iZ) != 0 || pos == fanLength) {
				RenderVertex(x - fEdgeOffset, z, texLeft, midZ);
				trisPerFrame++;
			}
			RenderVertex(x - fEdgeOffset, z - fEdgeOffset, texLeft, texBottom);
			trisPerFrame++;
			if (pos == 1) {
				RenderVertex(x, z - fEdgeOffset, midX, texBottom);
				trisPerFrame++;
			}
			break;

		case QT_UL_NODE:
			if (((iZ + adjOffset) >= size) || GetQuadMatrixData(iX, iZ + adjOffset) != 0 || pos == fanLength) {
				RenderVertex(x, z + fEdgeOffset, midX, texTop);
				trisPerFrame++;
			}
			RenderVertex(x - fEdgeOffset, z + fEdgeOffset, texLeft, texTop);
			if (pos == 1) {
				RenderVertex(x - fEdgeOffset, z, texLeft, midZ);
				trisPerFrame++;
			}
			break;

		case QT_UR_NODE:
			if (((iX + adjOffset) >= size) || GetQuadMatrixData(iX + adjOffset, iZ) != 0 || pos == fanLength) {
				RenderVertex(x + fEdgeOffset, z, texRight, midZ);
				trisPerFrame++;
			}
			RenderVertex(x + fEdgeOffset, z + fEdgeOffset, texRight, texTop);
			trisPerFrame++;
			if (pos == 1) {
				RenderVertex(x, z + fEdgeOffset, midX, texTop);
				trisPerFrame++;
			}
			break;
		}

		start--;
		start &= 3;
	}
	glEnd();

	// Recurse down to the children that were not rendered as fans
	for (int pos = (4 - fanLength); pos > 0; pos--) {
		switch (start) {
		case QT_LR_NODE:
			RenderNode(x + fChildOffset, z - fChildOffset, childEdgeLength, texRepeat);
			break;
		case QT_LL_NODE:
			RenderNode(x - fChildOffset, z - fChildOffset, childEdgeLength, texRepeat);
			break;
		case QT_UL_NODE:
			RenderNode(x - fChildOffset, z + fChildOffset, childEdgeLength, texRepeat);
			break;
		case QT_UR_NODE:
			RenderNode(x + fChildOffset, z + fChildOffset, childEdgeLength, texRepeat);
			break;
		}

		start--;
		start &= 3;
	}
}
