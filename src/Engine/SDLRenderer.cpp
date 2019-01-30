


#include <assert.h>
#include <SDL.h>
#include <SDL_pnglite.h>
#include "SDLRenderer.h"
#include "Exception.h"
#include "Logger.h"


namespace OpenXcom
{

SDLRenderer::SDLRenderer(SDL_Window *window, int driver, Uint32 flags): _window(window), _renderer(NULL), _texture(NULL), _format(SDL_PIXELFORMAT_ARGB8888), _surface(0)
{
	listSDLRendererDrivers();
	_renderer = SDL_CreateRenderer(window, -1, flags);
	_srcRect.x = _srcRect.y = _srcRect.w = _srcRect.h = 0;
	_dstRect.x = _dstRect.y = _dstRect.w = _dstRect.h = 0;
	if (_renderer == NULL)
	{
		Log(LOG_ERROR) << "[SDLRenderer] Couldn't create renderer; error message: " << SDL_GetError();
		throw Exception(SDL_GetError());
	}
	Log(LOG_INFO) << "[SDLRenderer] Renderer created";
	const char *scaleHint = SDL_GetHint(SDL_HINT_RENDER_SCALE_QUALITY);
	if (!scaleHint)
	{
		_scaleHint = "nearest";
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, _scaleHint.c_str());
	}
	else
	{
		_scaleHint = scaleHint;
	}
	SDL_RendererInfo info;
	SDL_GetRendererInfo(_renderer, &info);
	Log(LOG_INFO) << "[SDLRenderer] Created new SDLRenderer, using " << info.name;
	Log(LOG_INFO) << "[SDLRenderer] Current scaler is " << _scaleHint;
	for (Uint32 j = 0; j < info.num_texture_formats; ++j) {
		Log(LOG_INFO) << "[SDLRenderer]     Texture format " << j << ": " << SDL_GetPixelFormatName(info.texture_formats[j]);
	}
	_format = info.texture_formats[0];
}

void SDLRenderer::setInternalRect(SDL_Rect *srcRect)
{
	// Internal rectangle should not have any X or Y offset.
	assert(srcRect->x == 0 && srcRect->y == 0);

	if (_texture) { SDL_DestroyTexture(_texture); }
	if (_surface) { SDL_FreeSurface(_surface); }

	_texture = SDL_CreateTexture(_renderer, _format,
					SDL_TEXTUREACCESS_STREAMING,
					srcRect->w, srcRect->h);
	int bpp;
	Uint32 r, g, b, a;
	SDL_PixelFormatEnumToMasks(_format, &bpp, &r, &g, &b, &a);
	_surface = SDL_CreateRGBSurfaceWithFormat(0, srcRect->w, srcRect->h, bpp, _format);
	SDL_SetSurfaceBlendMode(_surface, SDL_BLENDMODE_NONE);

	if (_texture == NULL || _surface == NULL) {
		throw Exception(SDL_GetError());
	}

	_srcRect.w = srcRect->w;
	_srcRect.h = srcRect->h;
	Log(LOG_INFO) << "[SDLRenderer] setInternalRect(): Texture is now "
		<< _srcRect.w << "x" << _srcRect.h << " " << SDL_GetPixelFormatName(_format);
}

void SDLRenderer::setOutputRect(SDL_Rect *dstRect)
{
	_dstRect.x = dstRect->x;
	_dstRect.y = dstRect->y;
	_dstRect.w = dstRect->w;
	_dstRect.h = dstRect->h;
	Log(LOG_INFO) << "[SDLRenderer] setOutputRect(): Output "
		<< _dstRect.w << "x" << _dstRect.h << " at " << _dstRect.x << "x" << _dstRect.y;
}


SDLRenderer::~SDLRenderer(void)
{
	SDL_DestroyTexture(_texture);
	SDL_DestroyRenderer(_renderer);
}

void SDLRenderer::flip(SDL_Surface *srcSurface)
{
	static size_t fc = 0;

	if (fc % 60 == 0) {
		Log(LOG_INFO)<<" frame "<<fc;
		char fname[256];
		sprintf(fname, "frame%04zd.png", fc);
		SDL_SavePNG(srcSurface, fname);
	}
	fc += 1;


	// okay, srcSurface may be ARGB32 or INDEXED8
	// and also may or may not be the size of dstRect.
#if 0
	Log(LOG_INFO) << "[SDLRenderer] src: "<<srcSurface->w<<"x"<<srcSurface->h<<" "
		<< SDL_GetPixelFormatName(srcSurface->format->format)
		<< " dst: "<<_surface->w<<"x"<<_surface->h<<" srcR: "<<_srcRect.w <<"x"<< _srcRect.h
		<< " dstR: "<<_dstRect.w <<"x"<<_dstRect.h;
#endif

	//SDL_SetSurfaceBlendMode(srcSurface, SDL_BLENDMODE_NONE);
	if (SDL_BlitSurface(srcSurface, NULL, _surface, NULL)) {
		SDL_BlendMode srcBM, dstBM;
		SDL_GetSurfaceBlendMode(srcSurface, &srcBM);
		SDL_GetSurfaceBlendMode(_surface, &dstBM);
		Log(LOG_ERROR)<< "[SDLRenderer] SDL_BlitSurface(): " << SDL_GetError();
		Log(LOG_ERROR) << " src pf: " << SDL_GetPixelFormatName(srcSurface->format->format)
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

	// flip starts here.
	// the most honorable cursor should be rendercopied too.

	SDL_SetRenderDrawColor(_renderer, fc%255, fc%255, fc%255, 255);
	if (SDL_RenderClear(_renderer)) {
		Log(LOG_ERROR)<< "[SDLRenderer] SDL_RenderClear(): " << SDL_GetError();
		throw Exception(SDL_GetError());
	}
	// rendercopy does the scaling
	if (SDL_RenderCopy(_renderer, _texture, NULL, &_dstRect)) {
		Log(LOG_ERROR)<< "[SDLRenderer] SDL_RenderCopy(): " << SDL_GetError();
		throw Exception(SDL_GetError());
	}
	SDL_RenderPresent(_renderer);
}

void SDLRenderer::listSDLRendererDrivers()
{
	int numRenderDrivers = SDL_GetNumRenderDrivers();
	Log(LOG_INFO) << "[SDLRenderer] Listing available rendering drivers:";
	Log(LOG_INFO) << "[SDLRenderer]  Number of drivers: " << numRenderDrivers;
	for (int i = 0; i < numRenderDrivers; ++i)
	{
		SDL_RendererInfo info;
		SDL_GetRenderDriverInfo(i, &info);
		Log(LOG_INFO) << "[SDLRenderer]  Driver " << i << ": " << info.name;
		Log(LOG_INFO) << "[SDLRenderer]    Number of texture formats: " << info.num_texture_formats;
		for (Uint32 j = 0; j < info.num_texture_formats; ++j)
		{
			Log(LOG_INFO) << "[SDLRenderer]     Texture format " << j << ": " << SDL_GetPixelFormatName(info.texture_formats[j]);
		}
	}
}

void SDLRenderer::screenshot(const std::string &filename) const
{
	// do it on the next flip maybe. and what to save? render stuff obviously.
	if (SDL_SavePNG(_surface, filename.c_str())) {
		throw(SDL_GetError());
	}
}

}
