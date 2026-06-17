//
// Focus on 3D Terrain Programming
//
// A simple water system.
//
// Original coders: Trent Polack (trent@voxelsoft.com)
//
#include "retrogl.h"
#include "image.h"
#include "water.h"

//
// Render the water mesh (an animated, blended, textured quad)
//
void Water::Render(float worldSize)
{
	static float u = 0.0f;
	static float v = 0.0f;

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	glColor4f(color[0], color[1], color[2], transparency);

	glBindTexture(GL_TEXTURE_2D, texID);
	glEnable(GL_TEXTURE_2D);
	glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(u, v);           glVertex3f(0.0f, 0.0f, 0.0f);
		glTexCoord2f(u + 16, v);      glVertex3f(worldSize, 0.0f, 0.0f);
		glTexCoord2f(u, v + 16);      glVertex3f(0.0f, 0.0f, worldSize);
		glTexCoord2f(u + 16, v + 16); glVertex3f(worldSize, 0.0f, worldSize);
	glEnd();

	// "Animate" the water
	v += 0.01f;

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
}

//
// Load the water's texture map
//
void Water::LoadTextureMaps(const char *filename)
{
	Image image;
	image.Load(filename, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, true);
	texID = image.GetID();
}
