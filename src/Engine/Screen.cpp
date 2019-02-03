/*
 * Copyright 2010-2016 OpenXcom Developers.
 *
 * This file is part of OpenXcom.
 *
 * OpenXcom is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * OpenXcom is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <algorithm>
#include <sstream>
#include <cmath>
#include <iomanip>
#include <cassert>
#include <climits>
#include <SDL.h>
#include <SDL_pnglite.h>
#include "Screen.h"
#include "Exception.h"
#include "Surface.h"
#include "Logger.h"
#include "Action.h"
#include "Options.h"
#include "CrossPlatform.h"
#include "FileMap.h"
#include "Zoom.h"
#include "Timer.h"
#include "Renderer.h"
#include "SDLRenderer.h"

namespace OpenXcom
{

const int Screen::ORIGINAL_WIDTH = 320;
const int Screen::ORIGINAL_HEIGHT = 200;

/**
 * Sets up all the internal display flags depending on
 * the current video settings.
 */
void Screen::makeVideoFlags()
{
	_flags = SDL_WINDOW_OPENGL;
	if (Options::allowResize)
	{
		_flags |= SDL_WINDOW_RESIZABLE;
	}

    // Handle display mode
	if (Options::fullscreen)
	{
		_flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
	}
	if (Options::borderless)
	{
		_flags |= SDL_WINDOW_BORDERLESS;
	}

	// Try fixing the Samsung devices - see https://bugzilla.libsdl.org/show_bug.cgi?id=2291
#ifdef __ANDROID__
	if (Options::forceGLMode)
	{
		Log(LOG_INFO) << "Setting GL format to RGB565";
		SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
		SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 6);
		SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
		//SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	}
	CrossPlatform::setSystemUI();
#endif

	_baseWidth = Options::baseXResolution;
	_baseHeight = Options::baseYResolution;
}


/**
 * Initializes a new display screen for the game to render contents to.
 * The screen is set up based on the current options.
 */
Screen::Screen() : _window(NULL), _renderer(NULL),
	_baseWidth(ORIGINAL_WIDTH), _baseHeight(ORIGINAL_HEIGHT), _scaleX(1.0), _scaleY(1.0),
	_numColors(0), _firstColor(0), _pushPalette(false), _screenshotFilename(),
	_currentScaleType(SCALE_SCREEN), _currentScaleMode(SC_STARTSTATE), _resizeAccountedFor(false)
{
	resetVideo(Options::displayWidth, Options::displayHeight);
	memset(deferredPalette, 0, 256*sizeof(SDL_Color));
}

/**
 * Deletes the buffer from memory. The display screen itself
 * is automatically freed once SDL shuts down.
 */
Screen::~Screen()
{
	delete _renderer;
}

/**
 * Returns the screen's internal buffer surface. Any
 * contents that need to be shown will be blitted to this.
 * @return Pointer to the buffer surface.
 */
SDL_Surface *Screen::getSurface()
{
	return _surface.get();
}

/**
 * Handles screen key shortcuts.
 * @param action Pointer to an action.
 */
void Screen::handle(Action *action)
{
	if (Options::debug)
	{
		if (action->getType() == SDL_KEYDOWN && action->getKeycode() == SDLK_F8 && (action->getKeymods() & KMOD_ALT) != 0)
		{
			switch(Timer::gameSlowSpeed)
			{
				case 1: Timer::gameSlowSpeed = 5; break;
				case 5: Timer::gameSlowSpeed = 15; break;
				default: Timer::gameSlowSpeed = 1; break;
			}
		}
	}

	if (action->getType() == SDL_KEYDOWN && action->getKeycode() == SDLK_RETURN && (SDL_GetModState() & KMOD_ALT) != 0)
	{
		Options::fullscreen = !Options::fullscreen;
		resetVideo(Options::displayWidth, Options::displayHeight);
	}
	else if (action->getType() == SDL_KEYDOWN && action->getKeycode() == Options::keyScreenshot)
	{
		std::ostringstream ss;
		int i = 0;
		do
		{
			ss.str("");
			ss << Options::getMasterUserFolder() << "screen" << std::setfill('0') << std::setw(3) << i << ".png";
			i++;
		}
		while (CrossPlatform::fileExists(ss.str()));
		if (action->getKeymods()) {
			// modded (alt/shift/ctrl/etc) screenshot keypresses
			// shoot the unscaled screen
			screenshot(ss.str());
		} else {
			// unmodded shoot the window
			_renderer->screenshot(ss.str());
		}
		return;
	}
}


/**
 * Renders the buffer's contents onto the screen, applying
 * any necessary filters or conversions in the process.
 * If the scaling factor is bigger than 1, the entire contents
 * of the buffer are resized by that factor (eg. 2 = doubled)
 * before being put on screen.
 */
