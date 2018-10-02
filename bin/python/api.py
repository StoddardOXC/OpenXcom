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

ARROW_VERTICAL          = 0
ARROW_HORIZONTAL        = 1

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
    """ Base class for UI states aka windows, a wrapper for state_t from adapter.cpp"""
    def __init__(self, w, h, x, y, ui_id, ui_category):
        """
            parent - parent state.
            w,h,x,y - int
            popmode - WindowPopup
            ui_category, bg - interface stuff, pass "geoscape", "saveMenus" for now
            bg - background image name, like "BACK01.SCR"
         """
        log_info("State({}, {}, {}, {}, '{}', '{}')".format(w, h, x, y, ui_id, ui_category))
        self._st = lib.new_state(w, h, x, y, ui_id.encode('utf-8'), ui_category.encode('utf-8'))
        self._ran_this_frame = False
        self._frame_count = 0
        self._cbuf = []
        self.input = lib.st_get_input_state(self._st)

    @property
    def ptr(self):
        return self._st

    def tr(self, key):
        """ access translations """
        return ffi.string(lib.st_translate(self._st, key.encode('utf-8'))).decode('utf-8')

    def pop(self):
        """ pop itself. after this the c++ object is deleted.  """
        lib.pop_state(self._st)

    def run(self):
        """ this method to be overriden to run the actually useful code """
        pass

    def _inner_input(self):
        # if that's a subsequent run for this frame, drop the previous command buffer
        if self._ran_this_frame:
            self._cbuf = []
        # extract underlying event from the underlying class.
        self.input = lib.st_get_input_state(self._st)
        self.run()
        self._ran_this_frame = True

    # A game frame is processed as follows:
    #
    # - all input events are converted to Action-s and fed to the topmost State only.
    # - topmost State's think() method is called.
    # - all States' blit() method is called.
    #
    # Since our states all first blit to an underlying Surface, which then
    # gets blitted on a blit() call in C++ code, an inactive (not topmost) state
    # only has to not clear that surface.

    def _inner_think(self):
        if not self._ran_this_frame:
            self.run()  # generate command list
            self._ran_this_frame = True

    def _inner_blit(self):
        if self._ran_this_frame:
            self._execute() # execute it
            self._cbuf = [] # drop the buffer
            self._frame_count += 1
            self._ran_this_frame = False # okay, that's the next frame

    # opcodes for the buffer
    FILL = 1        # (cmd (x,y,w,h), color
    BLIT = 2        # (cmd (x,y,w,h), interned surface_handle)
    SOUND = 3  # (cmd name)  one of BUTTON_PRESS; WINDOW_POPUP_{012}

    # rendering command submission
    def blit(self, dst, srcrect, handle, index=0):
        """ blit sprite (sub) image """
        self._cbuf.append((self.BLIT, dst, srcrect, handle, index))

    def fill(self, rect, color):
        self._cbuf.append((self.FILL, rect, color))

    def sound(self, sound):
        self._cbuf.append((self.SOUND, sound))

    def get_surface(self, name, idx = None):
        if idx is None:
            return lib.st_intern_surface(self.ptr, name.encode('utf-8'))
        else:
            return lib.st_intern_sub(self.ptr, name.encode('utf-8'), idx)

    def get_text(self, w, h, text, ha, va, wrap, prim, seco, small):
        return lib.st_intern_text(self.ptr, w, h, text.encode('utf-8'), ha, va, wrap, prim, seco, small)

    def _execute(self):
        """ does the actual blits to the underlying surface """
        # first, clear the surface to be transparent.
        lib.st_clear(self.ptr)
        # execute commands
        for cmd in self._cbuf:
            opcode = cmd[0]

            if opcode == self.FILL:
                x,y,w,h = cmd[1]
                color = cmd[2]
                lib.st_fill(self.ptr, x,y, w, h, color)
            elif opcode == self.BLIT:
                dstx, dsty = cmd[1]
                srcx,srcy, srcw,srch = cmd[2]
                src_handle = cmd[3]
                if src_handle == 0:
                    continue
                lib.st_blit(self.ptr, dstx, dsty, srcx, srcy, srcw, srch, src_handle)
            elif opcode == self.SOUND:
                lib.st_cue_sound(self.ptr, cmd[1].encode('utf-8'))
            else:
                pass

BOX_NAMES = { # TODO: those should be translation keys
    -1: "Stores",
    -2: "Laboratories",
    -3: "Workshop input",
    -4: "Workshop output",
    -5: "On a mission from god",
    -6: "Incoming transfers",
    -7: "Unknown",
    -8: "Up a shit creek without a paddle",
}

class Game(object): # just a namespace.
    @staticmethod
    def tr(state, cstring): # csting mub
        if type(cstring) is bytes:
            return ffi.string(lib.st_translate(state.ptr, cstring)).decode('utf-8')
        elif type(cstring) is str:
            return ffi.string(lib.st_translate(state.ptr, cstring.encode('utf-8'))).decode('utf-8')
        else:
            print(repr(cstring))
            raise Hell

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
                        'handle': f.handle,
                        'ware_cap': f.ware_capacity,
                        'crew_cap': f.crew_capacity,
                        'craft_cap': f.craft_capacity,
                        'jail_cap': f.jail_capacity,
                        'jail_type': f.jail_type,
                        'maintenance': f.maintenance })
            base = {
                    'name': ffi.string(_c_base.name).decode('utf-8'),
                    'idx': _c_base.idx,
                    'handle': _c_base.handle,
                    'facilities': facilities,
                    'inventory_count':  _c_base.inventory_count
            }
            bases.append(base)
            idx += 1
        return bases

    @staticmethod
    def get_base_inventory(state, base_idx, item_cap):
        item_vec = ffi.new("struct _item_t[]", item_cap)
        item_count = lib.get_base_inventory(state.ptr, base_idx, item_vec, item_cap)
        rv = []
        for idx in range(item_count):
            item = item_vec[idx]
            name = ffi.string(item.name).decode("utf-8") if item.name != ffi.NULL else None
            item_id = ffi.string(item.id).decode('utf-8') if item.id != ffi.NULL else None
            rv.append({
                'id': item_id,
                'cats': item.cats,
                'name': name,
                'box_idx': item.box_idx,
                'amount': item.amount,
                'meta': item.meta,
                'handle': item.handle,
                'completes_in': item.completes_in,
                'pedia_unlocked': item.pedia_unlocked,
                'in_processing': item.in_processing,
                'researchable': item.researchable,
                'precursor': item.precursor,
                'autosell': item.autosell,
                'ware_size': item.ware_size,
                'crew_size': item.crew_size,
                'craft_size': item.craft_size,
                'jail_size': item.jail_size,
                'jail_type': item.jail_type,
                'buy_price': item.buy_price,
                'rent_price': item.rent_price,
                'sell_price': item.sell_price,
            })
        return rv
