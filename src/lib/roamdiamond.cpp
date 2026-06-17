//
// Focus on 3D Terrain Programming
//
// ROAM with a diamond backbone structure.
//
// Original coders: Trent Polack (trent@voxelsoft.com) and Mark Duchaineau
//
// Ported from the original. The OpenGL render uses ARB multitexturing to
// combine the color and detail maps in a single pass (with a two-pass color +
// detail fallback when multitexturing is disabled), matching the book.
//
#include <math.h> // sqrt, pow, fabs
#include <stdio.h> // printf
#include "retrogl.h"
#include "roamdiamond.h"

#define SQR(n) ((n) * (n))
#define CLAMP(value, lo, hi) do { if ((value) < (lo)) (value) = (lo); else if ((value) > (hi)) (value) = (hi); } while (0)

//
// Initialize the ROAM engine
//
void RoamDiamond::Init(int level, int size_, RETRO_Camera *cam)
{
	maxLevel = level;

	// Displacement sizes versus levels. The base diamonds have negative levels
	// (-1, -2), so the table is offset to allow indices down to -2.
	levelMDAlloc = new float[maxLevel + 1 + 2];
	levelMDSize = levelMDAlloc + 2;
	for (int i = -2; i <= maxLevel; i++) {
		levelMDSize[i] = 255.0f / ((float)sqrt((float)pow(2.0, i)));
	}

	// Create the diamond pool and free list
	poolSize = size_;
	pool = new Diamond[poolSize];

	// Start all diamonds on the free list
	for (int i = 0; (i + 1) < poolSize; i++) {
		Diamond *prev = pool + i;
		Diamond *next = pool + (i + 1);
		prev->next = next;
		next->prev = prev;
	}

	freeDmnd[0] = pool;
	freeDmnd[1] = pool + (poolSize - 1);
	freeDmnd[0]->prev = NULL;
	freeDmnd[1]->next = NULL;

	// Mark the diamonds as "free"
	for (int k = 0; k < poolSize; k++) {
		pool[k].boundRad = -1.0f;
		pool[k].lockCount = 0;
	}

	// Allocate, position and link the base-mesh diamonds
	for (int k = 0; k < 25; k++) {
		Diamond *dmnd;
		int i, j;

		if (k < 9) {
			j = k / 3;
			i = k % 3;
			level0Dmnd[j][i] = dmnd = Create();
			dmnd->vert[0] = (2.0f * (float)(i - 1));
			dmnd->vert[2] = (2.0f * (float)(j - 1));
		} else {
			j = (k - 9) / 4;
			i = (k - 9) % 4;
			level1Dmnd[j][i] = dmnd = Create();
			dmnd->vert[0] = (2.0f * (float)i - 3.0f);
			dmnd->vert[2] = (2.0f * (float)j - 3.0f);
		}

		ShiftCoords(&dmnd->vert[0], &dmnd->vert[2]);

		CLAMP(dmnd->vert[0], 0, (size - 1));
		CLAMP(dmnd->vert[2], 0, (size - 1));

		dmnd->vert[1] = GetTrueHeightAtPoint((int)fabs(dmnd->vert[0]), (int)fabs(dmnd->vert[2]));

		dmnd->level = (k < 9 ? 0 : (((i ^ j) & 1) ? -1 : -2));

		dmnd->boundRad = (float)SQR(size);
		dmnd->errorRad = (float)size;

		dmnd->parent[0] = dmnd->parent[1] = dmnd->parent[2] = dmnd->parent[3] = NULL;
		dmnd->child[0] = dmnd->child[1] = dmnd->child[2] = dmnd->child[3] = NULL;
	}

	// Set the level-0 diamonds' links
	for (int k = 0; k < 9; k++) {
		int j = k / 3;
		int i = k % 3;

		Diamond *dmnd = level0Dmnd[j][i];
		int di = (((i ^ j) & 1) ? 1 : -1);
		int dj = 1;
		int ix, jx;

		ix = (2 * i + 1 - di) >> 1;
		jx = (2 * j + 1 - dj) >> 1;
		dmnd->parent[0] = level1Dmnd[jx][ix];

		ix = (2 * i + 1 + di) >> 1;
		jx = (2 * j + 1 + dj) >> 1;
		dmnd->parent[1] = level1Dmnd[jx][ix];

		ix = (2 * i + 1 - dj) >> 1;
		jx = (2 * j + 1 + di) >> 1;
		dmnd->parent[2] = level1Dmnd[jx][ix];

		ix = (2 * i + 1 + dj) >> 1;
		jx = (2 * j + 1 - di) >> 1;
		dmnd->parent[3] = level1Dmnd[jx][ix];

		ix = (di < 0 ? 0 : 3);
		dmnd->parent[0]->child[ix] = dmnd;
		dmnd->childIndex[0] = ix;

		ix = (di < 0 ? 2 : 1);
		dmnd->parent[1]->child[ix] = dmnd;
		dmnd->childIndex[1] = ix;
	}

	// Configure the level-1 diamond nodes' parents
	for (int k = 0; k < 16; k++) {
		int j = k / 4;
		int i = k % 4;

		Diamond *dmnd = level1Dmnd[j][i];
		if (j > 0) dmnd->parent[3] = level1Dmnd[j - 1][i];
		if (j < 3) dmnd->parent[2] = level1Dmnd[j + 1][i];
		if (i > 0) dmnd->parent[0] = level1Dmnd[j][i - 1];
		if (i < 3) dmnd->parent[1] = level1Dmnd[j][i + 1];
	}

	camera = cam;
}

