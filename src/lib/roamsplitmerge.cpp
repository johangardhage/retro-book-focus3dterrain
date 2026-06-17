//
// Focus on 3D Terrain Programming
//
// Full ROAM with split/merge priority queues.
//
// Original coders: Trent Polack (trent@voxelsoft.com) and Mark Duchaineau
//
// Ported from the original. Rendering is unchanged from the book: the mesh is
// drawn from an interleaved vertex/texcoord array with a single color texture
// (the book dropped lighting/detail mapping from this demo for simplicity).
//
#include <math.h> // sqrt, log, floor, fabs
#include <stdio.h> // printf
#include <stdlib.h> // exit
#include <string.h> // memcpy
#include "retrogl.h"
#include "roamsplitmerge.h"

#define SQR(n) ((n) * (n))
#define CLAMP(value, lo, hi) do { if ((value) < (lo)) (value) = (lo); else if ((value) > (hi)) (value) = (hi); } while (0)

//
// Initialize the ROAM engine
//
void RoamSplitMerge::Init(int level, int size_, RETRO_Camera *cam)
{
	queueCoarse = 1990;	// "Magic number" for the queue fineness
	frameCount = 0;

	maxLevel = level;

	// Displacement sizes versus levels (offset table so indices -2..maxLevel
	// are valid; the base diamonds use negative levels)
	levelMDAlloc = new float[maxLevel + 1 + 2];
	levelMDSize = levelMDAlloc + 2;
	for (int i = -2; i <= maxLevel; i++) {
		levelMDSize[i] = 255.0f / ((float)sqrt((double)((long long)1 << (i + 2)) / 4.0));
	}

	// Diamond pool and free list
	poolSize = size_;
	pool = new SMDiamond[poolSize];

	maxTriChunks = TRI_IMAX;
	dmndIS = new int[maxTriChunks];

	// Memory for the interleaved vertex/texcoord buffer (15 floats per tri)
	vertTexBuffer = new float[maxTriChunks * 15];

	// Start all diamonds on the free list
	for (int i = 0; i + 1 < poolSize; i++) {
		SMDiamond *a = pool + i;
		SMDiamond *b = pool + (i + 1);
		a->next = b;
		b->prev = a;
	}
	freeDmnd[0] = pool;
	freeDmnd[1] = pool + (poolSize - 1);
	freeDmnd[0]->prev = NULL;
	freeDmnd[1]->next = NULL;
	freeElements = poolSize;

	// Initialize diamonds to NEW and FREE
	for (int i = 0; i < poolSize; i++) {
		SMDiamond *d = pool + i;
		d->boundRad = -1;		// Indicates a NEW diamond
		d->lockCount = 0;
		d->flags = 0;
		d->frameCount = 255;
		d->parent[2] = d->parent[3] = NULL;
		d->cull = 0;
		d->childIndex[0] = d->childIndex[1] = 0;
		d->queueIndex = IQMAX / 2;
		d->child[0] = d->child[1] = d->child[2] = d->child[3] = NULL;
		d->level = -100;
		d->parent[0] = d->parent[1] = NULL;
		d->errorRad = 10.0f;
	}

	// Clear the priority queues
	for (int i = 0; i < IQMAX; i++) {
		splitQueue[i] = mergeQueue[i] = NULL;
	}
	pqMax = -1;
	pqMin = IQMAX;

	// Clear the triangle render list
	for (int i = 0; i < maxTriChunks; i++) {
		dmndIS[i] = -1;
	}
	freeTri = 1;
	trisPerFrame = 0;
	freeTriCount = maxTriChunks - 1;
	maxTris = 30000;	// Target visible triangle count

	// Generate the float->int log2 correction table
	for (int i = 0; i < 256; i++) {
		unsigned int bits = 0x3f800000 + (i << 15);
		float f;
		memcpy(&f, &bits, sizeof(f));
		log2Table[i] = (int)floor(size * (log(f) / log(2.0) - (float)i / 256.0) + 0.5f) << 12;
	}

	// Allocate base diamonds (positions and info, links set below)
	for (int k = 0; k < 32; k++) {
		SMDiamond *dmnd;
		int i, j;

		if (k < 16) {
			j = k / 4;
			i = k % 4;
			level0Dmnd[j][i] = dmnd = Create();
			dmnd->vert[0] = (2.0f * (float)(i - 1));
			dmnd->vert[2] = (2.0f * (float)(j - 1));
		} else {
			j = (k - 16) / 4;
			i = (k - 16) % 4;
			level1Dmnd[j][i] = dmnd = Create();
			dmnd->vert[0] = (2.0f * (float)i - 3.0f);
			dmnd->vert[2] = (2.0f * (float)j - 3.0f);
		}

		ShiftCoords(&dmnd->vert[0], &dmnd->vert[2]);

		CLAMP(dmnd->vert[0], 0, (size - 1));
		CLAMP(dmnd->vert[2], 0, (size - 1));

		dmnd->vert[1] = (float)GetTrueHeightAtPoint((int)(fabs(dmnd->vert[0])), (int)(fabs(dmnd->vert[2])));
		dmnd->triIndex[0] = dmnd->triIndex[1] = 0;

		dmnd->boundRad = (float)SQR(size);
		dmnd->errorRad = (float)size;

		dmnd->parent[0] = dmnd->parent[1] = dmnd->parent[2] = dmnd->parent[3] = NULL;
		dmnd->child[0] = dmnd->child[1] = dmnd->child[2] = dmnd->child[3] = NULL;

		dmnd->level = (k < 16 ? 0 : (((i ^ j) & 1) ? -1 : -2));
		dmnd->cull = 0;
		dmnd->flags = 0;
		dmnd->splitFlags = 0;
		dmnd->queueIndex = IQMAX - 1;

		if (k < 16 && k != 5) {
			dmnd->flags |= ROAM_CLIPPED;
		}
		if (dmnd->level < 0) {
			dmnd->flags |= ROAM_SPLIT;
		}
	}

	// Links for the level-0 diamonds
	for (int k = 0; k < 16; k++) {
		int j = k / 4;
		int i = k % 4;

		SMDiamond *dmnd = level0Dmnd[j][i];
		int di = (((i ^ j) & 1) ? 1 : -1);
		int dj = 1;
		int ix, jx;

		ix = ((2 * i + 1 - di) >> 1) % 4;
		jx = ((2 * j + 1 - dj) >> 1) % 4;
		dmnd->parent[0] = level1Dmnd[jx][ix];

		ix = ((2 * i + 1 + di) >> 1) % 4;
		jx = ((2 * j + 1 + dj) >> 1) % 4;
		dmnd->parent[1] = level1Dmnd[jx][ix];

		ix = ((2 * i + 1 - dj) >> 1) % 4;
		jx = ((2 * j + 1 + di) >> 1) % 4;
		dmnd->parent[2] = level1Dmnd[jx][ix];

		ix = ((2 * i + 1 + dj) >> 1) % 4;
		jx = ((2 * j + 1 - di) >> 1) % 4;
		dmnd->parent[3] = level1Dmnd[jx][ix];

		ix = (di < 0 ? 0 : 3);
		dmnd->parent[0]->child[ix] = dmnd;
		dmnd->childIndex[0] = ix;

		ix = (di < 0 ? 2 : 1);
		dmnd->parent[1]->child[ix] = dmnd;
		dmnd->childIndex[1] = ix;
	}

	// Links for the level-1 diamonds (wrap around)
	for (int k = 0; k < 16; k++) {
		int j = k / 4;
		int i = k % 4;

		SMDiamond *dmnd = level1Dmnd[j][i];
		dmnd->parent[3] = level1Dmnd[(j + 3) % 4][i];
		dmnd->parent[2] = level1Dmnd[(j + 1) % 4][i];
		dmnd->parent[0] = level1Dmnd[j][(i + 3) % 4];
		dmnd->parent[1] = level1Dmnd[j][(i + 1) % 4];
	}

	// Put the top-level diamond on the split queue
	SMDiamond *top = level0Dmnd[1][1];
	Enqueue(top, ROAM_SPLITQ, IQMAX - 1);

	// Allocate the base triangles
	AllocateTri(top, 0);
	AllocateTri(top, 1);

	camera = cam;
}