void Screen::flip()
{
	if (_pushPalette) {
		SDL_SetPaletteColors(_surface->format->palette, deferredPalette, 0, 256);
		_pushPalette = false;
	}

#if DEBUG_SCREENSHOT_PERIOD
	static size_t flipcounter = 0;
	if (flipcounter % DEBUG_SCREENSHOT_PERIOD == 0) {
		Log(LOG_INFO) << " frame " << flipcounter;
		char fname[256];
		sprintf(fname, "frame%04zd.png", flipcounter);
		SDL_SavePNG(_surface.get(), fname);
	}
	flipcounter += 1;
#endif

	if (!_screenshotFilename.empty()) {
		if (SDL_SavePNG(_surface.get(), _screenshotFilename.c_str())) {
			Log(LOG_FATAL) << "Screen::screenshot('" << _screenshotFilename << "'):" << SDL_GetError();
			throw(Exception(SDL_GetError()));
		}
		_screenshotFilename.clear();
	}
	_renderer->flip(_surface.get());
}

/**
 * Clears all the contents out of the internal buffer.
 */
void Screen::clear()
{
	SDL_FillRect(_surface.get(), NULL, 0);
}

/**
 * Changes the 8bpp palette used to render the screen's contents.
 * @param colors Pointer to the set of colors.
 * @param firstcolor Offset of the first color to replace.
 * @param ncolors Amount of colors to replace.
 * @param immediately Apply palette changes immediately, otherwise wait for next blit.
 */
void Screen::setPalette(const SDL_Color* colors, int firstcolor, int ncolors, bool immediately)
{
	// TODO FIXME: why ever not always defer setting the palette to just before blit?
	if (_numColors && (_numColors != ncolors) && (_firstColor != firstcolor))
	{
		// an initial palette setup has not been committed to the screen yet
		// just update it with whatever colors are being sent now
		memmove(&(deferredPalette[firstcolor]), colors, sizeof(SDL_Color)*ncolors);
		_numColors = 256; // all the use cases are just a full palette with 16-color follow-ups
		_firstColor = 0;
	}
	else
	{
		memmove(&(deferredPalette[firstcolor]), colors, sizeof(SDL_Color) * ncolors);
		_numColors = ncolors;
		_firstColor = firstcolor;
	}
	_pushPalette = true;
	//SDL_SetPaletteColors(_surface->format->palette, colors, firstcolor, ncolors);
}

/**
 * Returns the screen's 8bpp palette.
 * @return Pointer to the palette's colors.
 */
const SDL_Color *Screen::getPalette() const
{
	return deferredPalette;
}

/**
 * Returns the width of the screen.
 * @return Width in pixels.
 */
int Screen::getWidth() const
{
	return _baseWidth;
}

/**
 * Returns the height of the screen.
 * @return Height in pixels
 */
int Screen::getHeight() const
{
	return _baseHeight;
}

/**
 * Recreates video: both renderer and the window
 */
void Screen::resetVideo(int width, int height)
{
	Log(LOG_INFO) << "Screen::resetVideo(" << width << ", " << height << ")";
	if (_renderer) { delete _renderer; }
	if (_window) { SDL_DestroyWindow(_window); }

	Uint32 window_flags = SDL_WINDOW_OPENGL;
	if (Options::allowResize) { window_flags |= SDL_WINDOW_RESIZABLE; }
	if (Options::fullscreen) { window_flags |= SDL_WINDOW_FULLSCREEN_DESKTOP; }
	if (Options::borderless) { window_flags |= SDL_WINDOW_BORDERLESS; }

	/* Attempt to set scaling */
	if (Options::useNearestScaler) {
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
	} else if (Options::useLinearScaler) {
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	} else if (Options::useAnisotropicScaler) {
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "best");
	}

	int winX, winY;
	if (Options::borderless) {
		winX = SDL_WINDOWPOS_CENTERED;
		winY = SDL_WINDOWPOS_CENTERED;
	} else if (!Options::fullscreen && Options::rootWindowedMode) {
		winX = Options::windowedModePositionX;
		winY = Options::windowedModePositionY;
	} else {
		winX = SDL_WINDOWPOS_UNDEFINED;
		winY = SDL_WINDOWPOS_UNDEFINED;
	}

	_window = SDL_CreateWindow("OpenXcom Extended SDL2", winX, winY, width, height, window_flags);

	if (_window == NULL) {
		Log(LOG_ERROR) << SDL_GetError();
		throw Exception(SDL_GetError());
	}
	Log(LOG_INFO) << "Created a window, size is: " << width << "x" << height;

	_renderer = new SDLRenderer(_window, -1, SDL_RENDERER_ACCELERATED);

	if (!_renderer)	{
		Log(LOG_ERROR) << SDL_GetError();
		throw Exception(SDL_GetError());
	}

	// fix up scaling.
	setMode(_currentScaleMode);
}