//
// Shutdown the ROAM engine
//
void RoamDiamond::Shutdown(void)
{
	delete[] pool;
	delete[] levelMDAlloc;
	pool = NULL;
	levelMDAlloc = NULL;
}

//
// Render the ROAM engine
//
void RoamDiamond::Render(void)
{
	vertsPerFrame = 0;
	trisPerFrame = 0;

	bool doTexture = textureMapping && texture.IsLoaded();
	bool doDetail = detailMapping && detailMap.IsLoaded();

	// No texturing: render once with vertex colors only
	if (!doTexture && !doDetail) {
		glDisable(GL_TEXTURE_2D);
		glBegin(GL_TRIANGLES);
		RenderChild(level1Dmnd[1][2], 0, 0, 1.0f);
		RenderChild(level1Dmnd[2][1], 2, 0, 1.0f);
		glEnd();
		return;
	}

	// Single-pass ARB multitexturing: color map * detail map at once
	if (multitexture && doTexture && doDetail) {
		BeginMultitexture();
		glBegin(GL_TRIANGLES);
		RenderChild(level1Dmnd[1][2], 0, 0, 1.0f);
		RenderChild(level1Dmnd[2][1], 2, 0, 1.0f);
		glEnd();
		EndMultitexture();
		return;
	}

	// Color texture pass
	if (doTexture) {
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, texture.GetID());
		glBegin(GL_TRIANGLES);
		RenderChild(level1Dmnd[1][2], 0, 0, 1.0f);
		RenderChild(level1Dmnd[2][1], 2, 0, 1.0f);
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
		RenderChild(level1Dmnd[1][2], 0, 0, (float)repeatDetailMap);
		RenderChild(level1Dmnd[2][1], 2, 0, (float)repeatDetailMap);
		glEnd();

		glDisable(GL_BLEND);
	}
}

