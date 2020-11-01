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
#include "OptionsVideoState.h"
#include "../Engine/Language.h"
#include "../Interface/TextButton.h"
#include "../Engine/Action.h"
#include "../Interface/Text.h"
#include "../Interface/TextEdit.h"
#include "../Interface/ToggleTextButton.h"
#include "../Engine/Options.h"
#include "../Engine/Screen.h"
#include "../Interface/ArrowButton.h"
#include "../Engine/FileMap.h"
#include "../Engine/Logger.h"
#include "../Interface/ComboBox.h"
#include "../Engine/Game.h"
#include "SetWindowedRootState.h"
#include "../Engine/SDLRenderer.h"
#include "../Engine/GL2Renderer.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the Video Options screen.
 * @param game Pointer to the core game.
 * @param origin Game section that originated this state.
 */
OptionsVideoState::OptionsVideoState(OptionsOrigin origin) : OptionsBaseState(origin), _drivers(), _filtersSDL(), _filtersGL2()
{
	setCategory(_btnVideo);

	// Create objects
	_displaySurface = new InteractiveSurface(110, 32, 94, 18);

	// left column
	int cy = 8;
	_txtDisplayResolution = new Text(114, 9, 94, cy);
	_txtDisplayWidth = new TextEdit(this, 40, 17, 94, cy + 12);
	_txtDisplayX = new Text(16, 17, 132, cy + 12);
	_txtDisplayHeight = new TextEdit(this, 40, 17, 144, cy + 12);
	_btnDisplayResolutionUp = new ArrowButton(ARROW_BIG_UP, 14, 14, 186, cy += 3 );
	_btnDisplayResolutionDown = new ArrowButton(ARROW_BIG_DOWN, 14, 14, 186, cy += 14);

	cy = 23;
	_txtLanguage = new Text(114, 9, 94, cy += 14);
	_cbxLanguage = new ComboBox(this, 104, 16, 94, cy += 9 + 1);

	_txtGeoScale = new Text(114, 9, 94, cy += 16 + 3);
	_cbxGeoScale = new ComboBox(this, 104, 16, 94, cy += 9 + 1);

	_txtBattleScale = new Text(114, 9, 94, cy += 16 + 3);
	_cbxBattleScale = new ComboBox(this, 104, 16, 94, cy += 9 + 1);

	_cbxLetterbox = new ComboBox(this, 104, 16, 94, cy += 16 + 2 + 5);

	// right column
	cy = 8;
	_txtMode = new Text(114, 9, 206, cy);
	_cbxDisplayMode = new ComboBox(this, 104, 16, 206, cy += 9 + 1);

	_txtDriver = new Text(114, 9, 206, cy += 16 + 3);
	_cbxDriver = new ComboBox(this, 104, 16, 206, cy += 9 + 1);

	_txtFilter = new Text(114, 9, 206, cy += 16 + 3);
	_cbxFilter = new ComboBox(this, 104, 16, 206, cy += 9 + 1);

	_txtOptions = new Text(114, 9, 206, cy += 16 + 3);
	_btnLockMouse = new ToggleTextButton(104, 16, 206, cy += 9 + 1);
	_btnRootWindowedMode = new ToggleTextButton(104, 16, 206, cy += 16 + 1);

	/* TODO: add current mode */
	/* Get available fullscreen modes */
	_res.resize(SDL_GetNumDisplayModes(0));
	for (size_t i = 0; i < _res.size(); ++i)
		SDL_GetDisplayMode(0, i, &_res[i]);
	_resCurrent = -1;
	if (_res.size() != 0)
	{
#if 0
		int i;
		_resCurrent = -1;
		for (i = 0; _res[i]; ++i)
		{
			if (_resCurrent == -1 &&
				((_res[i]->w == Options::displayWidth && _res[i]->h <= Options::displayHeight) || _res[i]->w < Options::displayWidth))
			{
				_resCurrent = i;
			}
		}
		_res.size() = i;
#endif
	}
	else
	{
		_btnDisplayResolutionDown->setVisible(false);
		_btnDisplayResolutionUp->setVisible(false);
		Log(LOG_WARNING) << "Couldn't get display resolutions";
	}

	add(_displaySurface);
	add(_txtDisplayResolution, "text", "videoMenu");
	add(_txtDisplayWidth, "resolution", "videoMenu");
	add(_txtDisplayX, "resolution", "videoMenu");
	add(_txtDisplayHeight, "resolution", "videoMenu");
	add(_btnDisplayResolutionUp, "button", "videoMenu");
	add(_btnDisplayResolutionDown, "button", "videoMenu");

	add(_txtLanguage, "text", "videoMenu");
	add(_txtDriver, "text", "videoMenu");
	add(_txtFilter, "text", "videoMenu");

	add(_txtMode, "text", "videoMenu");

	add(_txtOptions, "text", "videoMenu");
	add(_cbxLetterbox, "button", "videoMenu");
	add(_btnLockMouse, "button", "videoMenu");
	add(_btnRootWindowedMode, "button", "videoMenu");

	add(_cbxFilter, "button", "videoMenu");
	add(_cbxDriver, "button", "videoMenu");

	add(_cbxDisplayMode, "button", "videoMenu");

	add(_txtBattleScale, "text", "videoMenu");
	add(_cbxBattleScale, "button", "videoMenu");

	add(_txtGeoScale, "text", "videoMenu");
	add(_cbxGeoScale, "button", "videoMenu");

	add(_cbxLanguage, "button", "videoMenu");

	// Set up objects
	_txtDisplayResolution->setText(tr("STR_DISPLAY_RESOLUTION"));

	_displaySurface->setTooltip("STR_DISPLAY_RESOLUTION_DESC");
	_displaySurface->onMouseIn((ActionHandler)&OptionsVideoState::txtTooltipIn);
	_displaySurface->onMouseOut((ActionHandler)&OptionsVideoState::txtTooltipOut);

	_txtDisplayWidth->setAlign(ALIGN_CENTER);
	_txtDisplayWidth->setBig();
	_txtDisplayWidth->setConstraint(TEC_NUMERIC_POSITIVE);
	_txtDisplayWidth->onChange((ActionHandler)&OptionsVideoState::txtDisplayWidthChange);

	_txtDisplayX->setAlign(ALIGN_CENTER);
	_txtDisplayX->setBig();
	_txtDisplayX->setText("x");

	_txtDisplayHeight->setAlign(ALIGN_CENTER);
	_txtDisplayHeight->setBig();
	_txtDisplayHeight->setConstraint(TEC_NUMERIC_POSITIVE);
	_txtDisplayHeight->onChange((ActionHandler)&OptionsVideoState::txtDisplayHeightChange);

	std::ostringstream ssW, ssH;
	ssW << Options::displayWidth;
	ssH << Options::displayHeight;
	_txtDisplayWidth->setText(ssW.str());
	_txtDisplayHeight->setText(ssH.str());

	_btnDisplayResolutionUp->onMouseClick((ActionHandler)&OptionsVideoState::btnDisplayResolutionUpClick);
	_btnDisplayResolutionDown->onMouseClick((ActionHandler)&OptionsVideoState::btnDisplayResolutionDownClick);

	_txtMode->setText(tr("STR_DISPLAY_MODE"));

	_txtOptions->setText(tr("STR_DISPLAY_OPTIONS"));

	std::vector<std::string> boxingChoices = {
		tr("STR_LETTERBOXED_1_20"),
		tr("STR_LETTERBOXED_1_00"),
		tr("STR_LETTERBOXED_NONE")
	};
	_cbxLetterbox->setOptions(boxingChoices);
	if (Options::keepAspectRatio) {
		if (Options::nonSquarePixelRatio) {
			_cbxLetterbox->setSelected(0);
		} else {
			_cbxLetterbox->setSelected(1);
		}
	} else {
		_cbxLetterbox->setSelected(2);
	}
	_cbxLetterbox->setTooltip("STR_LETTERBOXED_DESC");
	_cbxLetterbox->onChange((ActionHandler)&OptionsVideoState::cbxLetterboxChange);
	_cbxLetterbox->onMouseIn((ActionHandler)&OptionsVideoState::txtTooltipIn);
	_cbxLetterbox->onMouseOut((ActionHandler)&OptionsVideoState::txtTooltipOut);

	_btnLockMouse->setText(tr("STR_LOCK_MOUSE"));
	_btnLockMouse->setPressed(Options::captureMouse);
	_btnLockMouse->onMouseClick((ActionHandler)&OptionsVideoState::btnLockMouseClick);
	_btnLockMouse->setTooltip("STR_LOCK_MOUSE_DESC");
	_btnLockMouse->onMouseIn((ActionHandler)&OptionsVideoState::txtTooltipIn);
	_btnLockMouse->onMouseOut((ActionHandler)&OptionsVideoState::txtTooltipOut);

	_btnRootWindowedMode->setText(tr("STR_FIXED_WINDOW_POSITION"));
	_btnRootWindowedMode->setPressed(Options::rootWindowedMode);
	_btnRootWindowedMode->onMouseClick((ActionHandler)&OptionsVideoState::btnRootWindowedModeClick);
	_btnRootWindowedMode->setTooltip("STR_FIXED_WINDOW_POSITION_DESC");
	_btnRootWindowedMode->onMouseIn((ActionHandler)&OptionsVideoState::txtTooltipIn);
	_btnRootWindowedMode->onMouseOut((ActionHandler)&OptionsVideoState::txtTooltipOut);

	_txtLanguage->setText(tr("STR_DISPLAY_LANGUAGE"));

	std::vector<std::string> names;
	Language::getList(_langs, names);
	_cbxLanguage->setOptions(names);
	for (size_t i = 0; i < names.size(); ++i)
	{
		if (_langs[i] == Options::language)
		{
			_cbxLanguage->setSelected(i);
			break;
		}
	}
	_cbxLanguage->onChange((ActionHandler)&OptionsVideoState::cbxLanguageChange);
	_cbxLanguage->setTooltip("STR_DISPLAY_LANGUAGE_DESC");
	_cbxLanguage->onMouseIn((ActionHandler)&OptionsVideoState::txtTooltipIn);
	_cbxLanguage->onMouseOut((ActionHandler)&OptionsVideoState::txtTooltipOut);

	// first comes the driver.
	_txtDriver->setText(tr("STR_DISPLAY_DRIVER"));
	// TODO: this all to be reworked when the GL3 renderer gets written.
	// And we assume the order doesn't change between invocations,
	// but for SDL2 filters it's always true.
	_drivers = SDLRenderer::listDrivers();
	for (const auto& gl2driver: GL2Renderer::listDrivers()) {
		_drivers.push_back(gl2driver);
	}
	_cbxDriver->setOptions(_drivers);
	int i = 0;
	for (const auto& driverName: _drivers) {
		if (driverName == Options::renderDriver) {
			_cbxDriver->setSelected(i);
		}
		i += 1;
	}
	_cbxDriver->onChange((ActionHandler)&OptionsVideoState::cbxDriverChange);
	_cbxDriver->setTooltip("STR_DISPLAY_DRIVER_DESC");
	_cbxDriver->onMouseIn((ActionHandler)&OptionsVideoState::txtTooltipIn);
	_cbxDriver->onMouseOut((ActionHandler)&OptionsVideoState::txtTooltipOut);

	// then the filter
	_txtFilter->setText(tr("STR_DISPLAY_FILTER"));
	_filtersSDL = SDLRenderer::listFilters();
	_filtersGL2 = GL2Renderer::listFilters();

	updateDisplayFilterCbx();
	_cbxFilter->onChange((ActionHandler)&OptionsVideoState::cbxFilterChange);
	_cbxFilter->setTooltip("STR_DISPLAY_FILTER_DESC");
	_cbxFilter->onMouseIn((ActionHandler)&OptionsVideoState::txtTooltipIn);
	_cbxFilter->onMouseOut((ActionHandler)&OptionsVideoState::txtTooltipOut);

	std::vector<std::string> displayModes;
	displayModes.push_back(tr("STR_WINDOWED"));
	displayModes.push_back(tr("STR_FULLSCREEN"));
	displayModes.push_back(tr("STR_BORDERLESS"));
	displayModes.push_back(tr("STR_RESIZABLE"));

	int displayMode = 0;
	if (Options::fullscreen)
	{
		displayMode = 1;
	}
	else if (Options::borderless)
	{
		displayMode = 2;
	}
	else if (Options::allowResize)
	{
		displayMode = 3;
	}

	_cbxDisplayMode->setOptions(displayModes);
	_cbxDisplayMode->setSelected(displayMode);
	_cbxDisplayMode->onChange((ActionHandler)&OptionsVideoState::updateDisplayMode);
	_cbxDisplayMode->setTooltip("STR_DISPLAY_MODE_DESC");
	_cbxDisplayMode->onMouseIn((ActionHandler)&OptionsVideoState::txtTooltipIn);
	_cbxDisplayMode->onMouseOut((ActionHandler)&OptionsVideoState::txtTooltipOut);

	_txtGeoScale->setText(tr("STR_GEOSCAPE_SCALE"));

	std::vector<std::string> scales;
	scales.push_back(tr("STR_ORIGINAL"));
	scales.push_back(tr("STR_1_5X"));
	scales.push_back(tr("STR_2X"));
	scales.push_back(tr("STR_THIRD_DISPLAY"));
	scales.push_back(tr("STR_HALF_DISPLAY"));
	scales.push_back(tr("STR_FULL_DISPLAY"));
	scales.push_back(tr("STR_FOURTH_DISPLAY"));
	scales.push_back(tr("STR_FIFTH_DISPLAY"));
	scales.push_back(tr("STR_SIXTH_DISPLAY"));

	_cbxGeoScale->setOptions(scales);
	_cbxGeoScale->setSelected(Options::geoscapeScale);
	_cbxGeoScale->onChange((ActionHandler)&OptionsVideoState::updateGeoscapeScale);
	_cbxGeoScale->setTooltip("STR_GEOSCAPESCALE_SCALE_DESC");
	_cbxGeoScale->onMouseIn((ActionHandler)&OptionsVideoState::txtTooltipIn);
	_cbxGeoScale->onMouseOut((ActionHandler)&OptionsVideoState::txtTooltipOut);

	_txtBattleScale->setText(tr("STR_BATTLESCAPE_SCALE"));

	_cbxBattleScale->setOptions(scales);
	_cbxBattleScale->setSelected(Options::battlescapeScale);
	_cbxBattleScale->onChange((ActionHandler)&OptionsVideoState::updateBattlescapeScale);
	_cbxBattleScale->setTooltip("STR_BATTLESCAPE_SCALE_DESC");
	_cbxBattleScale->onMouseIn((ActionHandler)&OptionsVideoState::txtTooltipIn);
	_cbxBattleScale->onMouseOut((ActionHandler)&OptionsVideoState::txtTooltipOut);

}

