//
// Focus on 3D Terrain Programming
//
// A brute force terrain implementation: render the entire height field as
// triangle strips. Supports height-based grayscale coloring (when no texture
// is active), a stretched color texture, and an optional detail-map pass.
//
// Original coders: Trent Polack (trent@voxelsoft.com)
//
#include "retrogl.h"
#include "bruteforce.h"

//
// Render the height field as triangle strips. If grayscale is true, vertices
// are colored by height and no texture coordinates are emitted; otherwise the
// vertices are white with texture coordinates scaled by texRepeat.
//
void BruteForce::RenderStrips(bool grayscale, float texRepeat)
{
	for (int z = 0; z < size - 1; z++) {
		glBegin(GL_TRIANGLE_STRIP);

		for (int x = 0; x < size - 1; x++) {
			float texLeft = (float)x / size;
			float texBottom = (float)z / size;
			float texTop = (float)(z + 1) / size;

			if (grayscale) {
				unsigned char color = GetTrueHeightAtPoint(x, z);
				glColor3ub(color, color, color);
			} else {
				unsigned char shade = GetBrightnessAtPoint(x, z);
				glColor3ub(shade * lightColor[0], shade * lightColor[1], shade * lightColor[2]);
				EmitTexCoord(texLeft * texRepeat, texBottom * texRepeat);
			}
			glVertex3f((float)x, GetScaledHeightAtPoint(x, z), (float)z);

			if (grayscale) {
				unsigned char color = GetTrueHeightAtPoint(x, z + 1);
				glColor3ub(color, color, color);
			} else {
				unsigned char shade = GetBrightnessAtPoint(x, z + 1);
				glColor3ub(shade * lightColor[0], shade * lightColor[1], shade * lightColor[2]);
				EmitTexCoord(texLeft * texRepeat, texTop * texRepeat);
			}
			glVertex3f((float)x, GetScaledHeightAtPoint(x, z + 1), (float)z + 1);

			// Increase the vertex count by two
			vertsPerFrame += 2;

			// No triangles are rendered on the first X-loop, they just start
			// the triangle strip off
			if (x != 0) {
				trisPerFrame += 2;
			}
		}

		glEnd();
	}
}

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

	bool doTexture = textureMapping && texture.IsLoaded();
	bool doDetail = detailMapping && detailMap.IsLoaded();

	// No texturing: fall back to height-based grayscale coloring
	if (!doTexture && !doDetail) {
		glDisable(GL_TEXTURE_2D);
		RenderStrips(true, 1.0f);
		return;
	}

	// Single-pass ARB multitexturing: color map * detail map at once
	if (multitexture && doTexture && doDetail) {
		BeginMultitexture();
		RenderStrips(false, 1.0f);
		EndMultitexture();
		return;
	}

	// Color texture pass
	if (doTexture) {
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, texture.GetID());
		RenderStrips(false, 1.0f);
	}

	// Detail map pass (multiplies the detail map onto the color pass)
	if (doDetail) {
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, detailMap.GetID());

		// Only blend if a color pass was made underneath
		if (doTexture) {
			glEnable(GL_BLEND);
			glBlendFunc(GL_ZERO, GL_SRC_COLOR);
		}

		RenderStrips(false, (float)repeatDetailMap);

		glDisable(GL_BLEND);
	}
}
