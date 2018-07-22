/*
# Copyright 2017 Stoddard, https://github.com/StoddardOXC.
 *
 * This file is a part of OpenXcom.
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

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CFFI_DLLEXPORT
#  if defined(_MSC_VER)
#    define CFFI_DLLEXPORT  extern __declspec(dllimport)
#  else
#    define CFFI_DLLEXPORT  extern
#  endif
#endif

typedef struct {
	const char *python_dir;
	const char *cfg_dir;
	const char *user_dir;
	const char *data_dir;
} pypy_syspaths_t;

/* what ons/action listeners do we have :
	onChange                    -- ComboBox, TextEdit, Slider
	onEnter                     -- TextEdit
	onLeftArrowClick            -- TextList ...
	onLeftArrowPress
	onLeftArrowRelease
	onRightArrowClick
	onRightArrowPress
	onRightArrowRelease         -- ... TextList
	onListMouseIn               -- ComboBox ...
	onListMouseOut
	onListMouseOver             -- ... ComboBox

	onKeyboardPress             -- InteractiveSurface (all of them)
	onKeyboardRelease
	onMouseClick
	onMouseIn
	onMouseOut
	onMouseOver
	onMousePress
	onMouseRelease

	onTimer
 */
typedef enum {
	ACTION_OTHER = 0,
	ACTION_KEYBOARDPRESS,
	ACTION_KEYBOARDRELEASE,
	ACTION_MOUSECLICK,
	ACTION_MOUSEIN,
	ACTION_MOUSEOUT,
	ACTION_MOUSEOVER,
	ACTION_MOUSEPRESS,
	ACTION_MOUSERELEASE,
	ACTION_CHANGE,
} action_type_t;

typedef union SDL_Event SDL_Event;

typedef struct _action_t {
	void *element;              // the element that got this action
	void *state;                // the state the element belongs to
	action_type_t atype;        // which event map to look this up in
	int32_t is_mouse;
	int32_t button;
	int32_t kmod;               // SDLMod/SDL_Keymod depending on SDL version.
	int32_t key;                // keysym or keycode depending on SDL version.
} action_t;

typedef struct _state_t state_t;

CFFI_DLLEXPORT int32_t pypy_initialize(pypy_syspaths_t *paths);
CFFI_DLLEXPORT void *pypy_spawn_state(const char *state_name, void *parent_state);
CFFI_DLLEXPORT void pypy_handle_action(action_t *action);

#ifdef __cplusplus
} // extern "C" - this
#endif
