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

#include "Action.h"
#include "CrossPlatform.h"
#include "InteractiveSurface.h"
#include "Exception.h"

namespace OpenXcom
{

/**
 * Creates a new action.
 * @param scaleX Screen's X scaling factor.
 * @param scaleY Screen's Y scaling factor.
 * @param topBlackBand Screen's top black band height.
 * @param leftBlackBand Screen's left black band width.
 * @param ev Pointer to SDL_event.
 */
Action::Action(const SDL_Event *ev, double scaleX, double scaleY, int topBlackBand, int leftBlackBand) : _ev(ev),
	_scaleX(scaleX), _scaleY(scaleY), _topBlackBand(topBlackBand), _leftBlackBand(leftBlackBand),
	_mouseX(-1), _mouseY(-1), _surfaceX(-1), _surfaceY(-1), _mouseRelX(0), _mouseRelY(0),
	_mouseButton(0),  _sender(0)
{
	if (_ev == NULL)
		throw Exception("bad stuff in action handling") ;

	if (ev->type == SDL_MOUSEBUTTONUP || ev->type == SDL_MOUSEBUTTONDOWN)
	{
		_mouseX = (ev->button.x - leftBlackBand) / scaleX;
		_mouseY = (ev->button.y - topBlackBand) / scaleY;
		_mouseButton = ev->button.button;
	}
	else if ( ev->type == SDL_MOUSEMOTION )
	{
		_mouseX = (ev->motion.x - leftBlackBand) / scaleX; // where it ended up?
		_mouseY = (ev->motion.y - topBlackBand) / scaleY;  // or where it started from?
		_mouseRelX = ev->motion.xrel / scaleX;
		_mouseRelY = ev->motion.yrel / scaleY;
		_mouseButton = ev->motion.state;
	}
	else
	{	// for all the rest of events, this is _sometimes_ useful
		int mouseX, mouseY;
		_mouseButton = CrossPlatform::getPointerState(&mouseX, &mouseY);
		_mouseX = (mouseX - leftBlackBand) / scaleX;
		_mouseY = (mouseY - topBlackBand) / scaleY;
	}
	if (_mouseButton < 0 ) { throw(Exception("_mouseButton < 0!")); }
}

Action::~Action()
{
}

/**
 * Sets mouse position if this action was in any way related to mouse
 */
void Action::setMousePosition(int mouseX, int mouseY)
{
	if (_ev == NULL)
		throw(Exception("bad stuff in action handling"));
	switch(_ev->type) {
		case SDL_MOUSEMOTION:
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
		case SDL_MOUSEWHEEL:
			_mouseX = mouseX;
			_mouseY = mouseY;
			break;
		default:
			throw(Exception("WTF: setting mouse posn on non-mouse event"));
			break;
	}
}

/**
 * Gets if the action is a mouse action.
 */
bool Action::isMouseAction() const
{
	if (_ev == NULL)
		throw(Exception("bad stuff in action handling"));
	return _mouseX != -1;
}

/**
 * Gets if action is mouse left click.
 */
bool  Action::isMouseLeftClick() const
{
	return isMouseAction() && _mouseButton == SDL_BUTTON_LEFT;
}

/**
 * Gets if action is mouse right click.
 */
bool  Action::isMouseRightClick() const
{
	return isMouseAction() && _mouseButton == SDL_BUTTON_RIGHT;
}

/**
 * Returns the absolute X position of the
 * mouse cursor relative to the game window,
 * corrected for screen scaling.
 * @return Mouse's absolute X position.
 */
int Action::getAbsoluteXMouse() const
{
	return getMouseX();
}

/**
 * Returns the X position of the mouse cursor
 * in logical screen coordinates
 * @return Mouse's absolute X position.
 */
int Action::getMouseX() const
{
	if (_ev == NULL)
		throw(Exception("bad stuff in action handling"));
	return _mouseX;
}

/**
 * Returns the absolute Y position of the
 * mouse cursor relative to the game window,
 * corrected for screen scaling.
 * @return Mouse's absolute X position.
 */
int Action::getAbsoluteYMouse() const
{
	return getMouseY();
}

/**
 * Returns the Y position of the mouse cursor
 * in logical screen coordinates
 * @return Mouse's absolute X position.
 */
int Action::getMouseY() const
{
	if (_ev == NULL)
		throw(Exception("bad stuff in action handling"));
	return _mouseY;
}

/**
 * Returns the relative X position of the
 * mouse cursor relative to the surface that
 * triggered the action, corrected for screen scaling.
 * @return Mouse's relative X position.
 */
int Action::getRelativeXMouse() const
{
	if (_ev == NULL)
		throw(Exception("bad stuff in action handling"));
	return _mouseX - _surfaceX;
}

/**
 * Returns the relative X position of the
 * mouse cursor relative to the surface that
 * triggered the action, corrected for screen scaling.
 * @return Mouse's relative X position.
 */
int Action::getRelativeYMouse() const
{
	if (_ev == NULL)
		throw(Exception("bad stuff in action handling"));
	return _mouseY - _surfaceY;
}

/**
 * Returns the interactive surface that triggered
 * this action (the sender).
 * @return Pointer to interactive surface.
 */
InteractiveSurface *Action::getSender() const
{
	if (_ev == NULL)
		throw(Exception("bad stuff in action handling"));
	return _sender;
}

/**
 * Changes the interactive surface that triggered
 * this action (the sender).
 * @param sender Pointer to interactive surface.
 */
void Action::setSender(InteractiveSurface *sender)
{
	if (_ev == NULL)
		throw(Exception("bad stuff in action handling"));
	_sender = sender;
	_surfaceX = sender->getX();
	_surfaceY = sender->getY();
}

/// Gets the mouse's position delta x (relative motion), Screen coords.
int Action::getXMouseMotion() const
{
	if (_ev == NULL)
		throw(Exception("bad stuff in action handling"));
	return _mouseRelX;
}
/// Gets the mouse's position delta y (relative motion), Screen coords.
int Action::getYMouseMotion() const
{
	if (_ev == NULL)
		throw(Exception("bad stuff in action handling"));
	return _mouseRelY;
}
/// Sets the mouse motion
void Action::setMouseMotion(int mx, int my)
{
	if (_ev == NULL)
		throw(Exception("bad stuff in action handling"));
	_mouseRelX = mx;
	_mouseRelY = my;
}

/// Gets the action / event type
Uint32 Action::getType() const
{
	if (_ev == NULL) {
		return SDL_FIRSTEVENT;
	} else {
		return _ev->type;
	}
}
/// Gets mouse button number if that's a click or release
int Action::getMouseButton() const
{
	if (_ev == NULL)
		throw(Exception("bad stuff in action handling"));
	return _mouseButton;
}
/// Gets the key sym if that's a key press or release
SDL_Keycode Action::getKeycode() const
{
	if (_ev == NULL)
		throw(Exception("bad stuff in action handling"));
	switch (_ev->type) {
		case SDL_KEYDOWN:
		case SDL_KEYUP:
			return _ev->key.keysym.sym;
		default:
			return SDLK_UNKNOWN;
	}
}

/// Gets the keys mods status
Uint16 Action::getKeymods() const
{
	if (_ev == NULL)
		throw(Exception("bad stuff in action handling"));
	switch (_ev->type) {
		case SDL_KEYDOWN:
		case SDL_KEYUP:
			return _ev->key.keysym.mod;
		default:
			return 0;
	}
}
/// Gets the key sym if that's a key press or release
SDL_Scancode Action::getScancode() const
{
	if (_ev == NULL)
		throw(Exception("bad stuff in action handling"));
	switch (_ev->type) {
		case SDL_KEYDOWN:
		case SDL_KEYUP:
			return _ev->key.keysym.scancode;
		default:
			return SDL_SCANCODE_UNKNOWN;
	}
}
const char *Action::getText() const
{
	if (_ev == NULL)
		throw(Exception("bad stuff in action handling"));
	return _ev->type == SDL_TEXTINPUT ? _ev->text.text : NULL;
}
/// Gets the wheel motion in y axis
Sint32 Action::getMouseWheelY() const
{
	if (_ev == NULL)
		throw(Exception("bad stuff in action handling"));
	return _ev->type == SDL_MOUSEWHEEL ? _ev->wheel.y : 0;
}

/// SDL_MULTIGESTURE y
float Action::getMultigestureY() const
{
	if (_ev == NULL)
		throw(Exception("bad stuff in action handling"));
	return _ev->type == SDL_MULTIGESTURE ? _ev->mgesture.y : 0.0;
}
/// SDL_MULTIGESTURE dDist
float Action::getMultigestureDDist() const
{
	if (_ev == NULL)
		throw(Exception("bad stuff in action handling"));
	return _ev->type == SDL_MULTIGESTURE ? _ev->mgesture.dDist : 0.0;
}

void Action::setConsumed(void)
{
	_ev = NULL;
	CrossPlatform::stackTrace (NULL);
}

bool Action::isConsumed() const
{
	return _ev == NULL;
}

}
