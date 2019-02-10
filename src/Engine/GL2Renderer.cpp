


#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_pnglite.h>
#include "GL2Renderer.h"
#include "Exception.h"
#include "Logger.h"
#include "FileMap.h"


namespace OpenXcom
{

RenderItemGL2::RenderItemGL2() : _surface(0), _textureName(0), _srcRect(), _dstRect(), _visible(false) { }  // not needed at all I suspect.

RenderItemGL2::~RenderItemGL2()
{
	if (_surface) { SDL_FreeSurface(_surface); }
}

void RenderItemGL2::setInternalRect(SDL_Rect *srcRect, int bpp, Uint32 format, bool blend)
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
	recreateTexture();

	Log(LOG_INFO) << "[GLRenderer] setInternalRect(): Texture is now "
		<< _srcRect.w << "x" << _srcRect.h << " " << SDL_GetPixelFormatName( format );
}

void RenderItemGL2::recreateTexture()
{
	if (!_surface) { return; }
	/*
	 * fuck up the PBO.
	if (_texture) { SDL_DestroyTexture(_texture); }
	_texture = SDL_CreateTexture(renderer, _format, SDL_TEXTUREACCESS_STREAMING, _srcRect.w, _srcRect.h);
	if (_texture == NULL) {
		throw Exception(SDL_GetError());
	}
	SDL_SetTextureBlendMode(_texture, _blend ? SDL_BLENDMODE_BLEND : SDL_BLENDMODE_NONE);
	*/
}

void RenderItemGL2::setOutputRect(SDL_Rect *dstRect)
{
	if (dstRect == NULL) {
		_visible = false;
		return;
	}
	_dstRect.x = dstRect->x;
	_dstRect.y = dstRect->y;
	_dstRect.w = dstRect->w;
	_dstRect.h = dstRect->h;
	//Log(LOG_INFO) << "[GLRenderer] setOutputRect(): Output "
	//	<< _dstRect.w << "x" << _dstRect.h << " at " << _dstRect.x << "x" << _dstRect.y;
}

void RenderItemGL2::updateTexture(SDL_Surface *surface)
{
	if (surface == NULL) {
		_visible = false;
		return;
	} else {
		_visible = true;
	}
#if 0
	Log(LOG_INFO) << "[GLRenderer] src: "<<srcSurface->w<<"x"<<srcSurface->h<<" "
		<< SDL_GetPixelFormatName(srcSurface->format->format)
		<< " dst: "<<_surface->w<<"x"<<_surface->h<<" srcR: "<<_srcRect.w <<"x"<< _srcRect.h
		<< " dstR: "<<_dstRect.w <<"x"<<_dstRect.h;
#endif

	//SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_NONE);
	if (SDL_BlitSurface(surface, NULL, _surface, NULL)) {
		SDL_BlendMode srcBM, dstBM;
		SDL_GetSurfaceBlendMode(surface, &srcBM);
		SDL_GetSurfaceBlendMode(_surface, &dstBM);
		Log(LOG_ERROR)<< "[GLRenderer] SDL_BlitSurface(): " << SDL_GetError();
		Log(LOG_ERROR) << " src pf: " << SDL_GetPixelFormatName(surface->format->format)
			<< " BM: " << srcBM
			<< " dst pf: " << SDL_GetPixelFormatName(_surface->format->format)
			<< " BM: " << (int) dstBM;

		throw Exception(SDL_GetError());
	}
	// _surface must be the same size as _texture at all times
	// hence the NULL for the rect
	/* fuck up the PBO.
	if (SDL_UpdateTexture(_texture, NULL, _surface->pixels, _surface->pitch)) {
		Log(LOG_ERROR)<< "[GLRenderer] SDL_UpdateTexture(): " << SDL_GetError();
		throw Exception(SDL_GetError());
	}*/
}