//
// Shutdown the ROAM engine
//
void RoamSplitMerge::Shutdown(void)
{
	delete[] vertTexBuffer;
	delete[] dmndIS;
	delete[] pool;
	delete[] levelMDAlloc;
	vertTexBuffer = NULL;
	dmndIS = NULL;
	pool = NULL;
	levelMDAlloc = NULL;
}

//
// Update the engine's mesh (process the split/merge queues)
//
void RoamSplitMerge::Update(void)
{
	static int i0 = 0;
	SMDiamond *dmnd;

	// Recursive culling update over the active diamonds
	dmnd = level0Dmnd[1][1];
	UpdateChildCull(dmnd);
	for (int i = 0; i < 4; i++) {
		if (dmnd->child[i]) {
			UpdateChildCull(dmnd->child[i]);
		}
	}

	// Update a tenth of the queued diamonds' priorities each frame
	int i1 = i0 + (poolSize + 9) / 10;
	if (i1 >= poolSize) {
		i1 = poolSize - 1;
	}
	for (int i = i0; i <= i1; i++) {
		dmnd = pool + i;
		if (dmnd->flags & ROAM_ALLQ) {
			UpdatePriority(dmnd);
		}
	}
	i0 = (i1 + 1) % poolSize;

	// Keep splitting/merging until the target is met or limits are hit
	int maxOptCount = 2000;
	int optCount = 0;

	int side;
	if (trisPerFrame <= maxTris && pqMax >= queueCoarse && freeElements > 128 && freeTriCount > 128) {
		side = -1;
	} else {
		side = 1;
	}

	int overlap0 = pqMax - pqMin;

	while ((side != 0 || overlap0 > 1) && optCount < maxOptCount) {
		if (side <= 0) {
			if (pqMax > 0) {
				Split(splitQueue[pqMax]);
				if (!(trisPerFrame <= maxTris && pqMax >= queueCoarse && freeElements > 128 && freeTriCount > 128)) {
					side = 1;
				}
			} else {
				side = 0;
			}
		} else {
			Merge(mergeQueue[pqMin]);
			if (trisPerFrame <= maxTris && pqMax >= queueCoarse && freeElements > 128 && freeTriCount > 128) {
				side = 0;
			}
		}

		int overlap = pqMax - pqMin;
		if (overlap < overlap0) {
			overlap0 = overlap;
		}

		optCount++;
	}

	frameCount = (frameCount + 1) & 255;
}