/**
 *
 */
OptionsVideoState::~OptionsVideoState()
{

}

/**
 * What it says on the tin
 */
void OptionsVideoState::updateDisplayFilterCbx()
{
	size_t i;
	Log(LOG_INFO)<< "updateDisplayFilterCbx() : " << _drivers[_cbxDriver->getSelected()];
	if (_drivers[_cbxDriver->getSelected()] == "GL2") {
		_cbxFilter->setOptions(_filtersGL2);
		i = 0;
		for (const auto& filterName: _filtersGL2) {
			if (filterName == Options::renderFilterGL2) {
				_cbxFilter->setSelected(i);
			}
			i += 1;
		}
	} else {
		_cbxFilter->setOptions(_filtersSDL);
		i = 0;
		for (const auto& filterName: _filtersSDL) {
			if (filterName == Options::renderFilterSDL) {
				_cbxFilter->setSelected(i);
			}
			i += 1;
		}
	}
}


/**
 * Uppercases all the words in a string.
 * @param str Source string.
 * @return Destination string.
 */
std::string OptionsVideoState::ucWords(std::string str)
{
	for (size_t i = 0; i < str.length(); ++i)
	{
		if (i == 0)
			str[0] = toupper(str[0]);
		else if (str[i] == ' ' || str[i] == '-' || str[i] == '_')
		{
			str[i] = ' ';
			if (str.length() > i + 1)
				str[i + 1] = toupper(str[i + 1]);
		}
	}
	return str;
}

