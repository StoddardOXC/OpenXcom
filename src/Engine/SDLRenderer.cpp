


#include <SDL.h>
#include <SDL_pnglite.h>
#include "SDLRenderer.h"
#include "Exception.h"
#include "Logger.h"


namespace OpenXcom
{

RenderItemSDL::RenderItemSDL() : _surface(0), _texture(0), _srcRect(), _dstRect(), _visible(false) { }  // not needed at all I suspect.

RenderItemSDL::~RenderItemSDL()
{
	if (_surface) { SDL_FreeSurface(_surface); }
	if (_texture) { SDL_DestroyTexture(_texture); }
}

void RenderItemSDL::setInternalRect(SDL_Rect *srcRect, SDL_Renderer *renderer, int bpp, Uint32 format, bool blend)
{
	if (srcRect == NULL) {
		_visible = false;
		return;
	}
	if (srcRect->w == _srcRect.w && srcRect->h ==_srcRect.h) {
		return;
	}

	if (_surface) { SDL_FreeSurface(_surface); }
	_surface = SDL_CreateRGBSurfaceWithFormat(0, srcRect->w, srcRect->h, bpp, format );
	SDL_SetSurfaceBlendMode(_surface, SDL_BLENDMODE_NONE);
	SDL_FillRect(_surface, NULL, 0);

	if (_surface == NULL) {
		throw Exception(SDL_GetError());
	}
	_format = format;
	_blend = blend;
	_srcRect.w = srcRect->w;
	_srcRect.h = srcRect->h;
	recreateTexture(renderer);

	Log(LOG_INFO) << "[SDLRenderer] setInternalRect(): Texture is now "
		<< _srcRect.w << "x" << _srcRect.h << " " << SDL_GetPixelFormatName( format );
}

void RenderItemSDL::recreateTexture(SDL_Renderer *renderer)
{
	if (!_surface) { return; }
	if (_texture) { SDL_DestroyTexture(_texture); }
	_texture = SDL_CreateTexture(renderer, _format, SDL_TEXTUREACCESS_STREAMING, _srcRect.w, _srcRect.h);
	if (_texture == NULL) {
		throw Exception(SDL_GetError());
	}
	SDL_SetTextureBlendMode(_texture, _blend ? SDL_BLENDMODE_BLEND : SDL_BLENDMODE_NONE);
}

void RenderItemSDL::setOutputRect(SDL_Rect *dstRect)
{
	if (dstRect == NULL) {
		_visible = false;
		return;
	}
	_dstRect.x = dstRect->x;
	_dstRect.y = dstRect->y;
	_dstRect.w = dstRect->w;
	_dstRect.h = dstRect->h;
	//Log(LOG_INFO) << "[SDLRenderer] setOutputRect(): Output "
	//	<< _dstRect.w << "x" << _dstRect.h << " at " << _dstRect.x << "x" << _dstRect.y;
}

void RenderItemSDL::updateTexture(SDL_Surface *surface)
{
	if (surface == NULL || _texture == NULL) {
		_visible = false;
		return;
	} else {
		_visible = true;
	}
#if 0
	Log(LOG_INFO) << "[SDLRenderer] src: "<<srcSurface->w<<"x"<<srcSurface->h<<" "
		<< SDL_GetPixelFormatName(srcSurface->format->format)
		<< " dst: "<<_surface->w<<"x"<<_surface->h<<" srcR: "<<_srcRect.w <<"x"<< _srcRect.h
		<< " dstR: "<<_dstRect.w <<"x"<<_dstRect.h;
#endif

	//SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_NONE);
	if (SDL_BlitSurface(surface, NULL, _surface, NULL)) {
		SDL_BlendMode srcBM, dstBM;
		SDL_GetSurfaceBlendMode(surface, &srcBM);
		SDL_GetSurfaceBlendMode(_surface, &dstBM);
		Log(LOG_ERROR)<< "[SDLRenderer] SDL_BlitSurface(): " << SDL_GetError();
		Log(LOG_ERROR) << " src pf: " << SDL_GetPixelFormatName(surface->format->format)
			<< " BM: " << srcBM
			<< " dst pf: " << SDL_GetPixelFormatName(_surface->format->format)
			<< " BM: " << (int) dstBM;

		throw Exception(SDL_GetError());
	}
	// _surface must be the same size as _texture at all times
	// hence the NULL for the rect
	if (SDL_UpdateTexture(_texture, NULL, _surface->pixels, _surface->pitch)) {
		Log(LOG_ERROR)<< "[SDLRenderer] SDL_UpdateTexture(): " << SDL_GetError();
		throw Exception(SDL_GetError());
	}
}

const std::vector<std::string> SDLRenderer::listFilters()
{
	std::vector<std::string> rv;
	rv.push_back("Nearest");
	rv.push_back("Linear");
	rv.push_back("Best");
	return rv;
}

static std::string map_filter_name(const std::string& filtername)
{
	if (filtername == "Best") {
		return std::string("best");
	} else if (filtername == "Linear") {
		return std::string("linear");
	} else {
		return std::string("nearest");
	}
}

const std::vector<std::string> SDLRenderer::listDrivers()
{
	std::vector<std::string> rv;
	int numRenderDrivers = SDL_GetNumRenderDrivers();
	Log(LOG_INFO) << "[SDLRenderer] Listing available rendering drivers:";
	Log(LOG_INFO) << "[SDLRenderer]  Number of drivers: " << numRenderDrivers;
	for (int i = 0; i < numRenderDrivers; ++i) {
		SDL_RendererInfo info;
		SDL_GetRenderDriverInfo(i, &info);
		std::string rdname = "SDL2 ";
		rdname += info.name;
		rv.push_back(rdname);
		Log(LOG_INFO) << "[SDLRenderer]  Driver " << i << ": " << rdname;
		Log(LOG_INFO) << "[SDLRenderer]    Number of texture formats: " << info.num_texture_formats;
		for (Uint32 j = 0; j < info.num_texture_formats; ++j) {
			std::string pfname = SDL_GetPixelFormatName(info.texture_formats[j]);
			Log(LOG_INFO) << "[SDLRenderer]     Texture format " << j << ": " << pfname;
		}
	}
	return rv;
}

static int map_driver_name(const std::string& driver)
{
	if (driver.size() < 4) { return -1; } // no SDL2  prefix
	std::string unprefixed(driver.c_str() + 4);
	int numRenderDrivers = SDL_GetNumRenderDrivers();
	for (int i = 0; i < numRenderDrivers; ++i) {
		SDL_RendererInfo info;
		SDL_GetRenderDriverInfo(i, &info);
		if (unprefixed == std::string(info.name)) {
			return i;
		}
	}
	return -1;
}

SDLRenderer::SDLRenderer(SDL_Window *window, const std::string& driver, const std::string& filter):
	_window(window), _renderer(NULL), _r(0), _g(0), _b(0), _a(255), _format(SDL_PIXELFORMAT_UNKNOWN),
	_screenshotFilename()
{
	std::string scaleHint = map_filter_name(filter);
	// TODO: what it returns, why, and why don't we use SDL_SetHintWithPriority() etc etc.
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, scaleHint.c_str());
	_renderer = SDL_CreateRenderer(window, map_driver_name(driver), 0);
	if (_renderer == NULL)
	{
		Log(LOG_FATAL) << "[SDLRenderer] Couldn't create renderer; error message: " << SDL_GetError();
		throw(Exception(SDL_GetError()));
	}
	SDL_RendererInfo info;
	SDL_GetRendererInfo(_renderer, &info);
	if (_format == SDL_PIXELFORMAT_UNKNOWN) { _format = info.texture_formats[0]; }
	Log(LOG_INFO) << "[SDLRenderer] Created new SDLRenderer, " << info.name <<
			" scaleHint: " << scaleHint << " " << SDL_GetPixelFormatName(_format);