//
// Render the ROAM engine (interleaved vertex/texcoord array, color texture)
//
void RoamSplitMerge::Render(void)
{
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture.GetID());

	// The first 15-float chunk (index 0) is reserved; data starts at index 1
	float *vb = vertTexBuffer + 15;

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glVertexPointer(3, GL_FLOAT, 20, vb + 2);
	glTexCoordPointer(2, GL_FLOAT, 20, vb);
	glDrawArrays(GL_TRIANGLES, 0, 3 * (freeTri - 1));

	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);

	glDisable(GL_TEXTURE_2D);
}

//
// Allocate a triangle for the render list
//
void RoamSplitMerge::AllocateTri(SMDiamond *dmnd, int j)
{
	// CLIPPED diamonds never have triangles
	if (dmnd->flags & ROAM_CLIPPED) {
		return;
	}

	// CLIPPED parent j means no triangle on side j
	if (dmnd->parent[j]->flags & ROAM_CLIPPED) {
		return;
	}

	// Mark the triangle on side j as active
	dmnd->flags |= ROAM_TRI0 << j;

	// If not OUT, add the triangle to the render list
	if (!(dmnd->cull & CULL_OUT)) {
		AddTri(dmnd, j);
	}
}

//
// Free a triangle from the render list
//
void RoamSplitMerge::FreeTri(SMDiamond *dmnd, int j)
{
	if (dmnd->flags & ROAM_CLIPPED) {
		return;
	}
	if (dmnd->parent[j]->flags & ROAM_CLIPPED) {
		return;
	}

	// Mark the triangle on side j as inactive
	dmnd->flags &= ~(ROAM_TRI0 << j);

	if (!(dmnd->cull & CULL_OUT)) {
		RemoveTri(dmnd, j);
	}
}