/**
 * Resets the screen surfaces based on the current display options,
 * as they don't automatically take effect.
 * @param resetVideo Reset display surface.
 */
void Screen::resetDisplay(bool resetVideo)
{
}

/**
 * Saves a screenshot of the screen's contents.
 * @param filename Filename of the PNG file.
 */
void Screen::screenshot(const std::string &filename)
{
	_screenshotFilename = filename;
}

/**
 * Check whether a 32bpp scaler has been selected.
 * @return if it is enabled with a compatible resolution.
 */
bool Screen::use32bitScaler()
{
	int w = Options::displayWidth;
	int h = Options::displayHeight;
	int baseW = Options::baseXResolution;
	int baseH = Options::baseYResolution;
	int maxScale = 0;

	if (Options::useHQXFilter)
	{
		maxScale = 4;
	}
	else if (Options::useXBRZFilter)
	{
		maxScale = 6;
	}

	for (int i = 2; i <= maxScale; i++)
	{
		if (w == baseW * i && h == baseH * i)
		{
			return true;
		}
	}
	return false;
}

/**
 * Check if OpenGL is enabled.
 * @return if it is enabled.
 */
bool Screen::useOpenGL()
{
#ifdef __NO_OPENGL
	return false;
#else
	return Options::useOpenGL;
#endif
}

/**
 * Gets the Horizontal offset from the mid-point of the screen, in pixels.
 * More like - get X coordinate of the top left corner of the original
 * 320x200 screen on the current screen, because it might be bigger.
 * @return the horizontal offset.
 */
int Screen::getDX() const
{
	return (_baseWidth - ORIGINAL_WIDTH) / 2;
}

/**
 * Gets the Vertical offset from the mid-point of the screen, in pixels.
 * @return the vertical offset.
 */
int Screen::getDY() const
{
	return (_baseHeight - ORIGINAL_HEIGHT) / 2;
}

/**
 * Updates scaling in case the window got resized.
 * returns the delta in dX and dY.
 */
void Screen::updateScale(int& dX, int& dY)
{
	dX = getWidth();
	dY = getHeight();
	setMode(_currentScaleMode);
	dX = getWidth() - dX;
	dY = getHeight() - dY;
}

/**
 * Depending on the mode this sets up scaling.
 */
