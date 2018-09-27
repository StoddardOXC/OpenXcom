#
# Copyright 2017 Stoddard, https://github.com/StoddardOXC.
#
# This file is a part of OpenXcom.
#
# OpenXcom is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# OpenXcom is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with OpenXcom.  If not, see <http://www.gnu.org/licenses/>.
#

# api.py - next layer upon ffi api
# a bit close to the C++ ui framework - src/Interface/, src/Menu
#

from pypycom import ffi, lib

# enum WindowPopup { ... }
POPUP_NONE       = 0
POPUP_HORIZONTAL = 1
POPUP_VERTICAL   = 2
POPUP_BOTH       = 3

# enum TextHAlign { ALIGN_LEFT, ALIGN_CENTER, ALIGN_RIGHT };
ALIGN_LEFT       = 0
ALIGN_CENTER     = 1
ALIGN_RIGHT      = 2

# enum TextVAlign { ALIGN_TOP, ALIGN_MIDDLE, ALIGN_BOTTOM };
ALIGN_TOP        = 0
ALIGN_MIDDLE     = 1
ALIGN_BOTTOM     = 2

# action_type_t (enum)
ACTION_OTHER            = 0
ACTION_KEYBOARDPRESS    = 1
ACTION_KEYBOARDRELEASE  = 2
ACTION_MOUSECLICK       = 3
ACTION_MOUSEIN          = 4
ACTION_MOUSEOUT         = 5
ACTION_MOUSEOVER        = 6
ACTION_MOUSEPRESS       = 7
ACTION_MOUSERELEASE     = 8
ACTION_CHANGE           = 9


# TODO: don't repeat this here; somehow import from the embedding_init_code.
LOG_FATAL   = 0
LOG_ERROR   = 1
LOG_WARNING = 2
LOG_INFO    = 3
LOG_DEBUG   = 4
LOG_VERBOSE = 5
def _log_encode(*args):
    t = ''.join(str(a) for a in args)
    t = t.encode('utf-8')
    return t
def log_fatal(*args):
    lib.logg(LOG_FATAL, _log_encode(*args))
def log_error(*args):
    lib.logg(LOG_ERROR, _log_encode(*args))
def log_warning(*args):
    lib.logg(LOG_WARNING, _log_encode(*args))
def log_info(*args):
    lib.logg(LOG_INFO, _log_encode(*args))
def log_debug(*args):
    lib.logg(LOG_DEBUG, _log_encode(*args))
def log_verbose(*args):
    lib.logg(LOG_VERBOSE, _log_encode(*args))

def ptr2int(ptr):
    return int(ffi.cast("uintptr_t", ptr))

class State(object):
    """ The derived classes must have a constructor of the form:
            def __init__(self, parent): and no more parameters there.
        This constructor is only for calling up from them.
        See aboutpy.py """
    def __init__(self, parent, w, h, x, y, popmode, ui_category, bg):
        """
            parent - parent state.
            w,h,x,y - int
            popmode - WindowPopup
            ui_category, bg - interface stuff, pass "geoscape", "saveMenus" for now
            bg - background image name, like "BACK01.SCR"
         """

        # hmm. not so sure about this. and if it's even needed
        # this should be expilict, if anything
        # OTOH, digging up .ptr every time doesn't look right anyway
        if isinstance(parent, State):
            self._parent_st = parent.ptr
        else: # elif cffi.whatever_type
            self._parent_st = parent

        self._st = lib.new_state(parent, w, h, x, y, popmode, ui_category.encode('utf-8'), bg.encode('utf-8'))
        self.elements = {} # keyed by the ptr of the C++ object.

    @property
    def ptr(self):
        return self._st

    def tr(self, key):
        """ access translations """
        return ffi.string(lib.st_translate(self._st, key.encode('utf-8'))).decode('utf-8')

    def pop(self):
        """ pop itself """
        lib.pop_state(self._st)

    def add(self, element):
        element_id = int(ffi.cast("uintptr_t", element.ptr))
        if element_id in self.elements:
            log_error("py::State::add(): adding an element twice: {!r}".format(element))
        else:
            self.elements[element_id] = element

    def handle(self, action):
        element_id = ptr2int(action.element)
        if element_id in self.elements:
            log_info("py::State::handle() got action {!r}".format(action))
            self.elements[element_id].handle(action)
        else:
            log_info("py::State::handle() skipping action action for element {!r} {!r}".format(action.element, action))

    def add_text(self, w, h, x, y, ui_element, ui_category, halign, valign, do_wrap, is_big, text):
        lib.st_add_text(self.ptr, w, h, x, y, ui_element.encode('utf-8'), ui_category.encode('utf-8'), halign, valign, do_wrap, is_big, text.encode('utf-8'))

