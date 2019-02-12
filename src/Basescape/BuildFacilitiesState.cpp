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
#include <algorithm>
#include "BuildFacilitiesState.h"
#include "../Engine/Game.h"
#include "../Mod/Mod.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextList.h"
#include "../Mod/RuleBaseFacility.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Base.h"
#include "PlaceFacilityState.h"
#include "../Mod/Mod.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the Build Facilities window.
 * @param game Pointer to the core game.
 * @param base Pointer to the base to get info from.
 * @param state Pointer to the base state to refresh.
 */
BuildFacilitiesState::BuildFacilitiesState(Base *base, State *state) : _base(base), _state(state)
{
	_screen = false;

	// Create objects
	_window = new Window(this, 128, 160, 192, 40, POPUP_VERTICAL);
	_btnOk = new TextButton(112, 16, 200, 176);
	_lstFacilities = new TextList(104, 104, 200, 64);
	_txtTitle = new Text(118, 17, 197, 48);

	// Set palette
	setInterface("selectFacility");

	add(_window, "window", "selectFacility");
	add(_btnOk, "button", "selectFacility");
	add(_txtTitle, "text", "selectFacility");
	add(_lstFacilities, "list", "selectFacility");

	// Set up objects
	setWindowBackground(_window, "selectFacility");

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&BuildFacilitiesState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&BuildFacilitiesState::btnOkClick, Options::keyCancel);

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_INSTALLATION"));

	_lstFacilities->setColumns(1, 104);
	_lstFacilities->setSelectable(true);
	_lstFacilities->setBackground(_window);
	_lstFacilities->setMargin(2);
	_lstFacilities->setWordWrap(true);
	_lstFacilities->setScrolling(true, 0);
	_lstFacilities->onMouseClick((ActionHandler)&BuildFacilitiesState::lstFacilitiesClick);

}

/**
 *
 */
BuildFacilitiesState::~BuildFacilitiesState()
{

}

namespace
{
/**
 * Find if two vector ranges intersects.
 * @param a First Vector.
 * @param b Second Vector.
 * @return True if have common value.
 */
bool intersection(const std::vector<std::string> &a, const std::vector<std::string> &b)
{
	auto a_begin = std::begin(a);
	auto a_end = std::end(a);
	auto b_begin = std::begin(b);
	auto b_end = std::end(b);
	while (true)
	{
		if (b_begin == b_end)
		{
			return false;
		}
		a_begin = std::lower_bound(a_begin, a_end, *b_begin);
		if (a_begin == a_end)
		{
			return false;
		}
		if (*a_begin == *b_begin)
		{
			return true;
		}
		b_begin = std::lower_bound(b_begin, b_end, *a_begin);
	}
}

}

/**
 * Populates the build list from the current "available" facilities.
 */
void BuildFacilitiesState::populateBuildList()
{
	_facilities.clear();
	_lstFacilities->clearList();

	std::vector<RuleBaseFacility*> _disabledFacilities;

	const std::vector<std::string> &providedBaseFunc = _base->getProvidedBaseFunc();
	const std::vector<std::string> &forbiddenBaseFunc = _base->getForbiddenBaseFunc();
	const std::vector<std::string> &futureBaseFunc = _base->getFutureBaseFunc();

	const std::vector<std::string> &facilities = _game->getMod()->getBaseFacilitiesList();
	for (std::vector<std::string>::const_iterator i = facilities.begin(); i != facilities.end(); ++i)
	{
		RuleBaseFacility *rule = _game->getMod()->getBaseFacility(*i);
		if (!rule->isAllowedForBaseType(_base->isFakeUnderwater()))
		{
			continue;
		}
		if (rule->isLift() || !_game->getSavedGame()->isResearched(rule->getRequirements()))
		{
			continue;
		}
		if (_base->isMaxAllowedLimitReached(rule))
		{
			_disabledFacilities.push_back(rule);
			continue;
		}
		const std::vector<std::string> &req = rule->getRequireBaseFunc();
		const std::vector<std::string> &forb = rule->getForbiddenBaseFunc();
		const std::vector<std::string> &prov = rule->getProvidedBaseFunc();
		if (!std::includes(providedBaseFunc.begin(), providedBaseFunc.end(), req.begin(), req.end()))
		{
			_disabledFacilities.push_back(rule);
			continue;
		}
		if (intersection(forbiddenBaseFunc, prov))
		{
			_disabledFacilities.push_back(rule);
			continue;
		}
		if (intersection(futureBaseFunc, forb))
		{
			_disabledFacilities.push_back(rule);
			continue;
		}
		_facilities.push_back(rule);
	}

	int row = 0;
	for (std::vector<RuleBaseFacility*>::iterator i = _facilities.begin(); i != _facilities.end(); ++i)
	{
		_lstFacilities->addRow(1, tr((*i)->getType()).c_str());
		++row;
	}

	if (!_disabledFacilities.empty())
	{
		auto disabledColor = _txtTitle->getColor();
		for (std::vector<RuleBaseFacility*>::iterator i = _disabledFacilities.begin(); i != _disabledFacilities.end(); ++i)
		{
			_lstFacilities->addRow(1, tr((*i)->getType()).c_str());
			_lstFacilities->setRowColor(row, disabledColor);
			++row;
		}
	}
}

/**
 * The player can change the selected base
 * or change info on other screens.
 */
void BuildFacilitiesState::init()
{
	_state->init();
	State::init();

	populateBuildList();
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void BuildFacilitiesState::btnOkClick(Action *)
{
	_game->popState();
}

/**
 * Places the selected facility.
 * @param action Pointer to an action.
 */
void BuildFacilitiesState::lstFacilitiesClick(Action *)
{
	auto index = _lstFacilities->getSelectedRow();
	if (index >= _facilities.size())
	{
		return;
	}
	_game->pushState(new PlaceFacilityState(_base, _facilities[index]));
}

}
