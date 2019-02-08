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
#include "OptionsConfirmState.h"
#include <sstream>
#include <iomanip>
#include "../Engine/Game.h"
#include "../Mod/Mod.h"
#include "../Engine/LocalizedText.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Engine/Options.h"
#include "../Engine/Timer.h"
#include "../Engine/Screen.h"
#include "../Savegame/SavedGame.h"
#include "../Menu/StartState.h"
#include "../Menu/OptionsVideoState.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the Confirm Display Options screen.
 * @param game Pointer to the core game.
 * @param origin Game section that originated this state.
 */
OptionsConfirmState::OptionsConfirmState(OptionsOrigin origin) : _origin(origin), _countdown(15)
{
	_screen = false;

	// Create objects
	_window = new Window(this, 216, 100, 52, 50, POPUP_BOTH);
	_btnYes = new TextButton(50, 20, 70, 120);
	_btnNo = new TextButton(50, 20, 200, 120);
	_txtTitle = new Text(206, 20, 57, 70);
	_txtTimer = new Text(206, 20, 57, 100);
	_timer = new Timer(1000);

	// Set palette
	setInterface("optionsMenu", false, _game->getSavedGame() ? _game->getSavedGame()->getSavedBattle() : 0);

	add(_window, "confirmVideo", "optionsMenu");
	add(_btnYes, "confirmVideo", "optionsMenu");
	add(_btnNo, "confirmVideo", "optionsMenu");
	add(_txtTitle, "confirmVideo", "optionsMenu");
	add(_txtTimer, "confirmVideo", "optionsMenu");

	centerAllSurfaces();

	// Set up objects
	setWindowBackground(_window, "optionsMenu");

	_btnYes->setText(tr("STR_YES"));
	_btnYes->onMouseClick((ActionHandler)&OptionsConfirmState::btnYesClick);

	_btnNo->setText(tr("STR_NO"));
	_btnNo->onMouseClick((ActionHandler)&OptionsConfirmState::btnNoClick);
	// no keyboard shortcuts to make sure users can see the message

	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setWordWrap(true);
	_txtTitle->setText(tr("STR_DISPLAY_OPTIONS_CONFIRM"));

	_txtTimer->setAlign(ALIGN_CENTER);
	_txtTimer->setWordWrap(true);
	_txtTimer->setText(tr("STR_DISPLAY_OPTIONS_REVERT").arg(_countdown));

	if (_origin == OPT_BATTLESCAPE)
	{
		applyBattlescapeTheme();
	}

	_timer->onTimer((StateHandler)&OptionsConfirmState::countdown);
	_timer->start();
}

/**
 *
 */
OptionsConfirmState::~OptionsConfirmState()
{
	delete _timer;
}

/**
 * Runs the countdown timer.
 */
void OptionsConfirmState::think()
{
	State::think();

	_timer->think(this, 0);
}

/**
 * Counts down the timer for reverting display options.
 */
void OptionsConfirmState::countdown()
{
	_countdown--;
	std::ostringstream ss;
	ss << std::setfill('0') << std::setw(2) << _countdown;
	_txtTimer->setText(tr("STR_DISPLAY_OPTIONS_REVERT").arg(ss.str()));
	if (_countdown == 0)
	{
		btnNoClick(0);
	}
}

/**
 * Saves options, goes back to the origin.
 * @param action Pointer to an action.
 */
void OptionsConfirmState::btnYesClick(Action *)
{
	Options::save();
	_game->popState();
	if (Options::reload && _origin == OPT_MENU)
	{
		_game->setState(new StartState);
	}
	else
	{
		OptionsBaseState::restart(_origin);
	}
}

/**
 * Restores the original display options.
 * @param action Pointer to an action.
 */
void OptionsConfirmState::btnNoClick(Action *)
{
	Options::switchDisplay(); 	//  reset video options back to previous set
	_game->getScreen()->resetVideo(); // apply the previous set back
	Options::save();			// save (this should not be needed, but it case something else was changed..)
	_game->popState(); 			// pops this state
	_game->popState(); 			// pops the old options state
	// if mods' options changes require a reload, do it anyway.
	if (Options::reload && _origin == OPT_MENU)
	{
		_game->setState(new StartState);
	}
	else
	{
		_game->pushState(new OptionsVideoState(_origin)); // pushes the new options state
	}
}

}
