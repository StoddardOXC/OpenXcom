/*
 * An implementation of a renderer using the SDL2 renderer infrastructure
 */
#ifndef OPENXCOM_GL2RENDERER_H
#define OPENXCOM_GL2RENDERER_H

#include <SDL.h>
#include <SDL_opengl.h>
#include "Renderer.h"
#include <string>
#include <vector>

namespace OpenXcom
{


struct RenderItemGL2 {
	SDL_Surface *_surface;
	GLuint _textureName;
	SDL_Rect _srcRect;
	SDL_Rect _dstRect;
	bool _visible;
	Uint32 _format;
	bool _blend;

	RenderItemGL2();
	~RenderItemGL2();
	void setInternalRect(SDL_Rect *srcRect, int bpp, Uint32 format, bool blend);
	void setOutputRect(SDL_Rect *dstRect);
	void updateTexture(SDL_Surface *surface);
	void recreateTexture();
	bool visible() const { return _visible; };
	GLuint texture() { return _textureName; }
	const SDL_Rect *dstRect() { return &_dstRect; }
};

class GL2Renderer : public Renderer
{
private:
	SDL_Window *_window;
	SDL_GLContext _glContext;
	Uint8 _r, _g, _b, _a;
	Uint32 _format;
	int _format_bpp; // to be deprecated in some future SDL.

	std::vector<RenderItemGL2> _renderList;

	std::string _screenshotFilename; // empty = no screenshot this frame.
	void doScreenshot(void);

public:
	static std::string ucWords(std::string str);
	static const std::string GL_EXT, GL_FOLDER, GL_STRING;

	GL2Renderer(SDL_Window *window);
	~GL2Renderer(void);
	/// ala glGetTexture()
	unsigned getTexture() override;
	/// Where in the window to blit the surface on flip.
	void setInternalRect(unsigned item, SDL_Rect *srcRect, bool blend) override;
	/// What size we'll be getting the surfaces of.
	void setOutputRect(unsigned item, SDL_Rect *dstRect) override;
	/// Sets the surface data
	void updateTexture(unsigned item, SDL_Surface *surface) override;
	/// Blits the contents of RenderItems to the screen.
	void flip() override;
	void screenshot(const std::string &filename) override;
	void getOutputSize(int& w, int& h) const override;
	void setClearColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a) override;
	void setFilter(const std::string& filter) override;
	static const std::vector<std::string> listDrivers();
	static const std::vector<std::string> listFilters();
};

}
#endif