//
// Add a triangle to the render list
//
void RoamSplitMerge::AddTri(SMDiamond *dmnd, int j)
{
	SMDiamond *table[3];

	// Grab a free triangle chunk
	int i = freeTri++;
	if (i >= maxTriChunks) {
		printf("[ERROR] RoamSplitMerge::AddTri() Out of triangle memory\n");
		exit(1);
	}
	freeTriCount--;
	dmnd->triIndex[j] = i;
	dmndIS[i] = ((int)(dmnd - pool) << 1) | j;

	// The triangle's three vertices
	table[1] = dmnd->parent[j];
	if (j) {
		table[0] = dmnd->parent[3];
		table[2] = dmnd->parent[2];
	} else {
		table[0] = dmnd->parent[2];
		table[2] = dmnd->parent[3];
	}

	// Fill the interleaved vertex/texcoord buffer
	float *vb = vertTexBuffer + 15 * i;
	for (int vi = 0; vi < 3; vi++, vb += 5) {
		vb[2] = table[vi]->vert[0];
		vb[3] = table[vi]->vert[1];
		vb[4] = table[vi]->vert[2];

		vb[0] = vb[2] / size;
		vb[1] = vb[4] / size;
	}

	vertsPerFrame += 3;
	trisPerFrame++;
}

//
// Remove a triangle from the render list
//
void RoamSplitMerge::RemoveTri(SMDiamond *dmnd, int j)
{
	int i = dmnd->triIndex[j];

	// Put the triangle chunk back on the free list
	dmnd->triIndex[j] = 0;
	freeTri--;
	freeTriCount++;

	// Move the last active triangle into the freed slot
	int ix = freeTri;
	int is = dmndIS[ix];
	int jx = is & 1;
	SMDiamond *dmndX = pool + (is >> 1);

	dmndX->triIndex[jx] = i;
	dmndIS[i] = is;

	memcpy(vertTexBuffer + 15 * i, vertTexBuffer + 15 * ix, 15 * sizeof(float));

	vertsPerFrame -= 3;
	trisPerFrame--;
}

//
// Create a new diamond (recycle the least-recently-used free diamond)
//
SMDiamond *RoamSplitMerge::Create(void)
{
	SMDiamond *dmnd = freeDmnd[0];
	if (!dmnd) {
		printf("[ERROR] RoamSplitMerge::Create() Out of ROAM diamond storage\n");
		exit(1);
	}

	// If the diamond is not NEW, reset its links
	if (dmnd->boundRad >= 0.0f) {
		dmnd->parent[0]->child[(int)dmnd->childIndex[0]] = NULL;
		Unlock(dmnd->parent[0]);

		dmnd->parent[1]->child[(int)dmnd->childIndex[1]] = NULL;
		Unlock(dmnd->parent[1]);
		dmnd->queueIndex = IQMAX >> 1;
	} else {
		dmnd->boundRad = 0.0f;	// Mark the diamond as used (no longer NEW)
	}

	// Make sure the frame count is old so updates will run
	dmnd->frameCount = (frameCount - 1) & 255;

	Lock(dmnd);
	return dmnd;
}

