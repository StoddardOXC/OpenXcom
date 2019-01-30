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
#include "OpenGLRenderer.h"

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

#if 0
	if (Options::asyncBlit)
	{
		_flags |= SDL_ASYNCBLIT;
	}
	if (useOpenGL())
	{
		_flags = SDL_OPENGL;
		SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 5 );
		SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 5 );
		SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 5 );
		SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 16 );
		SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
	}
#endif

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

	//_bpp = (use32bitScaler() || useOpenGL()) ? 32 : 8;
	_bpp = 32;
	_baseWidth = Options::baseXResolution;
	_baseHeight = Options::baseYResolution;
}


/**
 * Initializes a new display screen for the game to render contents to.
 * The screen is set up based on the current options.
 */
Screen::Screen() : _window(NULL), _renderer(NULL),
	_baseWidth(ORIGINAL_WIDTH), _baseHeight(ORIGINAL_HEIGHT), _scaleX(1.0), _scaleY(1.0),
	_numColors(0), _firstColor(0), _pushPalette(false),
	_prevWidth(0), _prevHeight(0)
{
	resetDisplay();
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
		if (action->getDetails()->type == SDL_KEYDOWN && action->getDetails()->key.keysym.sym == SDLK_F8)
		{
			switch(Timer::gameSlowSpeed)
			{
				case 1: Timer::gameSlowSpeed = 5; break;
				case 5: Timer::gameSlowSpeed = 15; break;
				default: Timer::gameSlowSpeed = 1; break;
			}
		}
	}

	if (action->getDetails()->type == SDL_KEYDOWN && action->getDetails()->key.keysym.sym == SDLK_RETURN && (SDL_GetModState() & KMOD_ALT) != 0)
	{
		Options::fullscreen = !Options::fullscreen;
		resetDisplay();
	}
	else if (action->getDetails()->type == SDL_KEYDOWN && action->getDetails()->key.keysym.sym == Options::keyScreenshot)
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
		screenshot(ss.str());
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
	int w, h;
	SDL_GetWindowSize(_window, &w, &h);
	return w;
}

/**
 * Returns the height of the screen.
 * @return Height in pixels
 */
int Screen::getHeight() const
{
	int w, h;
	SDL_GetWindowSize(_window, &w, &h);
	return h;
}

/**
 * Resets the screen surfaces based on the current display options,
 * as they don't automatically take effect.
 * @param resetVideo Reset display surface.
 */
