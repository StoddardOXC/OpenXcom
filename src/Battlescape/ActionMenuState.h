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
 * along with OpenXcom.  If not, see <http:///www.gnu.org/licenses/>.
 */
#include "../Engine/State.h"
#include "BattlescapeGame.h"

namespace OpenXcom
{

class ActionMenuItem;

/**
 * Window that allows the player
 * to select a battlescape action.
 */
class ActionMenuState : public State
{
protected:
	BattleAction *_action;
	ActionMenuItem *_actionMenu[6];
	/// Adds a new menu item for an action.
	void addItem(BattleActionType ba, const std::string &name, int *id, SDL_Keycode key);
	/// Acts on the action instance that has been chosen and set.
	void handleAction();
	/// Actually builds the menu; overriden in SkillMenuAction
	virtual void buildMenu();
#ifdef __MOBILE__
	InteractiveSurface *_outside;
#endif
public:
	/// Creates the Action Menu state.
	ActionMenuState(BattleAction *action, const int upshift);
	/// Cleans up the Action Menu state.
	~ActionMenuState();
	/// Init function.
	void init() override;
	/// Handler for right-clicking anything.
	void handle(Action *action) override;
	/// Handler for clicking a action menu item.
	virtual void btnActionMenuItemClick(Action *action);
	/// Update the resolution settings, we just resized the window. FIXME: drop that obsolete shit.
	void resize(const int dW, const int dH) override;
#ifdef __MOBILE__
	/// Pop the state in case of clicking.
	void outsideClick(Action *action);
#endif
};

}