class InteractiveSurface(object):
    def __init__(self):
        self._handlers = {}
        self._ptr = ffi.NULL

    @property
    def ptr(self):
        return self._ptr

    def onMouseClick(self, handler, button):
        lib.btn_set_click_handler(self.ptr, button)
        akey = (ACTION_MOUSECLICK, button)
        self._handlers[akey] = handler

    def onKeyboardPress(self, handler, keysym = None):
        if keysym is None:
            lib.btn_set_anykey_handler(self.ptr)
        else:
            lib.btn_set_keypress_handler(self.ptr, keysym)
        akey = (ACTION_KEYBOARDPRESS, keysym)
        self._handlers[akey] = handler

    def handle(self, action):
        if action.atype in (ACTION_KEYBOARDPRESS, ACTION_KEYBOARDRELEASE):
            akey = (action.atype, action.key)
            if akey not in self._handlers:
                akey = (action.atype, None)
        elif action.atype  in (ACTION_MOUSECLICK, ACTION_MOUSEPRESS, ACTION_MOUSERELEASE):
            akey = (action.atype, action.button)
        elif action.atype  in (ACTION_MOUSEIN, ACTION_MOUSEOUT, ACTION_MOUSEOVER):
            akey = (action.atype, "mousemove")
        elif action.atype in (ACTION_CHANGE, ACTION_OTHER):
            akey = (action.atype,)
        if akey in self._handlers:
            self._handlers[akey](action)

class TextButton(InteractiveSurface):
    def __init__(self, parent, w, h, x, y, ui_element, ui_category, text):
        super(TextButton, self).__init__()
        self._ptr = lib.st_add_text_button(parent.ptr, int(w), int(h), int(x), int(y), ui_element.encode('utf-8'),
                                           ui_category.encode('utf-8'), text.encode('utf-8'))

    def set_high_contrast(self):
        lib.btn_set_high_contrast(self.ptr)

class TextList(InteractiveSurface):
    def __init__(self, parent, w, h, x, y, ui_element, ui_category):
        super(TextList, self).__init__()
        self._ptr = lib.st_add_text_list(parent.ptr, int(w), int(h), int(x), int(y), ui_element.encode('utf-8'),
                                           ui_category.encode('utf-8'))

    def add_row(self, *args):
        bargs = list(ffi.new("wchar_t[]", str(a)) for a in args)
        lib.textlist_add_row(self.ptr, len(bargs), *bargs)

    def set_columns(self, *args):
        bargs = list(ffi.cast("int32_t", a) for a in args)
        lib.textlist_set_columns(self.ptr, len(bargs), *bargs)

    def set_selectable(self, flag):
        pass

    def set_background(self, what):
        pass

    def set_margin(self, margin):
        pass

    def set_arrow_column(self, ac):
        pass

class Game(object): # just a namespace.
    @staticmethod
    def tr(state, cstring):
        return ffi.string(lib.st_translate(state.ptr, cstring)).decode('utf-8')

    @staticmethod
    def get_bases(state):
        FAC_CAP = 6*6
        bases = []
        idx = 0
        _c_base = ffi.new("struct _base_t *");
        _c_facs = ffi.new("struct _facility_t[]", FAC_CAP)
        while True:
            rv = lib.get_base_facilities(state.ptr, idx, _c_base, _c_facs, FAC_CAP)
            if rv <= 0:
                if idx == 0:  # no bases : game might not be loaded
                    return None
                break
            facilities = []
            for i in range(rv):
                f = _c_facs[i]
                facilities.append({
                        'type': Game.tr(state, ffi.string(f.id)),
                        'online': f.online,
                        'ware_cap': f.ware_capacity,
                        'crew_cap': f.crew_capacity,
                        'craft_cap': f.craft_capacity,
                        'jail_cap': f.jail_capacity,
                        'jail_type': f.jail_type,
                        'maintenance': f.maintenance })
            base = { 'name': ffi.string(_c_base.name), 'idx': _c_base.idx, 'facilities': facilities }
            bases.append(base)
            idx += 1
        return bases