void Screen::resetDisplay(bool resetVideo)
{
	int width = Options::displayWidth;
	int height = Options::displayHeight;
	bool switchRenderer = false;
	if (!_renderer)
	{
		switchRenderer = true;
	}
	else
	{
		if ( (Options::useOpenGL && _renderer->getRendererType() == RENDERER_SDL2)
			|| (!Options::useOpenGL && _renderer->getRendererType() == RENDERER_OPENGL) )
		{
			switchRenderer = true;
		}
	}
	makeVideoFlags();

	Log(LOG_INFO) << "resetDisplay( resetVideo="<<(resetVideo? "true":"false")<<")";
	Log(LOG_INFO) << "Current _baseWidth x _baseHeight: " << _baseWidth << "x" << _baseHeight;

	std::tie(_buffer, _surface) = Surface::NewPair8Bit(_baseWidth, _baseHeight);
	SDL_SetPaletteColors(_surface->format->palette, deferredPalette, 0, 256);
	SDL_SetColorKey(_surface.get(), 0, 0); // turn off color key! FIXME: probably not needed.

	if (resetVideo)
	{
		/* FIXME: leak? */  // what leak?
		Log(LOG_INFO) << "Attempting to set display to " << width << "x" << height << "x" << _bpp << "...";
		/* Attempt to set scaling */
		if (Options::useNearestScaler)
		{
			SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
		}
		else if (Options::useLinearScaler)
		{
			SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
		}
		else if (Options::useAnisotropicScaler)
		{
			SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "best");
		}
		// Hack: you have to destroy and recreate a window to make it resizable.
		if (_window)
		{
			if ((SDL_GetWindowFlags(_window) & SDL_WINDOW_RESIZABLE) != (_flags & SDL_WINDOW_RESIZABLE))
			{
				if (_renderer)
				{
					delete _renderer;
					_renderer = NULL;
				}
				SDL_DestroyWindow(_window);
				_window = NULL;
			}
		}
		/* Now, we only need to create a window AND a renderer when we have none*/
		if (_window == NULL)
		{
			Log(LOG_INFO) << "Attempting to create a new window since we have none yet";
			int winX, winY;
			if (Options::borderless)
			{
				winX = SDL_WINDOWPOS_CENTERED;
				winY = SDL_WINDOWPOS_CENTERED;
			}
			else
			{
				// FIXME: Check if this code is correct w.r.t. latest patches
				//if ((Options::windowedModePositionX != -1) && (Options::windowedModePositionY != -1))
				if (!Options::fullscreen && Options::rootWindowedMode)
				{
					winX = Options::windowedModePositionX;
					winY = Options::windowedModePositionY;
				}
				else
				{
					winX = SDL_WINDOWPOS_UNDEFINED;
					winY = SDL_WINDOWPOS_UNDEFINED;
				}
			}
			_window = SDL_CreateWindow("OpenXcom", winX, winY, width, height, _flags);
			/* In case something went horribly wrong */
			if (_window == NULL)
			{
				Log(LOG_ERROR) << SDL_GetError();
				throw Exception(SDL_GetError());
			}
			Log(LOG_INFO) << "Created a window, size is: " << width << "x" << height;
		}
		else
		{
#if 0 // FIXME? doesn't seem to be needed at all
#ifndef __ANDROID__
			SDL_SetWindowSize(_window, width, height);
#endif
#endif
			SDL_SetWindowBordered(_window, Options::borderless? SDL_FALSE : SDL_TRUE);
			SDL_SetWindowFullscreen(_window, Options::fullscreen? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
		}

		/* By this point we have a window and no renderer, so it's time to make one! */
		if (switchRenderer)
		{
			delete _renderer;
			_renderer = NULL;
		}

		if (!_renderer)
		{
//#ifndef __NO_OPENGL // okay, we don't have an OpenGL renderer yet.
#if 0
			if (Options::useOpenGL)
			{
				_renderer = new OpenGLRenderer(_window);
			}
			else
			{
				_renderer = new SDLRenderer(_window, -1, SDL_RENDERER_ACCELERATED);
			}
#else
			_renderer = new SDLRenderer(_window, -1, SDL_RENDERER_ACCELERATED);
#endif
			//_renderer = new OpenGLRenderer(_window/*, -1, SDL_RENDERER_ACCELERATED */);
			//_renderer->setPixelFormat(_surface->format->format); no fucking setting pixel formats here.
		}

		Log(LOG_INFO) << "    Display set to " << getWidth() << "x" << getHeight() << "x32 (32 is LIES).";

		/* Save new baseWidth and baseHeight */
		_prevWidth = _baseWidth;
		_prevHeight = _baseHeight;
	}
	else
	{
		clear();
	}

	assert (_window != 0 && _renderer != 0);

	Options::displayWidth = getWidth();
	Options::displayHeight = getHeight();
	_scaleX = getWidth() / (double)_baseWidth;
	_scaleY = getHeight() / (double)_baseHeight;
	Log(LOG_INFO) << "    Pre-bar scales: _scaleX = " << _scaleX << ", _scaleY = " << _scaleY;

	double pixelRatioY = 1.0;
	if (Options::nonSquarePixelRatio && !Options::allowResize)
	{
		pixelRatioY = 1.2;
	}
	bool cursorInBlackBands;
	if (!Options::keepAspectRatio)
	{
		cursorInBlackBands = false;
	}
#ifndef __ANDROID__
	else if (Options::fullscreen)
	{
		cursorInBlackBands = Options::cursorInBlackBandsInFullscreen;
	}
	else if (!Options::borderless)
	{
		cursorInBlackBands = Options::cursorInBlackBandsInWindow;
	}
	else
	{
		cursorInBlackBands = Options::cursorInBlackBandsInBorderlessWindow;
	}
#else
	else {
	// Force the scales to be equal?
		cursorInBlackBands = true;
	}
#endif

	if (_scaleX > _scaleY && Options::keepAspectRatio)
	{
		int targetWidth = (int)floor(_scaleY * (double)_baseWidth);
		_topBlackBand = _bottomBlackBand = 0;
		_leftBlackBand = (getWidth() - targetWidth) / 2;
		if (_leftBlackBand < 0)
		{
			_leftBlackBand = 0;
		}
		_rightBlackBand = getWidth() - targetWidth - _leftBlackBand;
		_cursorTopBlackBand = 0;

		if (cursorInBlackBands)
		{
			_scaleX = _scaleY;
			_cursorLeftBlackBand = _leftBlackBand;
		}
		else
		{
			_cursorLeftBlackBand = 0;
		}
	}
	else if (_scaleY > _scaleX && Options::keepAspectRatio)
	{
		int targetHeight = (int)floor(_scaleX * (double)_baseHeight * pixelRatioY);
		_topBlackBand = (getHeight() - targetHeight) / 2;
		if (_topBlackBand < 0)
		{
			_topBlackBand = 0;
		}
		_bottomBlackBand = getHeight() - targetHeight - _topBlackBand;
		if (_bottomBlackBand < 0)
		{
			_bottomBlackBand = 0;
		}
		_leftBlackBand = _rightBlackBand = 0;
		_cursorLeftBlackBand = 0;

		if (cursorInBlackBands)
		{
			_scaleY = _scaleX;
			_cursorTopBlackBand = _topBlackBand;
		}
		else
		{
			_cursorTopBlackBand = 0;
		}
	}
	else
	{
		_topBlackBand = _bottomBlackBand = _leftBlackBand = _rightBlackBand = _cursorTopBlackBand = _cursorLeftBlackBand = 0;
	}
	Log(LOG_INFO) << "    Scale (post-bar): scaleX = " << _scaleX << ", scaleY = " << _scaleY;
	Log(LOG_INFO) << "    Black bars: top: " << _topBlackBand << ", left: " << _leftBlackBand;

	SDL_Rect baseRect;
	baseRect.x = baseRect.y = 0;
	baseRect.w = _baseWidth;
	baseRect.h = _baseHeight;
	_renderer->setInternalRect(&baseRect);
	SDL_Rect outRect;
	outRect.x = _leftBlackBand;
	outRect.y = _topBlackBand;
	outRect.w = getWidth() - _leftBlackBand - _rightBlackBand;
	outRect.h = getHeight() - _topBlackBand - _bottomBlackBand;
	_renderer->setOutputRect(&outRect);

}

