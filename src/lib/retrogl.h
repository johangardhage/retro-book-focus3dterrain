//
// Retro graphics library
//
// Author: Johan Gardhage <johan.gardhage@gmail.com>
//
#ifndef _RETROGL_H_
#define _RETROGL_H_

#include <SDL3/SDL.h>
#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glu.h>
#endif

// Global OpenGL context
inline SDL_GLContext& RETROGL_Context(void)
{
	static SDL_GLContext context = NULL;
	return context;
}

inline void RETROGL_SetAttributes(void)
{
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
}

inline bool RETROGL_Initialize(SDL_Window *window)
{
	RETROGL_Context() = SDL_GL_CreateContext(window);
	if (RETROGL_Context() == NULL) {
		return false;
	}
	SDL_GL_SetSwapInterval(1);

	// Setup OpenGL render state
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);	// Black background
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glShadeModel(GL_SMOOTH);
	glClearDepth(1.0);
	glDepthFunc(GL_LEQUAL);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

	return true;
}

inline void RETROGL_Deinitialize(void)
{
	if (RETROGL_Context()) {
		SDL_GL_DestroyContext(RETROGL_Context());
		RETROGL_Context() = NULL;
	}
}

inline void RETROGL_BeginFrame(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

inline void RETROGL_EndFrame(SDL_Window *window)
{
	SDL_GL_SwapWindow(window);
}

inline void RETROGL_UpdateProjection(int width, int height, double fov, double znear, double zfar)
{
	if (width <= 0) {
		width = 1;
	}
	if (height <= 0) {
		height = 1;
	}

	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(fov, (double)width / (double)height, znear, zfar);
	glMatrixMode(GL_MODELVIEW);
}

inline void RETROGL_UpdateWindowProjection(SDL_Window *window, int *width, int *height,
		double fov, double znear, double zfar)
{
	SDL_GetWindowSizeInPixels(window, width, height);
	RETROGL_UpdateProjection(*width, *height, fov, znear, zfar);
}

#endif