//
// Render a child triangle of a diamond (recursively splitting as needed)
//
void RoamDiamond::RenderChild(Diamond *dmnd, int index, int cull, float texRepeat)
{
	// Get the child diamond that needs to be rendered
	Diamond *centerDmnd = GetChild(dmnd, index);
	float *center = centerDmnd->vert;

	// Check the triangle's bounding sphere against the view frustum
	if (cull != CULL_ALLIN) {
		float (*frustum)[4] = camera->GetViewFrustum();

		for (int j = 0, m = 1; j < 6; j++, m <<= 1) {
			if (!(cull & m)) {
				float r = frustum[j][0] * center[0] +
					frustum[j][1] * center[1] +
					frustum[j][2] * center[2] +
					frustum[j][3];

				if (SQR(r) > centerDmnd->boundRad) {
					if (r < 0.0f) {
						// Fully outside the frustum
						Unlock(centerDmnd);
						return;
					}
					cull |= m;	// Fully inside this plane
				}
			}
		}
	}

	// Distance from the camera to the triangle's center
	float dist = SQR(center[0] - camera->head[0]) +
		SQR(center[1] - camera->head[1]) +
		SQR(center[2] - camera->head[2]);

	// If not at max level and the error is large on screen, split
	if (centerDmnd->level < maxLevel && centerDmnd->errorRad > dist * 0.00001f) {
		if (centerDmnd->parent[0] == dmnd) {
			RenderChild(centerDmnd, 0, cull, texRepeat);
			RenderChild(centerDmnd, 1, cull, texRepeat);
		} else {
			RenderChild(centerDmnd, 2, cull, texRepeat);
			RenderChild(centerDmnd, 3, cull, texRepeat);
		}
	} else {
		// Draw the current triangle (leaf node)
		Diamond *prevDmnd, *nextDmnd;
		if (centerDmnd->parent[0] == dmnd) {
			prevDmnd = centerDmnd->parent[2];
			nextDmnd = centerDmnd->parent[3];
		} else {
			prevDmnd = centerDmnd->parent[3];
			nextDmnd = centerDmnd->parent[2];
		}

		unsigned char shade = GetBrightnessAtPoint((int)prevDmnd->vert[0], (int)prevDmnd->vert[2]);
		glColor3ub(shade, shade, shade);
		EmitTexCoord(prevDmnd->vert[0] / size * texRepeat, prevDmnd->vert[2] / size * texRepeat);
		glVertex3fv(prevDmnd->vert);

		shade = GetBrightnessAtPoint((int)dmnd->vert[0], (int)dmnd->vert[2]);
		glColor3ub(shade, shade, shade);
		EmitTexCoord(dmnd->vert[0] / size * texRepeat, dmnd->vert[2] / size * texRepeat);
		glVertex3fv(dmnd->vert);

		shade = GetBrightnessAtPoint((int)nextDmnd->vert[0], (int)nextDmnd->vert[2]);
		glColor3ub(shade, shade, shade);
		EmitTexCoord(nextDmnd->vert[0] / size * texRepeat, nextDmnd->vert[2] / size * texRepeat);
		glVertex3fv(nextDmnd->vert);

		vertsPerFrame += 3;
		trisPerFrame++;
	}

	// Unlock the diamond, we are done with it
	Unlock(centerDmnd);
}