//
// Get a child of a diamond, creating it (and its sibling parent) if needed
//
SMDiamond *RoamSplitMerge::GetChild(SMDiamond *dmnd, int i)
{
	SMDiamond *k;

	// If the child is already alive, return it
	if ((k = dmnd->child[i])) {
		Lock(k);
		return k;
	}

	// Lock the center diamond to prevent early recycling
	Lock(dmnd);

	// Recursively create the other parent of the child
	SMDiamond *parentX;
	int ix;
	if (i < 2) {
		parentX = dmnd->parent[0];
		ix = (dmnd->childIndex[0] + (i == 0 ? 1 : -1)) & 3;
	} else {
		parentX = dmnd->parent[1];
		ix = (dmnd->childIndex[1] + (i == 2 ? 1 : -1)) & 3;
	}

	SMDiamond *childX = GetChild(parentX, ix);

	// Create a new child and lock it
	k = Create();

	// Set all of the child's links
	dmnd->child[i] = k;
	ix = (i & 1) ^ 1;
	if (childX->parent[1] == parentX) {
		ix |= 2;
	}

	childX->child[ix] = k;
	if (i & 1) {
		k->parent[0] = childX;
		k->childIndex[0] = ix;
		k->parent[1] = dmnd;
		k->childIndex[1] = i;
	} else {
		k->parent[0] = dmnd;
		k->childIndex[0] = i;
		k->parent[1] = childX;
		k->childIndex[1] = ix;
	}
	k->parent[2] = dmnd->parent[i >> 1];
	k->parent[3] = dmnd->parent[(((i + 1) & 2) >> 1) + 2];
	k->child[0] = k->child[1] = k->child[2] = k->child[3] = NULL;

	// Child information
	k->cull = 0;
	k->flags = 0;
	k->splitFlags = 0;
	if ((k->parent[2]->flags & ROAM_CLIPPED) ||
		((dmnd->flags & ROAM_CLIPPED) && (childX->flags & ROAM_CLIPPED))) {
		k->flags |= ROAM_CLIPPED;
	}
	k->triIndex[0] = k->triIndex[1] = 0;
	k->queueIndex = -10;
	k->level = dmnd->level + 1;

	float *center = k->vert;

	float *parentVert0 = k->parent[2]->vert;
	float *parentVert1 = k->parent[3]->vert;
	k->vert[0] = (float)fabs((parentVert0[0] + parentVert1[0]) / 2.0f);
	k->vert[2] = (float)fabs((parentVert0[2] + parentVert1[2]) / 2.0f);
	k->vert[1] = GetTrueHeightAtPoint((int)k->vert[0], (int)k->vert[2]);

	// Squared bounding-sphere radius (max squared distance of any parent to
	// the center vertex)
	float *temp = k->parent[0]->vert;
	float sqrBound = SQR(temp[0] - center[0]) + SQR(temp[1] - center[1]) + SQR(temp[2] - center[2]);

	temp = k->parent[1]->vert;
	float sqrBoundTemp = SQR(temp[0] - center[0]) + SQR(temp[1] - center[1]) + SQR(temp[2] - center[2]);
	if (sqrBoundTemp > sqrBound) sqrBound = sqrBoundTemp;

	temp = k->parent[2]->vert;
	sqrBoundTemp = SQR(temp[0] - center[0]) + SQR(temp[1] - center[1]) + SQR(temp[2] - center[2]);
	if (sqrBoundTemp > sqrBound) sqrBound = sqrBoundTemp;

	temp = k->parent[3]->vert;
	sqrBoundTemp = SQR(temp[0] - center[0]) + SQR(temp[1] - center[1]) + SQR(temp[2] - center[2]);
	if (sqrBoundTemp > sqrBound) sqrBound = sqrBoundTemp;

	k->boundRad = sqrBound;
	k->errorRad = SQR(levelMDSize[(int)k->level]);

	return k;
}

//
// Lock a diamond to prevent a "diamond discharge"
//
void RoamSplitMerge::Lock(SMDiamond *dmnd)
{
	if (dmnd->lockCount == 0) {
		SMDiamond *prev = dmnd->prev;
		SMDiamond *next = dmnd->next;

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

		freeElements--;
	}

	dmnd->lockCount++;
}

//
// Unlock a diamond so it may be reused
//
void RoamSplitMerge::Unlock(SMDiamond *dmnd)
{
	dmnd->lockCount--;

	if (dmnd->lockCount == 0) {
		SMDiamond *prev = freeDmnd[1];

		dmnd->prev = prev;
		dmnd->next = NULL;

		if (prev) {
			prev->next = dmnd;
		} else {
			freeDmnd[0] = dmnd;
		}

		freeDmnd[1] = dmnd;
		freeElements++;
	}
}

//
// Update a child's culling flag (recursively)
//
void RoamSplitMerge::UpdateChildCull(SMDiamond *dmnd)
{
	if (dmnd->flags & ROAM_CLIPPED) {
		return;
	}

	int cull = dmnd->cull;	// Save old culling flag for comparison
	UpdateCull(dmnd);

	// Skip the subtree if nothing has really changed
	if (cull == dmnd->cull && (cull == CULL_OUT || cull == CULL_ALLIN)) {
		return;
	}

	// Update priority if the OUT state has changed
	if ((cull ^ dmnd->cull) & CULL_OUT) {
		UpdatePriority(dmnd);
	}

	// Recurse into children if split
	if (dmnd->flags & ROAM_SPLIT) {
		for (int i = 0; i < 4; i += 2) {
			SMDiamond *child = dmnd->child[i];
			if (child) {
				if (child->parent[0] == dmnd) {
					if (child->child[0]) UpdateChildCull(child->child[0]);
					if (child->child[1]) UpdateChildCull(child->child[1]);
				} else {
					if (child->child[2]) UpdateChildCull(child->child[2]);
					if (child->child[3]) UpdateChildCull(child->child[3]);
				}
			}
		}
	}
}