	Uint32 r, g, b, a;
	if (!SDL_PixelFormatEnumToMasks(_format, &_format_bpp, &r, &g, &b, &a)) {
		Log(LOG_FATAL) << "[SDLRenderer] Couldn't get bpp for "
		<< SDL_GetPixelFormatName(_format) << ": " << SDL_GetError();
		throw(Exception(SDL_GetError()));
	}
}

SDLRenderer::~SDLRenderer(void)
{
	SDL_DestroyRenderer(_renderer);
}

unsigned SDLRenderer::getTexture()
{
	_renderList.resize(_renderList.size() + 1);
	return _renderList.size() - 1;
}

/**
 * Sets up internal structures: an SDL_Surface for pixel format conversion
 * and an SDL_Texture to update and render.
 * @param item - texture id
 * @param srcRect - size of the expected surfaces
 * @param blend - whether to blit with or without alpha
 */
void SDLRenderer::setInternalRect(unsigned item, SDL_Rect *srcRect, bool blend)
{
	_renderList.at(item).setInternalRect(srcRect, _renderer, _format_bpp, _format, blend);
}

/**
 * Sets the blit destination
 * @param dstRect - blit destination in window coordinates
 */
void SDLRenderer::setOutputRect(unsigned item, SDL_Rect *dstRect)
{
	_renderList.at(item).setOutputRect(dstRect);
}

