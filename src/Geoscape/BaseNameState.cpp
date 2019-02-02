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
#include "BaseNameState.h"
#include "../Engine/Game.h"
#include "../Engine/Action.h"
#include "../Mod/Mod.h"
#include "../Engine/LocalizedText.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextEdit.h"
#include "../Interface/TextButton.h"
#include "../Savegame/Base.h"
#include "../Basescape/PlaceLiftState.h"
#include "../Engine/Options.h"
#include "../Engine/RNG.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in a Base Name window.
 * @param game Pointer to the core game.
 * @param base Pointer to the base to name.
 * @param globe Pointer to the Geoscape globe.
 * @param first Is this the first base in the game?
 * @param fixedLocation Is this the first base in the game on a fixed location?
 */
BaseNameState::BaseNameState(Base *base, Globe *globe, bool first, bool fixedLocation) : _base(base), _globe(globe), _first(first), _fixedLocation(fixedLocation)
{
	_globe->onMouseOver(0);

	_screen = false;

	// Create objects
#ifdef __MOBILE__
	int windowTop = 15;
	int windowLeft = 32;
#else
	int windowTop = 60;
	int windowLeft = 32;
#endif
	_window = new Window(this, 192, 80, windowLeft, windowTop, POPUP_BOTH);
#ifdef __MOBILE__
	if(_first)
	{
		_btnOk = new TextButton(81, 12, windowLeft + 15, windowTop + 58);
	}
	else
	{
		_btnOk = new TextButton(162, 12, windowLeft + 15, windowTop + 58);
	}
	_btnCancel = new TextButton(81, 12, windowLeft + 15 + 81, windowTop + 58);
#else
	_btnOk = new TextButton(162, 12, windowLeft + 15, windowTop + 58);
#endif
	_txtTitle = new Text(182, 17, windowLeft + 5, windowTop + 10);
	_edtName = new TextEdit(this, 127, 16, windowLeft + 27, windowTop + 34);

	// Set palette
	setInterface("baseNaming");

	add(_window, "window", "baseNaming");
	add(_btnOk, "button", "baseNaming");
	add(_txtTitle, "text", "baseNaming");
	add(_edtName, "text", "baseNaming");
#ifdef __MOBILE__
	add(_btnCancel, "text", "baseNaming");
#endif
	centerAllSurfaces();

	// Set up objects
	setWindowBackground(_window, "baseNaming");

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&BaseNameState::btnOkClick);
	//_btnOk->onKeyboardPress((ActionHandler)&BaseNameState::btnOkClick, Options::keyOk);
	_btnOk->onKeyboardPress((ActionHandler)&BaseNameState::btnOkClick, Options::keyCancel);
#ifdef __MOBILE__
	_btnCancel->setText(tr("STR_CANCEL_UC"));
	_btnCancel->onMouseClick((ActionHandler)&BaseNameState::btnCancelClick); /* That _will_ bite me in the ass, won't it? */

	_btnCancel->setVisible(_first);
#endif

	//something must be in the name before it is acceptable
	_btnOk->setVisible(false);

	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();
	_txtTitle->setText(tr("STR_BASE_NAME"));

	if (!_game->getMod()->getBaseNamesFirst().empty())
	{
		std::ostringstream ss;
		int pickFirst = RNG::seedless(0, _game->getMod()->getBaseNamesFirst().size() - 1);
		ss << _game->getMod()->getBaseNamesFirst().at(pickFirst);
		if (!_game->getMod()->getBaseNamesMiddle().empty())
		{
			int pickMiddle = RNG::seedless(0, _game->getMod()->getBaseNamesMiddle().size() - 1);
			ss << " " << _game->getMod()->getBaseNamesMiddle().at(pickMiddle);
		}
		if (!_game->getMod()->getBaseNamesLast().empty())
		{
			int pickLast = RNG::seedless(0, _game->getMod()->getBaseNamesLast().size() - 1);
			ss << " " << _game->getMod()->getBaseNamesLast().at(pickLast);
		}
		_edtName->setText(ss.str());
		_btnOk->setVisible(true);
	}

	_edtName->setBig();
	_edtName->setFocus(true, false);
	_edtName->onChange((ActionHandler)&BaseNameState::edtNameChange);
}

/**
 *
 */
BaseNameState::~BaseNameState()
{

}

/**
 * Updates the base name and disables the OK button
 * if no name is entered.
 * @param action Pointer to an action.
 */
void BaseNameState::edtNameChange(Action *action)
{
	_base->setName(_edtName->getText());
	if (action->getKeycode() == SDLK_RETURN ||
		action->getKeycode() == SDLK_KP_ENTER)
	{
		if (!_edtName->getText().empty())
		{
			btnOkClick(action);
		}
	}
	else
	{
		_btnOk->setVisible(!_edtName->getText().empty());
	}
}

/**
 * Returns to the previous screen
 * @param action Pointer to an action.
 */
void BaseNameState::btnOkClick(Action *)
{
	if (!_edtName->getText().empty())
	{
#ifdef __MOBILE__
		// Hide the keyboard (it won't hide itself)!
		if (SDL_IsScreenKeyboardShown(NULL))
		{
			SDL_StopTextInput();
		}
#endif
		_base->setName(_edtName->getText());
		_game->popState(); // pop BaseNameState

		if (!_fixedLocation)
		{
			_game->popState(); // pop ConfirmNewBaseState or BuildNewBaseState
			if (!_first)
			{
				_game->popState(); // pop BuildNewBaseState
			}
		}

		if (!_first || Options::customInitialBase)
		{
			_game->pushState(new PlaceLiftState(_base, _globe, _first));
		}
	}
}

#ifdef __MOBILE__
/**
 * Hopefully this will pop enough states to get back to the base placing view
 */
void BaseNameState::btnCancelClick(Action *)
{
#ifdef __MOBILE__
	if (SDL_IsScreenKeyboardShown(NULL))
	{
		SDL_StopTextInput();
	}
#endif
	_globe->onMouseOver(0);
	_game->popState();
}
#endif

}
