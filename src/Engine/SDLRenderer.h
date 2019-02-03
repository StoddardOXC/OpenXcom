/*
 * An implementation of a renderer using the SDL2 renderer infrastructure
 */
#ifndef OPENXCOM_SDLRENDERER_H
#define OPENXCOM_SDLRENDERER_H

#include <SDL.h>
#include "Renderer.h"
#include <string>

namespace OpenXcom
{

class SDLRenderer :
	public Renderer
{
private:
	static const RendererType _rendererType = RENDERER_SDL2;
	SDL_Window *_window;
	SDL_Renderer *_renderer;
	SDL_Texture *_texture;
	SDL_Rect _srcRect, _dstRect;
	Uint32 _format;
	int _format_bpp; // to be deprecated in some future SDL.
	SDL_Surface *_surface;
	std::string _scaleHint;
	std::string _screenshotFilename; // empty = no screenshot this frame.
	void doScreenshot(void);
public:
	SDLRenderer(SDL_Window *window, int driver, Uint32 flags);
	~SDLRenderer(void);
	/// Where in the window to blit the surface on flip.
	void setInternalRect(SDL_Rect *srcRect) override;
	/// What size we'll be getting the surfaces of.
	void setOutputRect(SDL_Rect *dstRect) override;
	/// Blits the contents of the SDL_Surface to the screen.
	void flip(SDL_Surface *srcSurface) override;
	/// INTERNAL: List available renderer drivers
	void listSDLRendererDrivers();
	void screenshot(const std::string &filename) override;
	RendererType getRendererType() const override { return _rendererType; };
};

}
#endif