const std::vector<std::string> GL2Renderer::listFilters()
{
	std::vector<std::string> rv;
	auto filters = FileMap::filterFiles(FileMap::getVFolderContents(GL_FOLDER), GL_EXT);
	for (const auto& file : filters) {
		std::string path = GL_FOLDER + file;
		std::string name = file.substr(0, file.length() - GL_EXT.length() - 1) + GL_STRING;
		rv.push_back(ucWords(name));
	}
	return rv;
}

const std::vector<std::string> GL2Renderer::listDrivers()
{
	std::vector<std::string> rv;
	rv.push_back("GL2");
	return rv;
}

static std::string map_filter_name(const std::string& filtername)
{
	std::string rv = "";
	auto filters = FileMap::filterFiles(FileMap::getVFolderContents(GL2Renderer::GL_FOLDER), GL2Renderer::GL_EXT);
	for (const auto& file : filters) {
		std::string path = GL2Renderer::GL_FOLDER + file;
		std::string name = file.substr(0, file.length() - GL2Renderer::GL_EXT.length() - 1) + GL2Renderer::GL_STRING;
		if (filtername == GL2Renderer::ucWords(name)) {
			if (rv == "") {
				rv = path;
			}
			return path;
		}
	}
	return rv;
}

GL2Renderer::GL2Renderer(SDL_Window *window):
	_window(window), _glContext (NULL), _r(0), _g(0), _b(0), _a(255), _format(SDL_PIXELFORMAT_ABGR8888),
	_screenshotFilename()
{
	// we trust the window was (re)created with SDL_WINDOW_OPENGL flag set.
	// now set whatever attributes we want.
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);  // FIXME / TBD: ?
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3); // maybe 2. if it works.
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0); // SDL_GL_CONTEXT_DEBUG_FLAG

	_glContext = SDL_GL_CreateContext(window);

	if ( _glContext == NULL)  // FIXME: look in the sources, what does it even return?
	{
		Log(LOG_FATAL) << "[GLRenderer] Couldn't create glContext; error message: " << SDL_GetError();
		throw(Exception(SDL_GetError()));
	}

	Log(LOG_INFO) << "[GLRenderer] Created new GLRenderer, in: " << SDL_GetPixelFormatName(_format);
	{
		int rs, gs, bs, as, ds, ss, db, maj, min, mask, flags;
		SDL_GL_GetAttribute(SDL_GL_RED_SIZE, &rs);  // kinda might have to do before window, and 565 for samsung.
		SDL_GL_GetAttribute(SDL_GL_GREEN_SIZE, &gs);
		SDL_GL_GetAttribute(SDL_GL_BLUE_SIZE, &bs);
		SDL_GL_GetAttribute(SDL_GL_ALPHA_SIZE, &as);
		SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE, &ds);
		SDL_GL_GetAttribute(SDL_GL_STENCIL_SIZE, &ss);
		SDL_GL_GetAttribute(SDL_GL_DOUBLEBUFFER, &db);
		SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &maj );
		SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &min );
		SDL_GL_GetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, &mask);
		SDL_GL_GetAttribute(SDL_GL_CONTEXT_FLAGS, &flags);
		Log(LOG_INFO) << "[GLRenderer]     rgba=" << rs << gs << bs << as << " dp=" << ds << " ss=" << ss <<
			" DB=" << db << " v=" << maj << "." << min << " mask=" << mask << " flags" << flags;
		glGetIntegerv(GL_MAJOR_VERSION, &maj);
		glGetIntegerv(GL_MINOR_VERSION, &min);
		glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
		Log(LOG_INFO) << "[GLRenderer]     glGet ver=" << maj << "." << min << " flags=" << flags;
	}

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

	Uint32 r, g, b, a;
	if (!SDL_PixelFormatEnumToMasks(_format, &_format_bpp, &r, &g, &b, &a)) {
		Log(LOG_FATAL) << "[GLRenderer] Couldn't get bpp for "
		<< SDL_GetPixelFormatName(_format) << ": " << SDL_GetError();
		throw(Exception(SDL_GetError()));
	}
}

GL2Renderer::~GL2Renderer(void)
{
	SDL_GL_DeleteContext( _glContext );
}

