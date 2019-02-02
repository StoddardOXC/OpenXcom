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

namespace OpenXcom
{

class InteractiveSurface;

/**
 * Container for all the information associated with a
 * given user action, like mouse clicks, key presses, etc.
 * @note Called action because event is reserved.
 */
class Action
{
private:
	SDL_Event *_ev;
	double _scaleX, _scaleY;
	int _topBlackBand, _leftBlackBand, _mouseX, _mouseY, _surfaceX, _surfaceY;
	int _mouseMotionX, _mouseMotionY;
	InteractiveSurface *_sender;
public:
	/// Creates an action with given event data.
	Action(SDL_Event *ev, double scaleX, double scaleY, int topBlackBand, int leftBlackBand);
	/// Cleans up the action.
	~Action();
	/// Sets the action as a mouse action.
	void setMouseAction(int mouseX, int mouseY, int surfaceX, int surfaceY);
	/// Gets if the action is a mouse action.
	bool isMouseAction() const;
	/// Gets if action is mouse left click.
	bool isMouseLeftClick() const;
	/// Gets if action is mouse right click.
	bool isMouseRightClick() const;
	/// Gets the mouse's absolute X position, Screen coords.
	int getAbsoluteXMouse() const;
	/// Gets the mouse's absolute Y position, Screen coords.
	int getAbsoluteYMouse() const;
	/// Gets the mouse's relative X position, Screen coords. (relative to some surface)
	int getRelativeXMouse() const;
	/// Gets the mouse's relative Y position, Screen coords.
	int getRelativeYMouse() const;
	/// Gets the mouse's position delta x (relative motion), Screen coords.
	int getXMouseMotion() const;
	/// Gets the mouse's position delta y (relative motion), Screen coords.
	int getYMouseMotion() const;
	/// Sets the mouse relative motion (xrel, yrel)
	void setMouseMotion(int, int);
	/// Gets the sender of the action.
	InteractiveSurface *getSender() const;
	/// Sets the sender of the action.
	void setSender(InteractiveSurface *sender);
	/// Gets the action / event type
	Uint32 getType() const;
	/// Gets mouse button number if that's a click or release
	int getMouseButton() const;
	/// Gets the key sym if that's a key press or release
	SDL_Keycode getKeycode() const;
	/// Gets the keys mods status
	Uint16 getKeymods() const;
	/// Gets the scancode
	SDL_Scancode getScancode() const;
	/// Gets the text of text input event
	const char *getText() const;
	/// Gets the wheel motion in y axis
	Sint32 getMouseWheelY() const;
	/// SDL_MULTIGESTURE y
	float getMultigestureY() const;
	/// SDL_MULTIGESTURE dDist
	float getMultigestureDDist() const;
	/// Marks action / event as consumed.
	void setConsumed(void);
	/// Sets as if we just pressed a button. This stuff in InteractiveSurface needs to be FIXME-ed
	void setMouseButton(int button);

};

}