//
// Update a diamond's culling flag against the view frustum
//
void RoamSplitMerge::UpdateCull(SMDiamond *dmnd)
{
	float (*frustum)[4] = camera->GetViewFrustum();

	int cull = dmnd->parent[2]->cull;

	if (cull != CULL_ALLIN && cull != CULL_OUT) {
		for (int j = 0, m = 1; j < 6; j++, m <<= 1) {
			if (!(cull & m)) {
				float r = frustum[j][0] * dmnd->vert[0] +
					frustum[j][1] * dmnd->vert[1] +
					frustum[j][2] * dmnd->vert[2] +
					frustum[j][3];

				if (SQR(r) > dmnd->boundRad) {
					if (r < 0.0f) {
						cull = CULL_OUT;
					} else {
						cull |= m;	// IN with respect to this plane
					}
				}
			}
		}
	}

	// If the OUT state changes, update the in/out listing of any drawn tris
	if ((dmnd->cull ^ cull) & CULL_OUT) {
		for (int j = 0; j < 2; j++) {
			if (dmnd->flags & (ROAM_TRI0 << j)) {
				if (cull & CULL_OUT) {
					RemoveTri(dmnd, j);
				} else {
					AddTri(dmnd, j);
				}
			}
		}
	}

	dmnd->cull = cull;
}

//
// Update a diamond's priority in the split/merge queues
//
void RoamSplitMerge::UpdatePriority(SMDiamond *dmnd)
{
	// Skip if already updated this frame
	if (frameCount == dmnd->frameCount) {
		return;
	}
	dmnd->frameCount = frameCount;

	int k;
	if ((dmnd->flags & ROAM_CLIPPED) || dmnd->level >= maxLevel) {
		k = 0;
	} else {
		// Fixed-point log_2 of the error metric (IEEE float-bit trick)
		float d = dmnd->errorRad;
		memcpy(&k, &d, sizeof(k));
		k += log2Table[(k >> 15) & 0xff];

		// Distance to the camera
		d = SQR(dmnd->vert[0] - camera->head[0]) +
			SQR(dmnd->vert[1] - camera->head[1]) +
			SQR(dmnd->vert[2] - camera->head[2]);

		int j;
		memcpy(&j, &d, sizeof(j));
		j += log2Table[(j >> 15) & 0xff];

		// Fixed-point log_2(error/distance)
		k = (k - j) + 0x10000000;

		if (k < 0) {
			k = 0;
		}
		k = (k >> 16) + 1;
		if (k >= IQMAX) {
			k = IQMAX - 1;
		}

		// For OUT diamonds, reduce priority (but keep them ordered)
		if (dmnd->cull & CULL_OUT) {
			if (k > size) {
				k -= (size / 2);
			} else {
				k = (k + 1) >> 1;
			}
		}
	}

	Enqueue(dmnd, dmnd->flags & ROAM_ALLQ, k);
}

//
// Re-queue a diamond in the appropriate priority queue
//
void RoamSplitMerge::Enqueue(SMDiamond *dmnd, int queueFlags, int newIndex)
{
	SMDiamond **queue, *dmndX;

	// Return early if the diamond is already where it should be
	if ((dmnd->flags & ROAM_ALLQ) == queueFlags && dmnd->queueIndex == newIndex) {
		return;
	}

	// Net change in the diamond's lock count
	int lockDelta = 0;
	if (dmnd->flags & ROAM_ALLQ) lockDelta--;
	if (queueFlags & ROAM_ALLQ) lockDelta++;

	// Remove from the old queue if needed
	if (dmnd->flags & ROAM_ALLQ) {
		queue = ((dmnd->flags & ROAM_SPLITQ) ? splitQueue : mergeQueue);
		if (dmnd->prev) {
			dmnd->prev->next = dmnd->next;
		} else {
			queue[dmnd->queueIndex] = dmnd->next;
			if (!dmnd->next) {
				if (dmnd->flags & ROAM_SPLITQ) {
					if (dmnd->queueIndex == pqMax) {
						dmndX = queue[0];
						queue[0] = (SMDiamond *)1;
						int i;
						for (i = dmnd->queueIndex; !queue[i]; i--)
							;
						if (!(queue[0] = dmndX) && i == 0) {
							i--;
						}
						pqMax = i;
					}
				} else {
					if (dmnd->queueIndex == pqMin) {
						dmndX = queue[IQMAX - 1];
						queue[IQMAX - 1] = (SMDiamond *)1;
						int i;
						for (i = dmnd->queueIndex; !queue[i]; i++)
							;
						if (!(queue[IQMAX - 1] = dmndX) && i == IQMAX - 1) {
							i++;
						}
						pqMin = i;
					}
				}
			}
		}
		if (dmnd->next) {
			dmnd->next->prev = dmnd->prev;
		}
		dmnd->flags &= ~ROAM_ALLQ;
	}

	// Update the diamond's priority
	dmnd->queueIndex = newIndex;

	// Insert into the new queue if needed
	if (queueFlags & ROAM_ALLQ) {
		queue = ((queueFlags & ROAM_SPLITQ) ? splitQueue : mergeQueue);
		dmnd->prev = NULL;
		dmnd->next = queue[dmnd->queueIndex];

		queue[dmnd->queueIndex] = dmnd;
		if (dmnd->next) {
			dmnd->next->prev = dmnd;
		} else {
			if (queueFlags & ROAM_SPLITQ) {
				if (dmnd->queueIndex > pqMax) {
					pqMax = dmnd->queueIndex;
				}
			} else {
				if (dmnd->queueIndex < pqMin) {
					pqMin = dmnd->queueIndex;
				}
			}
		}

		dmnd->flags |= queueFlags;
	}

	// Apply the required locking/unlocking
	if (lockDelta != 0) {
		if (lockDelta < 0) {
			Unlock(dmnd);
		} else {
			Lock(dmnd);
		}
	}
}