/**
 * Updates pixel data
 * @param surface - what to blit. Size must match what was set up in setInternalRect().
 */
void SDLRenderer::updateTexture(unsigned item, SDL_Surface *surface)
{
	_renderList.at(item).updateTexture(surface);
}

/**
 * Performs the blits and presents the result.
 * Also does the screenshot if one was requested.
 */
void SDLRenderer::flip()
{
	SDL_SetRenderDrawColor(_renderer, _r, _g, _b, _a);
	if (SDL_RenderClear(_renderer)) {
		Log(LOG_ERROR)<< "[SDLRenderer] SDL_RenderClear(): " << SDL_GetError();
		throw Exception(SDL_GetError());
	}
	// rendercopy does the scaling
	for (auto& item: _renderList) {
		if (!item.visible()) {
			continue;
		}
		if (SDL_RenderCopy(_renderer, item.texture(), NULL, item.dstRect())) {
			Log(LOG_ERROR)<< "[SDLRenderer] SDL_RenderCopy(): " << SDL_GetError();
			throw Exception(SDL_GetError());
		}
	}
	SDL_RenderPresent(_renderer);
	doScreenshot();
}

/**
 * Sets a filename for the next screenshot.
 * Actual screenshotting is done during the next flip().
 *
 * We are interested in two different images
 *  - what was submitted to the flip()
 *  - what actually got to the window
 * The first one is rightfully done in Screen.
 * (or hackily in the flip() method)
 */
void SDLRenderer::screenshot(const std::string &filename) {
	_screenshotFilename = filename;
}

/**
 * Does the actual screenshotting
 */
void SDLRenderer::doScreenshot() {
	if (_screenshotFilename.empty()) { return; }

	SDL_Rect vprect;
	SDL_RenderGetViewport(_renderer, &vprect );
	SDL_Surface *shot = SDL_CreateRGBSurfaceWithFormat(0, vprect.w, vprect.h, _format_bpp, _format);

	if (SDL_RenderReadPixels(_renderer, &vprect, _format, shot->pixels, shot->pitch)) {
		Log(LOG_FATAL) << "SDLRenderer::doScreenshot(): SDL_RenderReadPixels(): " << SDL_GetError();
		throw(Exception(SDL_GetError()));
	}
	if (SDL_SavePNG(shot, _screenshotFilename.c_str())) {
		Log(LOG_FATAL) << "SDLRenderer::doScreenshot(): SDL_SavePNG('" << _screenshotFilename << "'): " << SDL_GetError();
		throw(Exception(SDL_GetError()));
	}
	SDL_FreeSurface(shot);
	_screenshotFilename.clear();
}

void SDLRenderer::getOutputSize(int& w, int& h) const {
	SDL_GetRendererOutputSize(_renderer, &w, &h);
}
void SDLRenderer::setClearColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
	_r = r; _g = g; _b = b; _a = a;
}
void SDLRenderer::setFilter(const std::string& filter) {
	// this is applied in SDL_CreateTexture(), so recreate them.
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, map_filter_name(filter).c_str());
	for (auto& ri: _renderList) { ri.recreateTexture(_renderer); }
}

}
