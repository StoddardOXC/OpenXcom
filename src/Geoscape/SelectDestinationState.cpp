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
#include "SelectDestinationState.h"
#include <cmath>
#include "../Engine/Game.h"
#include "../Engine/Screen.h"
#include "../Engine/Action.h"
#include "../Mod/Mod.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Surface.h"
#include "../Interface/Window.h"
#include "Globe.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Savegame/Waypoint.h"
#include "MultipleTargetsState.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Craft.h"
#include "../Mod/RuleCraft.h"
#include "ConfirmCydoniaState.h"
#include "../Engine/Options.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the Select Destination window.
 * @param game Pointer to the core game.
 * @param craft Pointer to the craft to target.
 * @param globe Pointer to the Geoscape globe.
 */
SelectDestinationState::SelectDestinationState(Craft *craft, Globe *globe) : _craft(craft), _globe(globe)
{
	_screen = false;

	// Create objects
	_btnRotateLeft = new InteractiveSurface(12, 10);
	_btnRotateRight = new InteractiveSurface(12, 10);
	_btnRotateUp = new InteractiveSurface(13, 12);
	_btnRotateDown = new InteractiveSurface(13, 12);
	_btnZoomIn = new InteractiveSurface(23, 23);
	_btnZoomOut = new InteractiveSurface(13, 17);

	_window = new Window(this, 256, 28);
	_btnCancel = new TextButton(60, 12);
	_btnCydonia = new TextButton(60, 12);
	_txtTitle = new Text(100, 16);

	// Set palette
	setInterface("geoscape");

	add(_btnRotateLeft);
	add(_btnRotateRight);
	add(_btnRotateUp);
	add(_btnRotateDown);
	add(_btnZoomIn);
	add(_btnZoomOut);

	add(_window, "genericWindow", "geoscape");
	add(_btnCancel, "genericButton1", "geoscape");
	add(_btnCydonia, "genericButton1", "geoscape");
	add(_txtTitle, "genericText", "geoscape");

	// Set up objects
	_globe->onMouseClick((ActionHandler)&SelectDestinationState::globeClick);

	_btnRotateLeft->onMousePress((ActionHandler)&SelectDestinationState::btnRotateLeftPress);
	_btnRotateLeft->onMouseRelease((ActionHandler)&SelectDestinationState::btnRotateLeftRelease);
	_btnRotateLeft->onKeyboardPress((ActionHandler)&SelectDestinationState::btnRotateLeftPress, Options::keyGeoLeft);
	_btnRotateLeft->onKeyboardRelease((ActionHandler)&SelectDestinationState::btnRotateLeftRelease, Options::keyGeoLeft);

	_btnRotateRight->onMousePress((ActionHandler)&SelectDestinationState::btnRotateRightPress);
	_btnRotateRight->onMouseRelease((ActionHandler)&SelectDestinationState::btnRotateRightRelease);
	_btnRotateRight->onKeyboardPress((ActionHandler)&SelectDestinationState::btnRotateRightPress, Options::keyGeoRight);
	_btnRotateRight->onKeyboardRelease((ActionHandler)&SelectDestinationState::btnRotateRightRelease, Options::keyGeoRight);

	_btnRotateUp->onMousePress((ActionHandler)&SelectDestinationState::btnRotateUpPress);
	_btnRotateUp->onMouseRelease((ActionHandler)&SelectDestinationState::btnRotateUpRelease);
	_btnRotateUp->onKeyboardPress((ActionHandler)&SelectDestinationState::btnRotateUpPress, Options::keyGeoUp);
	_btnRotateUp->onKeyboardRelease((ActionHandler)&SelectDestinationState::btnRotateUpRelease, Options::keyGeoUp);

	_btnRotateDown->onMousePress((ActionHandler)&SelectDestinationState::btnRotateDownPress);
	_btnRotateDown->onMouseRelease((ActionHandler)&SelectDestinationState::btnRotateDownRelease);
	_btnRotateDown->onKeyboardPress((ActionHandler)&SelectDestinationState::btnRotateDownPress, Options::keyGeoDown);
	_btnRotateDown->onKeyboardRelease((ActionHandler)&SelectDestinationState::btnRotateDownRelease, Options::keyGeoDown);

	_btnZoomIn->onMouseClick((ActionHandler)&SelectDestinationState::btnZoomInLeftClick, SDL_BUTTON_LEFT);
	_btnZoomIn->onMouseClick((ActionHandler)&SelectDestinationState::btnZoomInRightClick, SDL_BUTTON_RIGHT);
	_btnZoomIn->onKeyboardPress((ActionHandler)&SelectDestinationState::btnZoomInLeftClick, Options::keyGeoZoomIn);

	_btnZoomOut->onMouseClick((ActionHandler)&SelectDestinationState::btnZoomOutLeftClick, SDL_BUTTON_LEFT);
	_btnZoomOut->onMouseClick((ActionHandler)&SelectDestinationState::btnZoomOutRightClick, SDL_BUTTON_RIGHT);
	_btnZoomOut->onKeyboardPress((ActionHandler)&SelectDestinationState::btnZoomOutLeftClick, Options::keyGeoZoomOut);

	// dirty hacks to get the rotate buttons to work in "classic" style
	_btnRotateLeft->setListButton();
	_btnRotateRight->setListButton();
	_btnRotateUp->setListButton();
	_btnRotateDown->setListButton();

	setWindowBackground(_window, "geoscape");

	_btnCancel->setText(tr("STR_CANCEL_UC"));
	_btnCancel->onMouseClick((ActionHandler)&SelectDestinationState::btnCancelClick);
	_btnCancel->onKeyboardPress((ActionHandler)&SelectDestinationState::btnCancelClick, Options::keyCancel);

	_txtTitle->setText(tr("STR_SELECT_DESTINATION"));
	_txtTitle->setVerticalAlign(ALIGN_MIDDLE);
	_txtTitle->setWordWrap(true);

	if (_craft->getFuelPercentage() < 100 || !_craft->getRules()->getSpacecraft() || !_game->getSavedGame()->isResearched(_game->getMod()->getFinalResearch()))
	{
		_btnCydonia->setVisible(false);
	}
	else
	{
		_btnCydonia->setText(tr("STR_CYDONIA"));
		_btnCydonia->onMouseClick((ActionHandler)&SelectDestinationState::btnCydoniaClick);
	}

	if (_craft->getStatus() != "STR_OUT")
	{
		_globe->setCraftRange(_craft->getLongitude(), _craft->getLatitude(), _craft->getBaseRange());
		_globe->invalidate();
	}

	resize();
}

