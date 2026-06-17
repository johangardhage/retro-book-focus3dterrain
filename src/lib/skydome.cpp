//
// Focus on 3D Terrain Programming
//
// A skydome (half-sphere) system.
//
// Original coders: Trent Polack (trent@voxelsoft.com)
//
#include <math.h> // sin, cos, atan2, asin, sqrt, pow
#include <stdlib.h> // (none, kept for parity)
#include <string.h> // memset
#include "retrogl.h"
#include "image.h"
#include "skydome.h"

#define PI 3.1415926535897932f
#define SQR(n) ((n) * (n))
#define CLAMP(value, lo, hi) do { if ((value) < (lo)) (value) = (lo); else if ((value) > (hi)) (value) = (hi); } while (0)

//
// Procedurally generate the skydome (theta/phi in degrees, given radius)
//
void Skydome::Init(float theta, float phi, float radius)
{
	const float DTOR = PI / 180.0f;
	int n = 0;

	// Number of vertices in the dome's triangle strip
	numVertices = (int)((360 / theta) * (90 / phi) * 4);

	vertices = new float[numVertices * 3];
	texCoords = new float[numVertices * 2];

	memset(vertices, 0, sizeof(float) * numVertices * 3);
	memset(texCoords, 0, sizeof(float) * numVertices * 2);

	for (int iPhi = 0; iPhi <= (90 - phi); iPhi += (int)phi) {
		for (int iTheta = 0; iTheta <= (360 - theta); iTheta += (int)theta) {
			// Helper to emit a vertex at (p, t) degrees
			// (inlined below four times to match the original)
			float vx, vy, vz, len;

			// phi, theta
			vx = radius * sinf(iPhi * DTOR) * cosf(iTheta * DTOR);
			vy = radius * sinf(iPhi * DTOR) * sinf(iTheta * DTOR);
			vz = radius * cosf(iPhi * DTOR);
			vertices[(n * 3) + 0] = vx;
			vertices[(n * 3) + 1] = vy;
			vertices[(n * 3) + 2] = vz;
			len = sqrtf(vx * vx + vy * vy + vz * vz);
			texCoords[(n * 2) + 0] = (float)(atan2(vx / len, vz / len) / (PI * 2)) + 0.5f;
			texCoords[(n * 2) + 1] = (float)(asinf(vy / len) / PI) + 0.5f;
			n++;

			// phi+phi, theta
			vx = radius * sinf((iPhi + phi) * DTOR) * cosf(iTheta * DTOR);
			vy = radius * sinf((iPhi + phi) * DTOR) * sinf(iTheta * DTOR);
			vz = radius * cosf((iPhi + phi) * DTOR);
			vertices[(n * 3) + 0] = vx;
			vertices[(n * 3) + 1] = vy;
			vertices[(n * 3) + 2] = vz;
			len = sqrtf(vx * vx + vy * vy + vz * vz);
			texCoords[(n * 2) + 0] = (float)(atan2(vx / len, vz / len) / (PI * 2)) + 0.5f;
			texCoords[(n * 2) + 1] = (float)(asinf(vy / len) / PI) + 0.5f;
			n++;

			// phi, theta+theta
			vx = radius * sinf(DTOR * iPhi) * cosf(DTOR * (iTheta + theta));
			vy = radius * sinf(DTOR * iPhi) * sinf(DTOR * (iTheta + theta));
			vz = radius * cosf(DTOR * iPhi);
			vertices[(n * 3) + 0] = vx;
			vertices[(n * 3) + 1] = vy;
			vertices[(n * 3) + 2] = vz;
			len = sqrtf(vx * vx + vy * vy + vz * vz);
			texCoords[(n * 2) + 0] = (float)(atan2(vx / len, vz / len) / (PI * 2)) + 0.5f;
			texCoords[(n * 2) + 1] = (float)(asinf(vy / len) / PI) + 0.5f;
			n++;

			if (iPhi > -90 && iPhi < 90) {
				// phi+phi, theta+theta
				vx = radius * sinf((iPhi + phi) * DTOR) * cosf(DTOR * (iTheta + theta));
				vy = radius * sinf((iPhi + phi) * DTOR) * sinf(DTOR * (iTheta + theta));
				vz = radius * cosf((iPhi + phi) * DTOR);
				vertices[(n * 3) + 0] = vx;
				vertices[(n * 3) + 1] = vy;
				vertices[(n * 3) + 2] = vz;
				len = sqrtf(vx * vx + vy * vy + vz * vz);
				texCoords[(n * 2) + 0] = (float)(atan2(vx / len, vz / len) / (PI * 2)) + 0.5f;
				texCoords[(n * 2) + 1] = (float)(asinf(vy / len) / PI) + 0.5f;
				n++;
			}
		}
	}

	// Fix the texture-seam problem
	for (int i = 0; i < numVertices - 3; i++) {
		int i0 = (i * 2);
		int i1 = ((i + 1) * 2);
		int i2 = ((i + 2) * 2);

		if ((texCoords[i0 + 0] - texCoords[i1 + 0]) > 0.9f) texCoords[i1 + 0] += 1.0f;
		if ((texCoords[i1 + 0] - texCoords[i0 + 0]) > 0.9f) texCoords[i0 + 0] += 1.0f;
		if ((texCoords[i0 + 0] - texCoords[i2 + 0]) > 0.9f) texCoords[i2 + 0] += 1.0f;
		if ((texCoords[i2 + 0] - texCoords[i0 + 0]) > 0.9f) texCoords[i0 + 0] += 1.0f;
		if ((texCoords[i1 + 0] - texCoords[i2 + 0]) > 0.9f) texCoords[i2 + 0] += 1.0f;
		if ((texCoords[i2 + 0] - texCoords[i1 + 0]) > 0.9f) texCoords[i1 + 0] += 1.0f;

		if ((texCoords[i0 + 1] - texCoords[i1 + 1]) > 0.8f) texCoords[i1 + 1] += 1.0f;
		if ((texCoords[i1 + 1] - texCoords[i0 + 1]) > 0.8f) texCoords[i0 + 1] += 1.0f;
		if ((texCoords[i0 + 1] - texCoords[i2 + 1]) > 0.8f) texCoords[i2 + 1] += 1.0f;
		if ((texCoords[i2 + 1] - texCoords[i0 + 1]) > 0.8f) texCoords[i0 + 1] += 1.0f;
		if ((texCoords[i1 + 1] - texCoords[i2 + 1]) > 0.8f) texCoords[i2 + 1] += 1.0f;
		if ((texCoords[i2 + 1] - texCoords[i1 + 1]) > 0.8f) texCoords[i1 + 1] += 1.0f;
	}
}