/**
 * Returns the screen's X scale.
 * @return Scale factor.
 */
double Screen::getXScale() const
{
	return _scaleX;
}

/**
 * Returns the screen's Y scale.
 * @return Scale factor.
 */
double Screen::getYScale() const
{
	return _scaleY;
}

double Screen::getScale() const
{
	return _scale;
}

/**
 * Returns the screen's top black forbidden to cursor band's height.
 * @return Height in pixel.
 */
int Screen::getCursorTopBlackBand() const
{
	return _cursorTopBlackBand;
}

/**
 * Returns the screen's left black forbidden to cursor band's width.
 * @return Width in pixel.
 */
int Screen::getCursorLeftBlackBand() const
{
	return _cursorLeftBlackBand;
}

/**
 * Saves a screenshot of the screen's contents.
 * @param filename Filename of the PNG file.
 */
void Screen::screenshot(const std::string &filename) const
{
	_renderer->screenshot(filename);
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
 * Changes a given scale, and if necessary, switch the current base resolution.
 * @param type the new scale level.
 * @param width reference to which x scale to adjust.
 * @param height reference to which y scale to adjust.
 * @param change should we change the current scale.
 */
void Screen::updateScale(int type, int &width, int &height, bool change)
{
	double pixelRatioY = 1.0;

	if (Options::nonSquarePixelRatio)
	{
		pixelRatioY = 1.2;
	}

	switch (type)
	{
	case SCALE_15X:
		width = Screen::ORIGINAL_WIDTH * 1.5;
		height = Screen::ORIGINAL_HEIGHT * 1.5;
		break;
	case SCALE_2X:
		width = Screen::ORIGINAL_WIDTH * 2;
		height = Screen::ORIGINAL_HEIGHT * 2;
		break;
	case SCALE_SCREEN_DIV_6:
		width = Options::displayWidth / 6.0;
		height = Options::displayHeight / pixelRatioY / 6.0;
		break;
	case SCALE_SCREEN_DIV_5:
		width = Options::displayWidth / 5.0;
		height = Options::displayHeight / pixelRatioY / 5.0;
		break;
	case SCALE_SCREEN_DIV_4:
		width = Options::displayWidth / 4.0;
		height = Options::displayHeight / pixelRatioY / 4.0;
		break;
	case SCALE_SCREEN_DIV_3:
		width = Options::displayWidth / 3.0;
		height = Options::displayHeight / pixelRatioY / 3.0;
		break;
	case SCALE_SCREEN_DIV_2:
		width = Options::displayWidth / 2.0;
		height = Options::displayHeight / pixelRatioY  / 2.0;
		break;
	case SCALE_SCREEN:
		width = Options::displayWidth;
		height = Options::displayHeight / pixelRatioY;
		break;
	case SCALE_ORIGINAL:
	default:
		width = Screen::ORIGINAL_WIDTH;
		height = Screen::ORIGINAL_HEIGHT;
		break;
	}

	// don't go under minimum resolution... it's bad, mmkay?
	width = std::max(width, Screen::ORIGINAL_WIDTH);
	height = std::max(height, Screen::ORIGINAL_HEIGHT);

	if (change && (Options::baseXResolution != width || Options::baseYResolution != height))
	{
		Options::baseXResolution = width;
		Options::baseYResolution = height;
	}
}

SDL_Window * Screen::getWindow() const
{
	return _window;
}
}