void Screen::setMode(ScreenMode mode)
{
	if (_resizeAccountedFor && _currentScaleMode == mode) {
		return;
	}
	_currentScaleMode = mode;
	int type;
	switch (mode) {
		case SC_STARTSTATE:
			type = SCALE_SCREEN;
			break;
		case SC_INTRO:
			type = SCALE_ORIGINAL;
			break;
		case SC_GEOSCAPE:
			type = Options::geoscapeScale;
			break;
		case SC_BATTLESCAPE:
			type = Options::battlescapeScale;
			break;
		case SC_INFOSCREEN:
			if (Options::maximizeInfoScreens) {
				type = SCALE_ORIGINAL;
			} else {
				return;
			}
			break;
		case SC_NONE:
		case SC_ORIGINAL:
		default:
			// dunno. maybe don't accept blits.
			type = SCALE_ORIGINAL;
			break;
	}

	// if nothing changed, do nothing.
	if (_resizeAccountedFor && type == _currentScaleType) {
		return;
	}
	_currentScaleType = type;
	// the type from above determines logical game screen size.
	int target_width, target_height;
	_renderer->getOutputSize(target_width, target_height);

	Options::displayWidth = target_width;   //FIXME: those obnoxious globals..
	Options::displayHeight = target_height;

	switch (type) {
		case SCALE_15X:
			_baseWidth  = ORIGINAL_WIDTH * 1.5;
			_baseHeight = ORIGINAL_HEIGHT * 1.5;
			break;
		case SCALE_2X:
			_baseWidth  = ORIGINAL_WIDTH * 2;
			_baseHeight = ORIGINAL_HEIGHT * 2;
			break;
		case SCALE_SCREEN_DIV_6:
			_baseWidth  = target_width / 6;
			_baseHeight = target_width / 6;
			break;
		case SCALE_SCREEN_DIV_5:
			_baseWidth  = target_width / 5;
			_baseHeight = target_width / 5;
			break;
		case SCALE_SCREEN_DIV_4:
			_baseWidth  = target_width / 4;
			_baseHeight = target_width / 4;
			break;
		case SCALE_SCREEN_DIV_3:
			_baseWidth  = target_width / 3;
			_baseHeight = target_width / 3;
			break;
		case SCALE_SCREEN_DIV_2:
			_baseWidth  = target_width / 2;
			_baseHeight = target_width  / 2;
			break;
		case SCALE_SCREEN:
			_baseWidth  = target_width;
			_baseHeight = target_width;
			break;
		case SCALE_ORIGINAL:
		default:
			_baseWidth  = ORIGINAL_WIDTH;
			_baseHeight = ORIGINAL_HEIGHT;
			break;
	}

	// make sure we don't get below the original
	if (_baseWidth < ORIGINAL_WIDTH) { _baseWidth  = ORIGINAL_WIDTH; }
	if (_baseHeight < ORIGINAL_HEIGHT) { _baseHeight  = ORIGINAL_HEIGHT; }

	// set the source rect for the renderer
	SDL_Rect baseRect;
	baseRect.x = baseRect.y = 0;
	baseRect.w = _baseWidth;
	baseRect.h = _baseHeight;
	_renderer->setInternalRect(&baseRect);

	std::tie(_buffer, _surface) = Surface::NewPair8Bit(_baseWidth, _baseHeight);
	SDL_SetPaletteColors(_surface->format->palette, deferredPalette, 0, 256);
	SDL_SetColorKey(_surface.get(), 0, 0); // turn off color key! FIXME: probably not needed.

	Log(LOG_INFO) << "Screen::setMode(): logical size " << _baseWidth << "x" << _baseHeight;

	// Having determined the logical screen size (srcRect/internalRect) we can now
	// calculate scaling factors and dstRect / outputRect
	// This is where keepAspectRatio and nonSquarePixelRatio come into play.
	// I think we should drop keepAspectRatio and do it always letterboxed.
	// besides, keepAspectRatio makes nonSquarePixelRatio impossible.
	// outRect is in actual pixels.
	SDL_Rect outRect;
	if (!Options::keepAspectRatio) {
		// this just blows the screen up to completely fill the window.
		outRect.x = 0;
		outRect.y = 0;
		outRect.w = target_width;
		outRect.h = target_height;
		_renderer->setOutputRect(&outRect);
		_scaleX = target_width / _baseWidth;
		_scaleY = target_height / _baseHeight;
		_leftBlackBand = 0;
		_topBlackBand = 0;
		Log(LOG_INFO) << "Screen::setMode(): scaled size " << target_width << "x" << target_height << ", no letterboxing.";
		_resizeAccountedFor = true;
		return;
	}

	// now we're letterboxing.
	// first determine which of width or height will fit the window

	double pixelRatioY = Options::nonSquarePixelRatio ? 1.2 : 1.0;
	double scaleX = target_width / _baseWidth;
	double scaleY = target_height / (_baseHeight * pixelRatioY);
	double scale;

	if (scaleX < scaleY) { // width wins
		scale = scaleX;
	} else { // height wins
		scale = scaleY;
	}

	// now would be the time to clamp it down to an integer.
	if (Options::integerRatioScaling) {
		scale = floor(scale);
	}
	_scaleX = _scaleY = scale;

	int scaledWidth = scale * _baseWidth;
	int scaledHeight = scale * _baseHeight * pixelRatioY; // this breaks the integer ratio but what you can do

	Log(LOG_INFO) << "Screen::setMode(): scaled size " << scaledWidth << "x" << scaledHeight
				  << ", target size " << target_width << "x" << target_height;

	_leftBlackBand = ( target_width - scaledWidth)/2;
	_topBlackBand = ( target_height - scaledHeight)/2;
	outRect.x = _leftBlackBand;
	outRect.y = _topBlackBand;
	outRect.w = scaledWidth;
	outRect.h = scaledHeight;
	_renderer->setOutputRect(&outRect);
	_resizeAccountedFor = true;
}

SDL_Window * Screen::getWindow() const
{
	return _window;
}

std::unique_ptr<Action> Screen::makeAction(const SDL_Event *ev) const
{
	return std::unique_ptr<Action>(new Action(ev, _scaleX, _scaleY, _topBlackBand, _leftBlackBand));
}

void Screen::warpMouse(int x, int y)
{
#ifndef __MOBILE__
	SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);
	SDL_WarpMouseInWindow(_window, x * _scaleX + _leftBlackBand, y * _scaleY + _topBlackBand);
	SDL_EventState(SDL_MOUSEMOTION, SDL_ENABLE);
#endif
}

void Screen::warpMouseRelative(int dx, int dy)
{
#ifndef __MOBILE__
	int x, y;
	SDL_GetMouseState(&x, &y);
	SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);
	SDL_WarpMouseInWindow(_window, x + dx * _scaleX, y + dy * _scaleY);
	SDL_EventState(SDL_MOUSEMOTION, SDL_ENABLE);
#endif
}

void Screen::setWindowGrab(int grab)
{
	// was : Breaks stuff. Hard. FIXME: why? what?
	SDL_SetWindowGrab(_window, grab ? SDL_TRUE : SDL_FALSE);
}
}
