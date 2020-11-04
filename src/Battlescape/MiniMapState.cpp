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
#include "MiniMapState.h"
#include "../Engine/Game.h"
#include "../Engine/Screen.h"
#include "../Interface/BattlescapeButton.h"
#include "../Mod/Mod.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Palette.h"
#include "../Interface/Text.h"
#include "MiniMapView.h"
#include "Camera.h"
#include "../Engine/Timer.h"
#include "../Engine/Action.h"
#include "../Engine/Options.h"
#include "../Savegame/SavedBattleGame.h"

namespace OpenXcom
{
/**
 * Initializes all the elements in the MiniMapState screen.
 * @param game Pointer to the core game.
 * @param camera The Battlescape camera.
 * @param battleGame The Battlescape save.
 */
MiniMapState::MiniMapState (Camera * camera, SavedBattleGame * battleGame)
{
	_screenMode = SC_INFOSCREEN;
	_bg = new Surface(320, 200);
	_miniMapView = new MiniMapView(221, 148, 48, 16, _game, camera, battleGame);
	_btnLvlUp = new BattlescapeButton(18, 20, 24, 62);
	_btnLvlDwn = new BattlescapeButton(18, 20, 24, 88);
	_btnOk = new BattlescapeButton(32, 32, 275, 145);
	_txtLevel = new Text(28, 16, 281, 75);

	// Set palette
	battleGame->setPaletteByDepth(this);

	add(_bg);
	_game->getMod()->getSurface("SCANBORD.PCK")->blitNShade(_bg, 0, 0);
	add(_miniMapView);
	add(_btnLvlUp, "buttonUp", "minimap");
	add(_btnLvlDwn, "buttonDown", "minimap");
	add(_btnOk, "buttonOK", "minimap");
	add(_txtLevel, "textLevel", "minimap");

	// FIXME this condition must be made more explicit.
	if (_game->getScreen()->getDY() > 50)
	{
		_screen = false;
		_bg->drawRect(46, 14, 223, 151, Palette::blockOffset(15)+15);
	}

	_btnLvlUp->onMouseClick((ActionHandler)&MiniMapState::btnLevelUpClick);
	_btnLvlDwn->onMouseClick((ActionHandler)&MiniMapState::btnLevelDownClick);
	_btnOk->onMouseClick((ActionHandler)&MiniMapState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&MiniMapState::btnOkClick, Options::keyCancel);
	_btnOk->onKeyboardPress((ActionHandler)&MiniMapState::btnOkClick, Options::keyBattleMap);
	_txtLevel->setBig();
	_txtLevel->setHighContrast(true);
	_txtLevel->setText(tr("STR_LEVEL_SHORT").arg(camera->getViewLevel()));
	_timerAnimate = new Timer(125);
	_timerAnimate->onTimer((StateHandler)&MiniMapState::animate);
	_timerAnimate->start();
	_miniMapView->draw();
}

/**
 *
 */
MiniMapState::~MiniMapState()
{
	delete _timerAnimate;
}

/**
 * Handles mouse-wheeling.
 * @param action Pointer to an action.
 */
void MiniMapState::handle(Action *action)
{
	State::handle(action);
	if (action->getType() == SDL_MOUSEWHEEL)
	{
		if (action->getMouseWheelY() > 0)
		{
			btnLevelUpClick(action);
		}
		else
		{
			btnLevelDownClick(action);
		}
	}
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void MiniMapState::btnOkClick(Action *)
{
	_game->popState();
}

/**
 * Changes the currently displayed minimap level.
 * @param action Pointer to an action.
 */
void MiniMapState::btnLevelUpClick(Action *)
{
	_txtLevel->setText(tr("STR_LEVEL_SHORT").arg(_miniMapView->up()));
}

/**
 * Changes the currently displayed minimap level.
 * @param action Pointer to an action.
 */
void MiniMapState::btnLevelDownClick(Action *)
{
	_txtLevel->setText(tr("STR_LEVEL_SHORT").arg(_miniMapView->down()));
}

/**
 * Animation handler. Updates the minimap view animation.
 */
void MiniMapState::animate()
{
	_miniMapView->animate();
}

/**
 * Handles timers.
 */
void MiniMapState::think()
{
	State::think();
	_timerAnimate->think(this, 0);
}

}