/**
 *
 */
SelectDestinationState::~SelectDestinationState()
{
	_globe->setCraftRange(0.0, 0.0, 0.0);
}

/**
 * Stop the globe movement.
 */
void SelectDestinationState::init()
{
	State::init();
	_globe->rotateStop();
}

/**
 * Runs the globe rotation timer.
 */
void SelectDestinationState::think()
{
	State::think();
	_globe->think();
}

/**
 * Handles the globe.
 * @param action Pointer to an action.
 */
void SelectDestinationState::handle(Action *action)
{
	State::handle(action);
	_globe->handle(action, this);
}

/**
 * Processes any left-clicks for picking a target,
 * or right-clicks to scroll the globe.
 * @param action Pointer to an action.
 */
void SelectDestinationState::globeClick(Action *action)
{
	double lon, lat;
	int mouseX = (int)floor(action->getMouseX()), mouseY = (int)floor(action->getMouseY());
	_globe->cartToPolar(mouseX, mouseY, &lon, &lat);

	// Ignore window clicks
	if (mouseY < 28)
	{
		return;
	}

	// Clicking on a valid target
	if (action->getMouseButton() == SDL_BUTTON_LEFT)
	{
		std::vector<Target*> v = _globe->getTargets(mouseX, mouseY, true, _craft);
		if (v.empty())
		{
			Waypoint *w = new Waypoint();
			w->setLongitude(lon);
			w->setLatitude(lat);
			v.push_back(w);
		}
		_game->pushState(new MultipleTargetsState(v, _craft, 0));
	}
}

