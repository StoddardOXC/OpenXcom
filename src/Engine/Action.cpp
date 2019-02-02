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
Action::Action(SDL_Event *ev, double scaleX, double scaleY, int topBlackBand, int leftBlackBand) : _ev(ev), _scaleX(scaleX), _scaleY(scaleY), _topBlackBand(topBlackBand), _leftBlackBand(leftBlackBand), _mouseX(-1), _mouseY(-1), _surfaceX(-1), _surfaceY(-1), _mouseMotionX(0), _mouseMotionY(0), _sender(0)
{
	if (ev->type == SDL_MOUSEBUTTONUP || ev->type == SDL_MOUSEBUTTONDOWN)
	{
		setMouseAction((ev->button.x - leftBlackBand) / scaleX, (ev->button.y - topBlackBand) / scaleY, 0, 0);
	}
	else if (ev->type == SDL_MOUSEWHEEL)
	{
		// wheel.x and wheel.y is the amount scrolled, not the coordinates... ouch.
		int mouseX, mouseY;
		CrossPlatform::getPointerState(&mouseX, &mouseY);
		setMouseAction((mouseX - leftBlackBand) / scaleX, (mouseY - topBlackBand) / scaleY, 0, 0);
	}
	else if ( ev->type == SDL_MOUSEMOTION )
	{
		setMouseAction((ev->motion.x - leftBlackBand) / scaleX, (ev->motion.y - topBlackBand) / scaleY, 0, 0);
		setMouseMotion(ev->motion.xrel / scaleX, ev->motion.yrel / scaleY);
	}
}

Action::~Action()
{
}

/**
 * Sets this action as a mouse action with
 * the respective mouse properties.
 * @param mouseX Mouse's X position.
 * @param mouseY Mouse's Y position.
 * @param surfaceX Surface's X position.
 * @param surfaceY Surface's Y position.
 */
void Action::setMouseAction(int mouseX, int mouseY, int surfaceX, int surfaceY)
{
	_mouseX = mouseX * _scaleX - _leftBlackBand;
	_mouseY = mouseY * _scaleY - _topBlackBand;
	_surfaceX = surfaceX;
	_surfaceY = surfaceY;
}

/**
 * Gets if the action is a mouse action.
 */
bool Action::isMouseAction() const
{
	return (_mouseX != -1);
}

/**
 * Gets if action is mouse left click.
 */
bool  Action::isMouseLeftClick() const
{
	return isMouseAction() && _ev->button.button == SDL_BUTTON_LEFT;
}

/**
 * Gets if action is mouse right click.
 */
bool  Action::isMouseRightClick() const
{
	return isMouseAction() && _ev->button.button == SDL_BUTTON_RIGHT;
}

/**
 * Returns the absolute X position of the
 * mouse cursor relative to the game window,
 * corrected for screen scaling.
 * @return Mouse's absolute X position.
 */
int Action::getAbsoluteXMouse() const
{
	if (_mouseX == -1)
		return -1;
	return floor(_mouseX / _scaleX);
}

/**
 * Returns the absolute Y position of the
 * mouse cursor relative to the game window,
 * corrected for screen scaling.
 * @return Mouse's absolute X position.
 */
int Action::getAbsoluteYMouse() const
{
	if (_mouseY == -1)
		return -1;
	return floor(_mouseY / _scaleY);
}

/**
 * Returns the relative X position of the
 * mouse cursor relative to the surface that
 * triggered the action, corrected for screen scaling.
 * @return Mouse's relative X position.
 */
int Action::getRelativeXMouse() const
{
	if (_mouseX == -1)
		return -1;
	return floor(_mouseX / _scaleX - _surfaceX);
}

/**
 * Returns the relative X position of the
 * mouse cursor relative to the surface that
 * triggered the action, corrected for screen scaling.
 * @return Mouse's relative X position.
 */
int Action::getRelativeYMouse() const
{
	if (_mouseY == -1)
		return -1;
	return floor(_mouseY / _scaleY - _surfaceY);
}

/**
 * Returns the interactive surface that triggered
 * this action (the sender).
 * @return Pointer to interactive surface.
 */
InteractiveSurface *Action::getSender() const
{
	return _sender;
}

/**
 * Changes the interactive surface that triggered
 * this action (the sender).
 * @param sender Pointer to interactive surface.
 */
void Action::setSender(InteractiveSurface *sender)
{
	_sender = sender;
	_surfaceX = sender->getY();
	_surfaceY = sender->getX();
}

/// Gets the mouse's position delta x (relative motion), Screen coords.
int Action::getXMouseMotion() const
{
	return _mouseMotionX;
}
/// Gets the mouse's position delta y (relative motion), Screen coords.
int Action::getYMouseMotion() const
{
	return _mouseMotionY;
}
/// Sets the mouse motion
void Action::setMouseMotion(int mx, int my)
{
	_mouseMotionX = mx;
	_mouseMotionY = my;
}

/// Gets the action / event type
Uint32 Action::getType() const
{
	return _ev->type;
}
/// Gets mouse button number if that's a click or release
int Action::getMouseButton() const
{
	switch (_ev->type) {
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
			return _ev->button.button;
		case SDL_MOUSEMOTION:
			return _ev->button.button;
		default:
			return 0;
	}
}
/// Gets the key sym if that's a key press or release
SDL_Keycode Action::getKeycode() const
{
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
	return _ev->type == SDL_TEXTINPUT ? _ev->text.text : NULL;
}
/// Gets the wheel motion in y axis
Sint32 Action::getMouseWheelY() const
{
	return _ev->type == SDL_MOUSEWHEEL ? _ev->wheel.y : 0;
}

/// SDL_MULTIGESTURE y
float Action::getMultigestureY() const
{
	return _ev->type == SDL_MULTIGESTURE ? _ev->mgesture.y : 0.0;
}
/// SDL_MULTIGESTURE dDist
float Action::getMultigestureDDist() const
{
	return _ev->type == SDL_MULTIGESTURE ? _ev->mgesture.dDist : 0.0;
}

void Action::setConsumed(void)
{
	_ev->type = SDL_FIRSTEVENT;
}
void Action::setMouseButton(int button)
{
	_ev->button.button = button;
}

}