/**
 * Selects a bigger display resolution.
 * @param action Pointer to an action.
 */
void OptionsVideoState::btnDisplayResolutionUpClick(Action *)
{
	if (_res.size() == 0)
		return;
	if (_resCurrent <= 0)
	{
		_resCurrent = _res.size()-1;
	}
	else
	{
		_resCurrent--;
	}
	updateDisplayResolution();
}

/**
 * Selects a smaller display resolution.
 * @param action Pointer to an action.
 */
void OptionsVideoState::btnDisplayResolutionDownClick(Action *)
{
	if (_res.size() == 0)
		return;
	if ( (size_t)(_resCurrent + 1) >= _res.size())
	{
		_resCurrent = 0;
	}
	else
	{
		_resCurrent++;
	}
	updateDisplayResolution();
}

/**
 * Updates the display resolution based on the selection.
 */
void OptionsVideoState::updateDisplayResolution()
{
	std::ostringstream ssW, ssH;
	ssW << _res[_resCurrent].w;
	ssH << _res[_resCurrent].h;
	_txtDisplayWidth->setText(ssW.str());
	_txtDisplayHeight->setText(ssH.str());

	Options::newDisplayWidth = _res[_resCurrent].w;
	Options::newDisplayHeight = _res[_resCurrent].h;
}

