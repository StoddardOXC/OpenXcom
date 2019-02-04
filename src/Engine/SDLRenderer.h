/*
 * An implementation of a renderer using the SDL2 renderer infrastructure
 */
#ifndef OPENXCOM_SDLRENDERER_H
#define OPENXCOM_SDLRENDERER_H

#include <SDL.h>
#include "Renderer.h"
#include <string>
#include <vector>

namespace OpenXcom
{


struct RenderItem {
	SDL_Surface *_surface;
	SDL_Texture *_texture;
	SDL_Rect _srcRect;
	SDL_Rect _dstRect;
	bool _visible;

	RenderItem();
	~RenderItem();
	void setInternalRect(SDL_Rect *srcRect, SDL_Renderer *renderer, int bpp, Uint32 format, bool blend);
	void setOutputRect(SDL_Rect *dstRect);
	void updateTexture(SDL_Surface *surface);
	bool visible() const { return _visible; };
	SDL_Texture *texture() { return _texture; }
	SDL_Rect *dstRect() { return &_dstRect; }
};

class SDLRenderer :
	public Renderer
{
private:
	static const RendererType _rendererType = RENDERER_SDL2;
	SDL_Window *_window;
	SDL_Renderer *_renderer;
	std::string _scaleHint;
	Uint8 _r, _g, _b, _a;
	Uint32 _format;
	int _format_bpp; // to be deprecated in some future SDL.

	std::vector<RenderItem> _renderList;

	std::string _screenshotFilename; // empty = no screenshot this frame.
	void doScreenshot(void);
public:
	SDLRenderer(SDL_Window *window, int driver, int filter);
	~SDLRenderer(void);
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
	RendererType getRendererType() const override { return _rendererType; };
	void getOutputSize(int& w, int& h) const override;
	void setClearColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a) override;
	static const std::vector<RendererDriver> listDrivers();
	static const std::vector<RendererFilter> listFilters();
};

}
#endif