//
// Split a diamond (finer detail)
//
void RoamSplitMerge::Split(SMDiamond *dmnd)
{
	// Skip if already split
	if (dmnd->flags & ROAM_SPLIT) {
		return;
	}

	// Split parents recursively as needed
	for (int i = 0; i < 2; i++) {
		SMDiamond *parent = dmnd->parent[i];
		Split(parent);

		// If this is the parent's first split child, take it off the merge queue
		if (!(parent->splitFlags & SPLIT_K)) {
			Enqueue(parent, ROAM_UNQ, parent->queueIndex);
		}
		parent->splitFlags |= SPLIT_K0 << dmnd->childIndex[i];
	}

	// Get the children, update flags, and put them on the split queue
	for (int i = 0; i < 4; i++) {
		SMDiamond *child = GetChild(dmnd, i);
		UpdateCull(child);
		UpdatePriority(child);

		Enqueue(child, ROAM_SPLITQ, child->queueIndex);
		int s = (child->parent[1] == dmnd ? 1 : 0);
		child->splitFlags |= SPLIT_P0 << s;
		Unlock(child);

		AllocateTri(child, s);
	}

	// The diamond is now split; move it to the merge queue
	dmnd->flags |= ROAM_SPLIT;
	Enqueue(dmnd, ROAM_MERGEQ, dmnd->queueIndex);

	// Put the parent triangles back on the free list
	FreeTri(dmnd, 0);
	FreeTri(dmnd, 1);
}

//
// Merge a diamond (coarser detail)
//
void RoamSplitMerge::Merge(SMDiamond *dmnd)
{
	// Skip if already merged
	if (!(dmnd->flags & ROAM_SPLIT)) {
		return;
	}

	// Take children off the split queue if their other parent is not split
	for (int i = 0; i < 4; i++) {
		SMDiamond *k = dmnd->child[i];
		int s = (k->parent[1] == dmnd ? 1 : 0);

		k->splitFlags &= ~(SPLIT_P0 << s);
		if (!(k->splitFlags & SPLIT_P)) {
			Enqueue(k, ROAM_UNQ, k->queueIndex);
		}

		FreeTri(k, s);
	}

	// The diamond is no longer split; move it to the split queue
	dmnd->flags &= ~ROAM_SPLIT;
	Enqueue(dmnd, ROAM_SPLITQ, dmnd->queueIndex);

	// Update the diamond's parents if needed
	for (int i = 0; i < 2; i++) {
		SMDiamond *parent = dmnd->parent[i];

		parent->splitFlags &= ~(SPLIT_K0 << dmnd->childIndex[i]);
		if (!(parent->splitFlags & SPLIT_K)) {
			UpdatePriority(parent);
			Enqueue(parent, ROAM_MERGEQ, parent->queueIndex);
		}
	}

	// Put the parent triangles back on the render list
	AllocateTri(dmnd, 0);
	AllocateTri(dmnd, 1);
}