void Skydome::Shutdown(void)
{
	delete[] texCoords;
	delete[] vertices;
	texCoords = NULL;
	vertices = NULL;
}

void Skydome::LoadTexture(const char *filename)
{
	Image texture;
	texture.Load(filename, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, true);
	texID = texture.GetID();
}

//
// Render the skydome (delta drives the cloud rotation when rotate is true)
//
void Skydome::Render(float delta, bool rotate)
{
	static float rot = 0.0f;

	glBindTexture(GL_TEXTURE_2D, texID);
	glEnable(GL_TEXTURE_2D);

	// The dome is viewed from the inside, so don't cull its back faces
	glDisable(GL_CULL_FACE);

	glPushMatrix();
	glTranslatef(center[0], center[1], center[2]);

	if (rotate) {
		rot += delta;
		glRotatef(rot, 0.0f, 1.0f, 0.0f);
	}

	// Orient the dome correctly
	glRotatef(270, 1.0f, 0.0f, 0.0f);

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, vertices);

	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, 0, texCoords);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, numVertices);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glPopMatrix();

	glEnable(GL_CULL_FACE);
	glDisable(GL_TEXTURE_2D);
}

//
// Generate a cloud texture procedurally using fractal Brownian motion
//
void Skydome::GenCloudTexture(int size, float blur, float octaves, float amplitude, float frequency, float h)
{
	// Fractally generate the data
	float *data = new float[SQR(size)];
	for (int y = 0; y < size; y++) {
		for (int x = 0; x < size; x++) {
			data[(y * size) + x] = FBM((float)x, (float)y, octaves, amplitude, frequency, h);
		}
	}

	// Normalize to 0-255, then drop the lower half (eliminate extra noise)
	NormalizeFractal(data, size);
	for (int y = 0; y < size - 1; y++) {
		for (int x = 0; x < size - 1; x++) {
			if (data[(y * size) + x] < 128) {
				data[(y * size) + x] = 0;
			}
		}
	}

	// Blur the data
	Blur(data, size, blur);

	// Build the RGB texture (bluish base + white clouds)
	float *texData = new float[SQR(size) * 3];
	for (int y = 0; y < size - 1; y++) {
		for (int x = 0; x < size - 1; x++) {
			int index = (y * size) + x;

			texData[(index * 3) + 0] = 0.25f + (data[index] / 255);
			texData[(index * 3) + 1] = 0.25f + (data[index] / 255);
			texData[(index * 3) + 2] = 1.0f + (data[index] / 255);

			CLAMP(texData[(index * 3) + 0], 0.0f, 1.0f);
			CLAMP(texData[(index * 3) + 1], 0.0f, 1.0f);
			CLAMP(texData[(index * 3) + 2], 0.0f, 1.0f);
		}
	}

	// Build the OpenGL texture
	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D, texID);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, size, size, GL_RGB, GL_FLOAT, texData);

	delete[] texData;
	delete[] data;
}

