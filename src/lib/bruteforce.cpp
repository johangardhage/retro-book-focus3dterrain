//
// Focus on 3D Terrain Programming
//
// A brute force terrain implementation: render the entire height field as
// triangle strips, with height-based grayscale coloring.
//
// Original coders: Trent Polack (trent@voxelsoft.com)
//
#include "retrogl.h"
#include "bruteforce.h"

//
// Render the terrain height field
//
void BruteForce::Render(void)
{
	// Reset the counting variables
	vertsPerFrame = 0;
	trisPerFrame = 0;

	// Cull non camera-facing polygons
	glEnable(GL_CULL_FACE);

	// Loop through the Z-axis of the terrain
	for (int z = 0; z < size - 1; z++) {
		// Begin a new triangle strip
		glBegin(GL_TRIANGLE_STRIP);

		// Loop through the X-axis of the terrain.
		// This is where the triangle strip is constructed.
		for (int x = 0; x < size - 1; x++) {
			// Use height-based coloring (high points are light, low points dark)
			unsigned char color = GetTrueHeightAtPoint(x, z);
			glColor3ub(color, color, color);
			glVertex3f((float)x, GetScaledHeightAtPoint(x, z), (float)z);

			color = GetTrueHeightAtPoint(x, z + 1);
			glColor3ub(color, color, color);
			glVertex3f((float)x, GetScaledHeightAtPoint(x, z + 1), (float)z + 1);

			// Increase the vertex count by two
			vertsPerFrame += 2;

			// No triangles are rendered on the first X-loop, they just start
			// the triangle strip off
			if (x != 0) {
				trisPerFrame += 2;
			}
		}

		// End the triangle strip
		glEnd();
	}
}
