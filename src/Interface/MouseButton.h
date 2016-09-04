/*
 * Copyright 2010-2015 OpenXcom Developers.
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
#ifndef OPENXCOM_MOUSEBUTTON_H
#define OPENXCOM_MOUSEBUTTON_H

#include "../Engine/InteractiveSurface.h"
#include "TextButton.h"

namespace OpenXcom
{

class Text;
class Font;
class Language;
class Sound;
class ComboBox;

/**
 * Coloured button with a text label.
 * Drawn to look like a 3D-shaped box with text on top,
 * responds to mouse clicks. Can be attached to a group of
 * buttons to turn it into a radio button (only one button
 * pushed at a time).
 */
class MouseButton : public TextButton
{
private:
	int _selected;
	std::vector<std::string> _mouseButtonNames;
    ActionHandler _change;
    State *_state;

	std::wstring getButtonName(int button);
public:
	/// Creates a new text button with the specified size and position.
	MouseButton(State *state, int width, int height, int x = 0, int y = 0);
	/// Cleans up the text button.
	~MouseButton();
	/// Handles clicks
	void mousePress(Action *action, State *state);
	/// Pops it back
	bool isButtonHandled(Uint8 button) { return true; }
	/// Gets the selection.
	int getSelected() const;
	/// Sets the selection
	void setSelected(int button);
    /// Hooks an action handler to when the button changes.
    void onChange(ActionHandler handler);
};

}

#endif
