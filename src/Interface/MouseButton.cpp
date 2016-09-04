/*
 * Copyright 2016 OpenXcom Developers.
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
#include "MouseButton.h"
#include <SDL.h>
#include <SDL_mixer.h>
#include "Text.h"
#include "../Engine/Sound.h"
#include "../Engine/Action.h"
#include "ComboBox.h"
#include "../Engine/LocalizedText.h"

namespace OpenXcom
{

/**
 * Sets up a mouse button selection button with the specified size and position.
 * The text is centered on the button.
 * @param width Width in pixels.
 * @param height Height in pixels.
 * @param x X position in pixels.
 * @param y Y position in pixels.
 */
MouseButton::MouseButton(State *state, int width, int height, int x, int y) : TextButton(width, height, x, y), _mouseButtonNames(), _change(0), _state(state)
{
	_selected = 0;
	_mouseButtonNames.push_back("STR_DISABLED");
	_mouseButtonNames.push_back("STR_LEFT_MOUSE_BUTTON");
	_mouseButtonNames.push_back("STR_MIDDLE_MOUSE_BUTTON");
	_mouseButtonNames.push_back("STR_RIGHT_MOUSE_BUTTON");
	_mouseButtonNames.push_back("STR_UP_MOUSE_WHEEL");
	_mouseButtonNames.push_back("STR_DOWN_MOUSE_WHEEL");
	_mouseButtonNames.push_back("STR_RIGHT_MOUSE_WHEEL");
	_mouseButtonNames.push_back("STR_LEFT_MOUSE_WHEEL");
	_mouseButtonNames.push_back("STR_BACK_MOUSE_BUTTON");
	_mouseButtonNames.push_back("STR_FORWARD_MOUSE_BUTTON");
}

/**
 * Deletes.
 */
MouseButton::~MouseButton()
{
}

/**
 * Gets button's name
 * @param button Button number
 */
std::wstring MouseButton::getButtonName(int button)
{
	unsigned ubutton = static_cast<size_t>(button);

	if (ubutton < _mouseButtonNames.size())
	{
		return _state->tr(_mouseButtonNames[ubutton]);
	}
	else
	{
		return _state->tr("STR_OTHER_MOUSE_BUTTON_").arg(button);
	}
}
/**
 * Sets the button as the pressed button if it's part of a group.
 * @param action Pointer to an action.
 * @param state State that the action handlers belong to.
 */
void MouseButton::mousePress(Action *action, State *state)
{
	int previously = _selected;
	int button = action->getDetails()->button.button;
	// ignore scrollwheel
	if (button == 4 or button == 5)
	{
		return;
	}
	_selected = button;
	// ctrl-click sets to disabled
	if (SDL_GetModState() & KMOD_CTRL)
	{
		_selected = 0;
	}
	if (previously != _selected)
	{
		setText(getButtonName(_selected));
		(state->*_change)(action);
	}
	this->TextButton::mousePress(action, state);
}
/**
 * Gets current selection
 */
int MouseButton::getSelected() const
{
	return _selected;
}

/**
 * Sets current selection
 */
void MouseButton::setSelected(int button)
{
	_selected = button;
	setText(getButtonName(_selected));
}

/**
 * Sets a function to be called every time the slider's value changes.
 * @param handler Action handler.
 */
void MouseButton::onChange(ActionHandler handler)
{
        _change = handler;
}


}
