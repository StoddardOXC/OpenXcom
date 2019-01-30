/*
 * An OpenGL renderer, based on OpenXcom's OpenGL.cpp 
 * (which, in turn, is based on bsnes's ruby.cpp?)
 */

#ifndef OPENXCOM_OPENGLRENDERER_H
#define OPENXCOM_OPENGLRENDERER_H

#ifndef __NO_OPENGL

#include <SDL_opengl.h>
#include <string>
#include "Renderer.h"

namespace OpenXcom 
{

class Surface;

#ifndef __APPLE__  // FIXME: this is already defined in SDL.
extern PFNGLCREATEPROGRAMPROC glCreateProgram;
extern PFNGLUSEPROGRAMPROC glUseProgram;
extern PFNGLCREATESHADERPROC glCreateShader;
extern PFNGLDELETESHADERPROC glDeleteShader;
extern PFNGLSHADERSOURCEPROC glShaderSource;
extern PFNGLCOMPILESHADERPROC glCompileShader;
extern PFNGLATTACHSHADERPROC glAttachShader;
extern PFNGLDETACHSHADERPROC glDetachShader;
extern PFNGLLINKPROGRAMPROC glLinkProgram;
extern PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
extern PFNGLUNIFORM1IPROC glUniform1i;
extern PFNGLUNIFORM2FVPROC glUniform2fv;
extern PFNGLUNIFORM4FVPROC glUniform4fv;
#endif

std::string strGLError(GLenum glErr);

#define glErrorCheck() {\
	static bool reported = false;\
	GLenum glErr;\
	if (OpenGLRenderer::checkErrors && !reported && (glErr = glGetError()) != GL_NO_ERROR)\
	{\
		reported = true;\
		\
		do \
		{ \
			Log(LOG_WARNING) << __FILE__ << ":" << __LINE__ << ": glGetError() complaint: " << strGLError(glErr);\
		} while (((glErr = glGetError()) != GL_NO_ERROR));\
	}\
}



class OpenGLRenderer :
	public Renderer
{
private:
	static const RendererType _rendererType = RENDERER_OPENGL;

	SDL_Window *_window;
	SDL_Rect _srcRect, _dstRect;

	bool _resizeRequested;
	
	GLuint gltexture;
	GLuint glprogram;
	GLuint fragmentshader;
	bool linear;
	GLuint vertexshader;
	bool shader_support;

	SDL_GLContext glContext;

	SDL_Surface *bufSurface;
	uint32_t *buffer;
	//Surface *buffer_surface;
	unsigned iwidth, iheight, iformat, ibpp;
	

	void resize(unsigned width, unsigned height);
	bool lock(uint32_t *&data, unsigned &pitch);
	void clear();
	void set_shader(const char *source);
	void set_fragment_shader(const char *source);
	void set_vertex_shader(const char *source);
	void refresh(bool smooth, unsigned inwidth, unsigned inheight, unsigned outwidth, unsigned outheight);
	void init(int width, int height);
	void term();
public:
	static bool checkErrors;

	OpenGLRenderer(SDL_Window *window);
	~OpenGLRenderer(void);
	/// Sets the expected surfaces' pixel format
	void setPixelFormat(Uint32 format);
	/// Sets the size of the expected SDL_Surface.
	void setInternalRect(SDL_Rect *srcRect);
	/// Sets the desired output rectangle.
	void setOutputRect(SDL_Rect *dstRect);
	/// Blits the contents of the SDL_Surface to the screen.
	void flip(SDL_Surface *srcSurface);
	void setShader(std::string shaderPath);
	RendererType getRendererType() { return _rendererType; };
	void setVSync(bool sync);
	void screenshot(const std::string &filename) const;
};
}
#endif //__NO_OPENGL

#endif //OPENXCOM_OPENGLRENDERER_H