//
// Interpolate two values with a cosine bias
//
float Skydome::CosineInterpolation(float num1, float num2, float x)
{
	float xPI = x * PI;
	float temp = (float)(1 - cos(xPI)) * 0.5f;
	return num1 * (1 - temp) + (num2 * temp);
}

//
// Pseudo-random value for an integer lattice point
//
float Skydome::RangedRandom(int x, int y)
{
	int n = x + y * 57;
	n = (n << 13) ^ n;
	return (1 - ((n * (n * n * 15731 + 789221) + 1376312589) & 2147483647) / 1073741824.0f);
}

//
// Smoothed pseudo-random value (corners/sides/center average)
//
float Skydome::RangedSmoothRandom(int x, int y)
{
	float center = RangedRandom(x, y) / 4;
	float corners = (RangedRandom(x - 1, y - 1) + RangedRandom(x + 1, y - 1) +
		RangedRandom(x - 1, y + 1) + RangedRandom(x + 1, y + 1)) / 16;
	float sides = (RangedRandom(x - 1, y) + RangedRandom(x + 1, y) +
		RangedRandom(x, y - 1) + RangedRandom(x, y + 1)) / 8;
	return corners + sides + center;
}

//
// Interpolated noise value
//
float Skydome::Noise(float x, float y)
{
	int iX = (int)x;
	int iY = (int)y;
	float fracX = x - iX;
	float fracY = y - iY;

	float f1 = RangedSmoothRandom(iX, iY);
	float f2 = RangedSmoothRandom(iX + 1, iY);
	float f3 = RangedSmoothRandom(iX, iY + 1);
	float f4 = RangedSmoothRandom(iX + 1, iY + 1);
	float i1 = CosineInterpolation(f1, f2, fracX);
	float i2 = CosineInterpolation(f3, f4, fracX);
	return CosineInterpolation(i1, i2, fracY);
}

//
// Fractal Brownian motion (sum of noise octaves)
//
float Skydome::FBM(float x, float y, float octaves, float amplitude, float frequency, float h)
{
	float value = 0;
	for (int i = 0; i < (octaves - 1); i++) {
		value += (Noise(x * frequency, y * frequency) * amplitude);
		amplitude *= h;
	}
	return value;
}

//
// Scale a fractal buffer to the range 0-255
//
void Skydome::NormalizeFractal(float *data, int size)
{
	float min = data[0];
	float max = data[0];

	for (int i = 1; i < SQR(size); i++) {
		if (data[i] > max) {
			max = data[i];
		} else if (data[i] < min) {
			min = data[i];
		}
	}

	if (max <= min) {
		return;
	}

	float height = max - min;
	for (int i = 0; i < SQR(size); i++) {
		data[i] = ((data[i] - min) / height) * 255.0f;
	}
}

//
// Blur one band of values
//
void Skydome::BlurBand(float *band, int stride, int count, float filter)
{
	float v = band[0];
	int j = stride;
	for (int i = 0; i < count - 1; i++) {
		band[j] = filter * v + (1 - filter) * band[j];
		v = band[j];
		j += stride;
	}
}

//
// Blur a buffer (in all four directions)
//
void Skydome::Blur(float *data, int size, float filter)
{
	for (int i = 0; i < size; i++) {
		BlurBand(&data[size * i], 1, size, filter);
	}
	for (int i = 0; i < size; i++) {
		BlurBand(&data[size * i + size - 1], -1, size, filter);
	}
	for (int i = 0; i < size; i++) {
		BlurBand(&data[i], size, size, filter);
	}
	for (int i = 0; i < size; i++) {
		BlurBand(&data[size * (size - 1) + i], -size, size, filter);
	}
}
