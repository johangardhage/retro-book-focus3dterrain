//
// Focus on 3D Terrain Programming
//
// A brute force terrain implementation.
//
// Original coders: Trent Polack (trent@voxelsoft.com)
//
#ifndef _BRUTEFORCE_H_
#define _BRUTEFORCE_H_

#include "terrain.h"

class BruteForce : public Terrain
{
public:
	void Render(void);

	BruteForce(void) {}
	~BruteForce(void) {}
};

#endif