/**
 * Changes the Display Width option.
 * @param action Pointer to an action.
 */
void OptionsVideoState::txtDisplayWidthChange(Action *)
{
	std::stringstream ss;
	int width = 0;
	ss << std::dec << _txtDisplayWidth->getText();
	ss >> std::dec >> width;
	Options::newDisplayWidth = width;
}

/**
 * Changes the Display Height option.
 * @param action Pointer to an action.
 */
void OptionsVideoState::txtDisplayHeightChange(Action *)
{
	std::stringstream ss;
	int height = 0;
	ss << std::dec << _txtDisplayHeight->getText();
	ss >> std::dec >> height;
	Options::newDisplayHeight = height;
	// Update resolution mode
#if 0
	if (_res.size() > 0)
	{
		int i;
		_resCurrent = -1;
		for (std::vector<SDL_DisplayMode>::iterator i = _res.begin(); i != res.end(); ++i)
		{
			if (_resCurrent == -1 &&
				((_res[i]->w == Options::newDisplayWidth && _res[i]->h <= Options::newDisplayHeight) || _res[i]->w < Options::newDisplayWidth))
			{
				_resCurrent = i;
			}
		}
	}
#endif
}

/**
 * Changes the Language option.
 * @param action Pointer to an action.
 */
void OptionsVideoState::cbxLanguageChange(Action *)
{
	Options::language = _langs[_cbxLanguage->getSelected()];
}