//
// Get a child of a diamond, creating it (and its sibling parent) if needed
//
Diamond *RoamDiamond::GetChild(Diamond *dmnd, int index)
{
	// No need to create the child if it already exists
	if (dmnd->child[index]) {
		Lock(dmnd->child[index]);
		return dmnd->child[index];
	}

	// Lock the center diamond early to avoid an automatic "diamond discharge"
	Lock(dmnd);

	// Allocate a new child
	Diamond *child = Create();

	// Recursively create the other parent of the child
	Diamond *px;
	int ix;
	if (index < 2) {
		px = dmnd->parent[0];
		ix = (dmnd->childIndex[0] + (index == 0 ? 1 : -1)) & 3;
	} else {
		px = dmnd->parent[1];
		ix = (dmnd->childIndex[1] + (index == 2 ? 1 : -1)) & 3;
	}

	Diamond *cx = GetChild(px, ix);

	// Set all the links
	dmnd->child[index] = child;
	ix = (index & 1) ^ 1;
	if (cx->parent[1] == px) {
		ix |= 2;
	}

	cx->child[ix] = child;
	if (index & 1) {
		child->parent[0] = cx;
		child->childIndex[0] = ix;
		child->parent[1] = dmnd;
		child->childIndex[1] = index;
	} else {
		child->parent[0] = dmnd;
		child->childIndex[0] = index;
		child->parent[1] = cx;
		child->childIndex[1] = ix;
	}

	child->parent[2] = dmnd->parent[index >> 1];
	child->parent[3] = dmnd->parent[(((index + 1) & 2) >> 1) + 2];

	child->child[0] = child->child[1] = child->child[2] = child->child[3] = NULL;

	// Calculate the child's level and vertex information
	child->level = dmnd->level + 1;

	float *parentVert0 = child->parent[2]->vert;
	float *parentVert1 = child->parent[3]->vert;
	child->vert[0] = (float)fabs((parentVert0[0] + parentVert1[0]) / 2.0f);
	child->vert[2] = (float)fabs((parentVert0[2] + parentVert1[2]) / 2.0f);

	CLAMP(child->vert[0], 0, (size - 1));
	CLAMP(child->vert[2], 0, (size - 1));

	child->vert[1] = GetTrueHeightAtPoint((int)child->vert[0], (int)child->vert[2]);

	float *center = child->vert;

	// Compute the squared radius of the diamond's bounding sphere (max squared
	// distance of any of the four parents to the center vertex)
	float *temp = child->parent[0]->vert;
	float sqrBound = SQR(temp[0] - center[0]) + SQR(temp[1] - center[1]) + SQR(temp[2] - center[2]);

	temp = child->parent[1]->vert;
	float sqrBoundTemp = SQR(temp[0] - center[0]) + SQR(temp[1] - center[1]) + SQR(temp[2] - center[2]);
	if (sqrBoundTemp > sqrBound) sqrBound = sqrBoundTemp;

	temp = child->parent[2]->vert;
	sqrBoundTemp = SQR(temp[0] - center[0]) + SQR(temp[1] - center[1]) + SQR(temp[2] - center[2]);
	if (sqrBoundTemp > sqrBound) sqrBound = sqrBoundTemp;

	temp = child->parent[3]->vert;
	sqrBoundTemp = SQR(temp[0] - center[0]) + SQR(temp[1] - center[1]) + SQR(temp[2] - center[2]);
	if (sqrBoundTemp > sqrBound) sqrBound = sqrBoundTemp;

	child->boundRad = sqrBound;
	child->errorRad = SQR(levelMDSize[(int)child->level]);

	return child;
}

//
// Create a new diamond (from the head of the free list)
//
Diamond *RoamDiamond::Create(void)
{
	Diamond *dmnd = freeDmnd[0];
	if (!dmnd) {
		printf("[ERROR] RoamDiamond::Create() Out of memory (diamond pool exhausted)\n");
		return NULL;
	}

	// If the diamond has been used before, delete its previous parent links
	if (dmnd->boundRad >= 0.0f) {
		dmnd->parent[0]->child[(int)dmnd->childIndex[0]] = NULL;
		Unlock(dmnd->parent[0]);

		dmnd->parent[1]->child[(int)dmnd->childIndex[1]] = NULL;
		Unlock(dmnd->parent[1]);
	}

	Lock(dmnd);
	return dmnd;
}

//
// Lock a diamond to prevent a "diamond discharge"
//
void RoamDiamond::Lock(Diamond *dmnd)
{
	// Remove from the free list on the first reference
	if (dmnd->lockCount == 0) {
		Diamond *prev = dmnd->prev;
		Diamond *next = dmnd->next;

		if (prev) {
			prev->next = next;
		} else {
			freeDmnd[0] = next;
		}

		if (next) {
			next->prev = prev;
		} else {
			freeDmnd[1] = prev;
		}
	}

	dmnd->lockCount++;
}

//
// Unlock a diamond so it may be reused by others
//
void RoamDiamond::Unlock(Diamond *dmnd)
{
	dmnd->lockCount--;

	// Add back to the free list when there are no references left
	if (dmnd->lockCount == 0) {
		Diamond *prev = freeDmnd[1];

		dmnd->prev = prev;
		dmnd->next = NULL;

		if (prev) {
			prev->next = dmnd;
		} else {
			freeDmnd[0] = dmnd;
		}

		freeDmnd[1] = dmnd;
	}
}
