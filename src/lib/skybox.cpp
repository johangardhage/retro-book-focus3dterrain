//
// Focus on 3D Terrain Programming
//
// A skybox class.
//
// Original coders: Trent Polack (trent@voxelsoft.com)
//
#include "retrogl.h"
#include "skybox.h"

//
// Load one side's texture
//
bool Skybox::LoadTexture(int side, const char *filename)
{
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	return textures[side].Load(filename, GL_LINEAR, GL_LINEAR, false);
}

//
// Render the skybox
//
void Skybox::Render(void)
{
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glEnable(GL_TEXTURE_2D);

	// The skybox is viewed from the inside, so don't cull its back faces
	glDisable(GL_CULL_FACE);

	glPushMatrix();
	glTranslatef(center[0], center[1], center[2]);

	// Front face
	glBindTexture(GL_TEXTURE_2D, textures[SBX_FRONT].GetID());
	glBegin(GL_TRIANGLE_FAN);
		glTexCoord2f(1.0f, 1.0f); glVertex3f(vmax[0], vmax[1], vmax[2]);
		glTexCoord2f(1.0f, 0.0f); glVertex3f(vmax[0], vmin[1], vmax[2]);
		glTexCoord2f(0.0f, 0.0f); glVertex3f(vmin[0], vmin[1], vmax[2]);
		glTexCoord2f(0.0f, 1.0f); glVertex3f(vmin[0], vmax[1], vmax[2]);
	glEnd();

	// Back face
	glBindTexture(GL_TEXTURE_2D, textures[SBX_BACK].GetID());
	glBegin(GL_TRIANGLE_FAN);
		glTexCoord2f(1.0f, 1.0f); glVertex3f(vmin[0], vmax[1], vmin[2]);
		glTexCoord2f(1.0f, 0.0f); glVertex3f(vmin[0], vmin[1], vmin[2]);
		glTexCoord2f(0.0f, 0.0f); glVertex3f(vmax[0], vmin[1], vmin[2]);
		glTexCoord2f(0.0f, 1.0f); glVertex3f(vmax[0], vmax[1], vmin[2]);
	glEnd();

	// Right face
	glBindTexture(GL_TEXTURE_2D, textures[SBX_RIGHT].GetID());
	glBegin(GL_TRIANGLE_FAN);
		glTexCoord2f(1.0f, 1.0f); glVertex3f(vmax[0], vmax[1], vmin[2]);
		glTexCoord2f(1.0f, 0.0f); glVertex3f(vmax[0], vmin[1], vmin[2]);
		glTexCoord2f(0.0f, 0.0f); glVertex3f(vmax[0], vmin[1], vmax[2]);
		glTexCoord2f(0.0f, 1.0f); glVertex3f(vmax[0], vmax[1], vmax[2]);
	glEnd();

	// Left face
	glBindTexture(GL_TEXTURE_2D, textures[SBX_LEFT].GetID());
	glBegin(GL_TRIANGLE_FAN);
		glTexCoord2f(1.0f, 1.0f); glVertex3f(vmin[0], vmax[1], vmax[2]);
		glTexCoord2f(1.0f, 0.0f); glVertex3f(vmin[0], vmin[1], vmax[2]);
		glTexCoord2f(0.0f, 0.0f); glVertex3f(vmin[0], vmin[1], vmin[2]);
		glTexCoord2f(0.0f, 1.0f); glVertex3f(vmin[0], vmax[1], vmin[2]);
	glEnd();

	// Top face
	glBindTexture(GL_TEXTURE_2D, textures[SBX_TOP].GetID());
	glBegin(GL_TRIANGLE_FAN);
		glTexCoord2f(0.0f, 0.0f); glVertex3f(vmin[0], vmax[1], vmax[2]);
		glTexCoord2f(0.0f, 1.0f); glVertex3f(vmin[0], vmax[1], vmin[2]);
		glTexCoord2f(1.0f, 1.0f); glVertex3f(vmax[0], vmax[1], vmin[2]);
		glTexCoord2f(1.0f, 0.0f); glVertex3f(vmax[0], vmax[1], vmax[2]);
	glEnd();

	// Bottom face
	glBindTexture(GL_TEXTURE_2D, textures[SBX_BOTTOM].GetID());
	glBegin(GL_TRIANGLE_FAN);
		glTexCoord2f(0.0f, 0.0f); glVertex3f(vmin[0], vmin[1], vmin[2]);
		glTexCoord2f(0.0f, 1.0f); glVertex3f(vmin[0], vmin[1], vmax[2]);
		glTexCoord2f(1.0f, 1.0f); glVertex3f(vmax[0], vmin[1], vmax[2]);
		glTexCoord2f(1.0f, 0.0f); glVertex3f(vmax[0], vmin[1], vmin[2]);
	glEnd();

	glPopMatrix();

	glEnable(GL_CULL_FACE);
	glDisable(GL_TEXTURE_2D);
}
