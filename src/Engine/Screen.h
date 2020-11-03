	#pragma once
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
#include <SDL.h>
#include <string>
#include <vector>
#include "Surface.h"

namespace OpenXcom
{

class Surface;
class Action;

class Renderer;

enum ScreenMode {
	SC_NONE = 0,
	SC_INHERITED,
	SC_ORIGINAL,
	SC_STARTSTATE,
	SC_MAINMENU,
	SC_INTRO,
	SC_GEOSCAPE,
	SC_BATTLESCAPE,
	SC_INFOSCREEN
};

/**
 * A display screen, handles rendering onto the game window.
 *
 * Responsible for all scaling calculations and setup.
 *
 * - owns the SDL_Window, does all its setup
 * - contains the screen SDL_Surface that most everything else draws on
 * - submits the surface to the Renderer, which is responsible
 *   for filtering, scaling and showing it to the user.
 * - all Actions are created from SDL_Events here
 */
class Screen
{
private:
	//SDL_Surface *_screen;
	SDL_Window *_window;
	Renderer *_renderer;
	unsigned _screenTexture, _cursorTexture, _fpsTexture;
	int _baseWidth, _baseHeight;
	double _scaleX, _scaleY;
	int _topBlackBand, _leftBlackBand;
	SDL_Color deferredPalette[256];
	int _numColors, _firstColor;
	bool _pushPalette;
	Surface::UniqueBufferPtr _buffer;
	Surface::UniqueSurfacePtr _surface;
	std::string _screenshotFilename;
	int _currentScaleType;
	ScreenMode _currentScaleMode;
	bool _resizeAccountedFor;
	std::string _title;

public:
	static const int ORIGINAL_WIDTH;
	static const int ORIGINAL_HEIGHT;

	/// Creates a new display screen.
	Screen(const std::string& title);
	/// Cleans up the display screen.
	~Screen();
	/// Get horizontal offset to the top-left of a centered 320x200 rectangle.
	int getDX() const;
	/// Get vertical offset to the top-left of a centered 320x200 rectangle.
	int getDY() const;
	/// Gets the internal buffer.
	SDL_Surface *getSurface();
	/// Handles keyboard events.
	void handle(Action *action);
	/// Sets up a blit of a surface at screen coordinates given by it (cursor, fps)
	void blit(Surface *surface);
	/// Renders the screen onto the game window.
	void flip();
	/// Clears the screen.
	void clear();
	/// sets the screen mode - battlescape, geoscape, startscreen, bigmenu, intro, etc..
	void setMode(ScreenMode mode);
	/// this is the new real updateScale - to be called on window resize / renderer recreation only.
	void updateScale();
	/// Sets the screen's 8bpp palette.
	void setPalette(const SDL_Color *colors, int firstcolor = 0, int ncolors = 256, bool immediately = false);
	/// Gets the screen's 8bpp palette.
	const SDL_Color *getPalette() const;
	/// Gets the screen's width, logical pixels.
	int getWidth() const;
	/// Gets the screen's height, logical pixels.
	int getHeight() const;
	/// Actually resets the video stuff.
	void resetVideo();
	/// Takes a screenshot.
	void screenshot(const std::string &filename);
	/// Get the pointer for our current window
	SDL_Window *getWindow() const;
	/// Make an Action
	std::unique_ptr<Action> makeAction(const SDL_Event *ev) const;
	/// Warp the mouse (absolute Screen coordinates)
	void warpMouse(int x, int y);
	/// Warp the mouse (Screen coordinates delta)
	void warpMouseRelative(int dx, int dy);
	/// Set the window grabbing status
	void setWindowGrab(int grab);
	/// Draw the cursor and the fpsCounter
	void drawCursorAndFPS(Surface  *cursor, Surface *fpsCounter);
};

}
