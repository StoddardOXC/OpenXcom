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
#include <stdint.h>

/* here goes the api that's exposed via cffi to something */

#ifdef __cplusplus
extern "C" {
#else
typedef uint8_t bool;
#endif

typedef struct _textbutton_t textbutton_t;
typedef struct _text_t text_t;
typedef struct _state_t state_t;

state_t *new_state(state_t *parent, int32_t w, int32_t h, int32_t x, int32_t y,
					int32_t anim_type, const char *ui_category,
					const char *background);

void push_state(state_t *state);
void pop_state(state_t *state);

textbutton_t *st_add_text_button(state_t *st, int32_t w, int32_t h, int32_t x, int32_t y,
					const char *ui_element, const char *ui_category, const char *text_utf8);

void btn_set_high_contrast(textbutton_t *btn, bool high_contrast);
void btn_set_text_utf8(textbutton_t *btn, const char *text);
void btn_set_click_handler(textbutton_t *btn, int button);

/* keyname is a string corresponding to an OptionsInfo's _id for a keybinding */
void btn_set_keypress_handler(textbutton_t *btn, const char *keyname);
/* just do something on the AnyKey pressed. */
void btn_set_anykey_handler(textbutton_t *btn);

/* just a static text. doesn't need any handlers or other crap */
void st_add_text(state_t *st, int32_t w, int32_t h, int32_t x, int32_t y,
				 const char *ui_element, const char *ui_category,
				 int32_t halign, int32_t valign,
				 bool do_wrap, bool is_big, const char *text_utf8);

const char *st_translate(state_t *st, const char *key);

/* log levels:
 *    LOG_FATAL = 0
 *    LOG_ERROR
 *    LOG_WARNING
 *    LOG_INFO
 *    LOG_DEBUG
 *    LOG_VERBOSE
 * message must be utf8
 */
void logg(int level, const char *message);

#ifdef __cplusplus
} // extern "C" - this comment is required due to pypy/cffi build system
#endif