unsigned GL2Renderer::getTexture()
{
	_renderList.resize(_renderList.size() + 1);
	glGenTextures(1, &(_renderList[_renderList.size() - 1]._textureName));
	return _renderList.size() - 1;
}

/**
 * Sets up internal structures: an SDL_Surface for pixel format conversion
 * and an SDL_Texture to update and render.
 * @param item - texture id
 * @param srcRect - size of the expected surfaces
 * @param blend - whether to blit with or without alpha
 */
void GL2Renderer::setInternalRect(unsigned item, SDL_Rect *srcRect, bool blend)
{
	_renderList.at(item).setInternalRect(srcRect, _format_bpp, _format, blend);
}

/**
 * Sets the blit destination
 * @param dstRect - blit destination in window coordinates
 */
void GL2Renderer::setOutputRect(unsigned item, SDL_Rect *dstRect)
{
	_renderList.at(item).setOutputRect(dstRect);
}

/**
 * Updates pixel data
 * @param surface - what to blit. Size must match what was set up in setInternalRect().
 */
void GL2Renderer::updateTexture(unsigned item, SDL_Surface *surface)
{
	_renderList.at(item).updateTexture(surface);
}

/**
 * Performs the blits and presents the result.
 * Also does the screenshot if one was requested.
 */
void GL2Renderer::flip()
{
	glClearColor(_r, _g, _b, _a);
	glClear(GL_COLOR_BUFFER_BIT);

	// rendercopy does the scaling
	for (auto& item: _renderList) {
		if (!item.visible()) {
			continue;
		}
		// set uniforms and crap.

		// glDrawShit()
#if 0
		if (SDL_RenderCopy(0, item.textureName(), NULL, item.dstRect())) {
			Log(LOG_ERROR)<< "[GLRenderer] SDL_RenderCopy(): " << SDL_GetError();
			throw Exception(SDL_GetError());
		}
#endif
	}

	// commit GL
	SDL_GL_SwapWindow(_window);
	// do a screenshot if requested
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
void GL2Renderer::screenshot(const std::string &filename) {
	_screenshotFilename = filename;
}

/**
 * Does the actual screenshotting
 */
void GL2Renderer::doScreenshot() {
	if (_screenshotFilename.empty()) { return; }

	int w, h;
	getOutputSize(w, h);
	SDL_Surface *shot = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_RGBA32);
	glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, shot->pixels);

	if (SDL_SavePNG(shot, _screenshotFilename.c_str())) {
		Log(LOG_FATAL) << "GLRenderer::doScreenshot(): SDL_SavePNG('" << _screenshotFilename << "'): " << SDL_GetError();
		throw(Exception(SDL_GetError()));
	}
	SDL_FreeSurface(shot);
	_screenshotFilename.clear();
}

void GL2Renderer::getOutputSize(int& w, int& h) const {
	SDL_GL_GetDrawableSize(_window, &w, &h);
}
void GL2Renderer::setClearColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
	_r = r; _g = g; _b = b; _a = a;
}
void GL2Renderer::setFilter(const std::string& filter) {
	// for (auto& ri: _renderList) { ri.recreateTexture(); }
	auto shader_filter_whatever = map_filter_name(filter);
	// now fucking set the shader.
	// also respect if it wants teh GL_LINEAR_SHIT

}

/**
 * Uppercases all the words in a string.
 * @param str Source string.
 * @return Destination string.
 */
std::string GL2Renderer::ucWords(std::string str)
{
	for (size_t i = 0; i < str.length(); ++i)
	{
		if (i == 0)
			str[0] = toupper(str[0]);
		else if (str[i] == ' ' || str[i] == '-' || str[i] == '_')
		{
			str[i] = ' ';
			if (str.length() > i + 1)
				str[i + 1] = toupper(str[i + 1]);
		}
	}
	return str;
}

const std::string GL2Renderer::GL_EXT = "OpenGL.shader";
const std::string GL2Renderer::GL_FOLDER = "Shaders/";
const std::string GL2Renderer::GL_STRING = "*";

}