/**
 * Changes the Driver options.
 * @param action Pointer to an action.
 */
void OptionsVideoState::cbxDriverChange(Action *)
{
	Log(LOG_INFO) << ":cbxDriverChange() -> " << _drivers[_cbxDriver->getSelected()];
	Options::newRenderDriver = _drivers[_cbxDriver->getSelected()];
	updateDisplayFilterCbx();
}

/**
 * Changes the Filter options.
 * @param action Pointer to an action.
 */
void OptionsVideoState::cbxFilterChange(Action *)
{
	if (_drivers[_cbxDriver->getSelected()] == "GL2") {

		Options::newRenderFilterGL2 = _filtersGL2[_cbxFilter->getSelected()];
		Log(LOG_INFO) << ":cbxFilterChange() -> GL2 " << _filtersGL2[_cbxDriver->getSelected()];
	} else {
		Options::newRenderFilterSDL = _filtersSDL[_cbxFilter->getSelected()];
		Log(LOG_INFO) << ":cbxFilterChange() -> SDL " << _filtersSDL[_cbxDriver->getSelected()];
	}
}

/**
 * Changes the Display Mode options.
 * @param action Pointer to an action.
 */
void OptionsVideoState::updateDisplayMode(Action *)
{
	switch(_cbxDisplayMode->getSelected())
	{
	case 0:
		Options::newFullscreen = false;
		Options::newBorderless = false;
		Options::newAllowResize = false;
		break;
	case 1:
		Options::newFullscreen = true;
		Options::newBorderless = false;
		Options::newAllowResize = false;
		break;
	case 2:
		Options::newFullscreen = false;
		Options::newBorderless = true;
		Options::newAllowResize = false;
		break;
	case 3:
		Options::newFullscreen = false;
		Options::newBorderless = false;
		Options::newAllowResize = true;
		break;
	default:
		break;
	}
}

