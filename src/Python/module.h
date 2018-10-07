/*
# Copyright 2017,2018 Stoddard, https://github.com/StoddardOXC.
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

typedef struct _pypy_syspaths_t {
	const char *python_dir;
	const char *cfg_dir;
	const char *user_dir;
	const char *data_dir;
} pypy_syspaths_t;

typedef struct _pypy_dumb_buf_t {
	const char *ptr;
	int32_t size;
} pypy_dumb_buf_t;

/* This and pypy_get_options() are used to inject options definitions into the options screen.
 * All those options' values are at all times being read back via lib.get_option(), so
 * we can skip caring about the std::string or what sizes the bool and SDLKey are, etc.
 *
 * pypy_get_options shall be called after every mod re/load, that is at options screen construction,
 * which is fine.
 *
 * creating the actual array of pypy_option_t-s is handled by C code in lib.optlist_add() or something.
 * and we don't care if it leaks right now.
 *
 * a separate hook, pypy_options_changed(), is called if anything changed.
 */
typedef enum _opt_type {
	PYPY_OPT_TYPE_NONE = 0,			// end of the list
	PYPY_OPT_TYPE_BOOL,
	PYPY_OPT_TYPE_INT,
	PYPY_OPT_TYPE_SDLKEY,
	PYPY_OPT_TYPE_STRING
} opt_type_t;

typedef struct _pypy_option_t {
	opt_type_t type;
	const char *id, *desc, *cat;
	const char *strdef;				// string value default
	int32_t intdef;			 		// bool, int, SDLKey defaults mapped here
	uintptr_t val;
} pypy_option_t;

typedef struct _state_t state_t;

CFFI_DLLEXPORT void pypy_initialize(void *);
CFFI_DLLEXPORT void pypy_set_modpath(const char *);
CFFI_DLLEXPORT void pypy_hook_up(const char *);
CFFI_DLLEXPORT void pypy_reset_mods_statics();
CFFI_DLLEXPORT void pypy_mods_loaded();
CFFI_DLLEXPORT const char *pypy_get_options();
CFFI_DLLEXPORT void pypy_set_options(const char *);

/* ui hooks */
// CFFI_DLLEXPORT void *pypy_spawn_state(const char *state_name);
// CFFI_DLLEXPORT void pypy_forget_state(void *);
// CFFI_DLLEXPORT void pypy_state_input(void *state);
// CFFI_DLLEXPORT void pypy_state_blit(void *state);
// CFFI_DLLEXPORT void pypy_state_think(void *state);
CFFI_DLLEXPORT void pypy_call_method(void *ffihandle, const char *method);

/* non-overriding hooks */
CFFI_DLLEXPORT void pypy_lang_changed();
CFFI_DLLEXPORT void pypy_game_loaded(const char *pypy_data, int32_t data_size);
CFFI_DLLEXPORT void pypy_saving_game(pypy_dumb_buf_t *buf);
CFFI_DLLEXPORT void pypy_game_saved();
CFFI_DLLEXPORT void pypy_game_abandoned();

/* geoscape specific non-overriding */
CFFI_DLLEXPORT void pypy_time_5sec();
CFFI_DLLEXPORT void pypy_time_10min();
CFFI_DLLEXPORT void pypy_time_30min();
CFFI_DLLEXPORT void pypy_time_1hour();
CFFI_DLLEXPORT void pypy_time_1day();
CFFI_DLLEXPORT void pypy_time_1mon();

/* start menu specific. */
CFFI_DLLEXPORT int32_t pypy_mainmenu_keydown(int32_t sym, int32_t mod);

/* geoscape-specific overriding; return value: nonzero if orig code is supposed to run. */
CFFI_DLLEXPORT int32_t pypy_geoscape_keydown(int32_t sym, int32_t mod);
CFFI_DLLEXPORT int32_t pypy_battle_ended();
CFFI_DLLEXPORT int32_t pypy_craft_clicked(void *craft, int32_t button, int32_t kmod);
CFFI_DLLEXPORT int32_t pypy_ufo_clicked(void *ufo, int32_t button, int32_t kmod);
CFFI_DLLEXPORT int32_t pypy_base_clicked(void *base, int32_t button, int32_t kmod);
CFFI_DLLEXPORT int32_t pypy_alienbase_clicked(void *abase, int32_t button, int32_t kmod);
CFFI_DLLEXPORT int32_t pypy_craft_shot_down(void *craft);
CFFI_DLLEXPORT int32_t pypy_craft_at_target(void *craft, void *target);
CFFI_DLLEXPORT int32_t pypy_transfer_done(void *transfer);
CFFI_DLLEXPORT int32_t pypy_research_done(void *research);
CFFI_DLLEXPORT int32_t pypy_manufacture_done(void *manufacture);
/*
CFFI_DLLEXPORT int32_t pypy_(, );
*/

#ifdef __cplusplus
} // extern "C" - this
#endif