/**
 * Starts rotating the globe to the left.
 * @param action Pointer to an action.
 */
void SelectDestinationState::btnRotateLeftPress(Action *)
{
	_globe->rotateLeft();
}

/**
 * Stops rotating the globe to the left.
 * @param action Pointer to an action.
 */
void SelectDestinationState::btnRotateLeftRelease(Action *)
{
	_globe->rotateStopLon();
}

/**
 * Starts rotating the globe to the right.
 * @param action Pointer to an action.
 */
void SelectDestinationState::btnRotateRightPress(Action *)
{
	_globe->rotateRight();
}

/**
 * Stops rotating the globe to the right.
 * @param action Pointer to an action.
 */
void SelectDestinationState::btnRotateRightRelease(Action *)
{
	_globe->rotateStopLon();
}

/**
 * Starts rotating the globe upwards.
 * @param action Pointer to an action.
 */
void SelectDestinationState::btnRotateUpPress(Action *)
{
	_globe->rotateUp();
}

/**
 * Stops rotating the globe upwards.
 * @param action Pointer to an action.
 */
void SelectDestinationState::btnRotateUpRelease(Action *)
{
	_globe->rotateStopLat();
}

/**
 * Starts rotating the globe downwards.
 * @param action Pointer to an action.
 */
void SelectDestinationState::btnRotateDownPress(Action *)
{
	_globe->rotateDown();
}

/**
 * Stops rotating the globe downwards.
 * @param action Pointer to an action.
 */
void SelectDestinationState::btnRotateDownRelease(Action *)
{
	_globe->rotateStopLat();
}

/**
 * Zooms into the globe.
 * @param action Pointer to an action.
 */
void SelectDestinationState::btnZoomInLeftClick(Action *)
{
	_globe->zoomIn();
}

/**
 * Zooms the globe maximum.
 * @param action Pointer to an action.
 */
void SelectDestinationState::btnZoomInRightClick(Action *)
{
	_globe->zoomMax();
}

/**
 * Zooms out of the globe.
 * @param action Pointer to an action.
 */
void SelectDestinationState::btnZoomOutLeftClick(Action *)
{
	_globe->zoomOut();
}

/**
 * Zooms the globe minimum.
 * @param action Pointer to an action.
 */
void SelectDestinationState::btnZoomOutRightClick(Action *)
{
	_globe->zoomMin();
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void SelectDestinationState::btnCancelClick(Action *)
{
	_game->popState();
}

void SelectDestinationState::btnCydoniaClick(Action *)
{
	if (_craft->getNumSoldiers() > 0 || _craft->getNumVehicles() > 0)
	{
		_game->pushState(new ConfirmCydoniaState(_craft));
	}
}

/**
 * Updates the scale.
 * @param dX delta of X;
 * @param dY delta of Y;
 */
void SelectDestinationState::resize()
{
	const int dx = _game->getScreen()->getDX();
	const int dy = _game->getScreen()->getDY();

	_btnRotateLeft->setX(259 + dx * 2); 	_btnRotateLeft->setY(176 + dy);
	_btnRotateRight->setX(283 + dx * 2);	_btnRotateRight->setY(176 + dy);
	_btnRotateUp->setX(271 + dx * 2); 		_btnRotateUp->setY(162 + dy);
	_btnRotateDown->setX(271 + dx * 2);		_btnRotateDown->setY(187 + dy);
	_btnZoomIn->setX(295 + dx * 2); 		_btnZoomIn->setY(156 + dy);
	_btnZoomOut->setX(300 + dx * 2); 		_btnZoomOut->setY(182 + dy);

	_window->setX(dx);						_window->setY(0);
	_btnCancel->setX(110 + dx);				_btnCancel->setY(8);
	_btnCydonia->setX(180 + dx);			_btnCydonia->setY(8);
	_txtTitle->setX(10 + dx);				_txtTitle->setY(6);
}

}