/**
 * Changes the Letterboxing option.
 * @param action Pointer to an action.
 */
void OptionsVideoState::cbxLetterboxChange(Action *)
{
	switch(_cbxLetterbox->getSelected()) {
	case 0:
		Options::newKeepAspectRatio = true;
		Options::newNonSquarePixelRatio = true;
		break;
	case 1:
		Options::newKeepAspectRatio = true;
		Options::newNonSquarePixelRatio = false;
		break;
	case 2:
		Options::newKeepAspectRatio = false;
		Options::newNonSquarePixelRatio = false;
		break;
	default:
		break;
	}
}

/**
 * Changes the Lock Mouse option.
 * @param action Pointer to an action.
 */
void OptionsVideoState::btnLockMouseClick(Action *)
{
	// Don't do that! Breaks stuff hard.
	// FIXME what is supposed to be going on here?
	Options::captureMouse = _btnLockMouse->getPressed();
	//SDL_SetRelativeMouseMode((Options::captureMouse)?SDL_TRUE:SDL_FALSE); //because a typecast is not enough
}

/**
 * Ask user where he wants to root screen.
 * @param action Pointer to an action.
 */
void OptionsVideoState::btnRootWindowedModeClick(Action *)
{
	if (_btnRootWindowedMode->getPressed())
	{
		_game->pushState(new SetWindowedRootState(_origin, this));
	}
	else
	{
		Options::newRootWindowedMode = false;
	}
}

/**
 * Changes the geoscape scale.
 * @param action Pointer to an action.
 */
void OptionsVideoState::updateGeoscapeScale(Action *)
{
	Options::newGeoscapeScale = _cbxGeoScale->getSelected();
}

/**
 * Updates the Battlescape scale.
 * @param action Pointer to an action.
 */
void OptionsVideoState::updateBattlescapeScale(Action *)
{
	Options::newBattlescapeScale = _cbxBattleScale->getSelected();
}

/**
 * Updates the scale.
 * @param dX delta of X;
 * @param dY delta of Y;
 */
void OptionsVideoState::resize(const int dW, const int dH)
{
	OptionsBaseState::resize(dW, dH);
	std::ostringstream ss;
	ss << Options::displayWidth;
	_txtDisplayWidth->setText(ss.str());
	ss.str("");
	ss << Options::displayHeight;
	_txtDisplayHeight->setText(ss.str());
}

/**
 * Takes care of any events from the core game engine.
 * @param action Pointer to an action.
 */
void OptionsVideoState::handle(Action *action)
{
	State::handle(action);
	if (action->getKeycode() == SDLK_g && (SDL_GetModState() & KMOD_CTRL) != 0)
	{
		_btnLockMouse->setPressed(Options::captureMouse);
	}

}

/**
 * Unpresses Fixed Borderless Pos button
 */
void OptionsVideoState::unpressRootWindowedMode()
{
	_btnRootWindowedMode->setPressed(false);
}

}
