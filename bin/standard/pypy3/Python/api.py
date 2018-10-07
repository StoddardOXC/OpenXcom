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

__states__ = []
__hooks__ = ['mods_loaded']

import pprint
from ruamel.yaml import YAML
from pypycom import ffi, lib

# have to duplicate this here... TODO: use the logging module instead, find how to call stuff from the embedding_init.py.
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
def log_exception(exc_type, exc_value, exc_traceback):
    for l in traceback.format_exception(exc_type, exc_value, exc_traceback):
        log_error("PyPy: {}".format(l)[:-1])
    log_fatal("PyPy code threw an exception: fatal.")

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

class IRULEMENT(object):
    x, y, w, h = None, None, None, None
    color, color2 = None, None
    border, TFTDMode = None, None
    def __init__(self, pod):
        for pname, val in pod.items():
            if pname.startswith('_'):
                log_error("InterfaceRule element member '{}' ignored.".format(pname))
                continue
            if pname == 'pos':
                self.x, self.y = val
            elif pname == 'size':
                self.w, self.h = val
            else:
                setattr(self, pname, val)
        self.rect = (self.x, self.y, self.w, self.h)

class IRULE(object):
    _prohibited = ('name', 'parent', 'backgroundImage', 'music', 'sound')
    def __init__(self, name, pod):
        """ the palette IRULE property and palette IRULE element:
        1. palette propery is renamed to palname

        2. State::setInterface() (which _IS_ called):

             - first, pal is gotten from the property. It is a string, like "PAL_GEOSCAPE" (which is the default for not-battlescape btw)
             - if we haven't got "palette" element here, we try to fetch such element from the parent.
             - if we haven't got "palette" property here we try to get it from the parent
             - if we've got the element:
                 - depending on alterPal, backPal color is set to either color2 (true) or color property of the "palette" element
             - if we haven't got the element, backPal is -1
             - if we're in battlescape, battleGame->setPaletteByDepth(this) is called (this is State)
             - if we're in geoscape and pal is empty, it's reset to PAL_GEOSCAPE and setPalette(pal, backPal) is called.
                - in case backPal is -1 ...
             - finally, setPalette(pal, backPal) is called, where pal is a string from a palette property from either the rule or its parent,
               or "PAL_GEOSCAPE" if the rule or the parent doesn't exist, or do not have the property.

        3. State::setPalette():
            - first it changes the cursor if palette is one of
            - then if backPal is not -1, it replaces 16 colors from posn Palette::backPos = 224
              with the ones at backPal*16 starting posn in BACKPALS.DAT palette.

        """
        self.name = name
        self.palname = "PAL_GEOSCAPE"
        #self.parent = None
        #self.backgroundImage = None
        #self.music = None
        #self.sound = None
        for pname, val in pod.items():
            if pname == 'palette':
                self.palname = val
            elif pname == 'elements':
                for elname, elt in val.items():
                    if elname in self._prohibited or elname.startswith('_'):
                        log_error("    InterfaceRule element name '{}' ignored.".format(elname))
                        continue
                    setattr(self, elname, IRULEMENT(elt))
                    #log_info("    InterfaceRule element {}".format(elname))
            elif pname in self._prohibited:
                setattr(self, pname, val)

class INTERFACE_RULES(object):
    def __init__(self):
        yaml = YAML()
        irul_str = ffi.string(lib.get_interface_rules())
        irul_pod =yaml.load(irul_str);
        if irul_pod is None:
            log_error("get_interface_rules returned '{}': can't parse".format(irul_str))
            return
        for catname, cat in irul_pod.items():
            if catname.startswith('_'):
                log_error("Ignoring interface rule '{}'".format(catname))
                continue
            #log_info("IRULE {}".format(catname))
            setattr(self, catname, IRULE(catname, cat))

        #log_info("INTERFACE_RULES id {:x}".format(id(self)))
        # TBD: resolve parent references in rules, which seems to be not necessary, since
        # we call setInterface anyways.

IRUL = INTERFACE_RULES()

def mods_loaded():
    global IRUL
    log_info("api.py::mods_loaded")
    IRUL = INTERFACE_RULES()
    #log_info("IRUL id {:x}".format(id(IRUL)))

class State(object):
    """ Base class for UI states aka windows, a wrapper for state_t from adapter.cpp"""
    def __init__(self, w, h, x, y, ui_id, ui_category, alterpal):
        """
            parent - parent state.
            w,h,x,y - int
            popmode - WindowPopup
            ui_category, bg - interface stuff, pass "geoscape", "saveMenus" for now
            bg - background image name, like "BACK01.SCR"
         """
        log_info("State({}, {}, {}, {}, '{}', '{}')".format(w, h, x, y, ui_id, ui_category))
        ffih = ffi.new_handle(self)
        self._st = lib.new_state(ffih, w, h, x, y, ui_id.encode('utf-8'), ui_category.encode('utf-8'), alterpal)
        self._ran_this_frame = False
        self._frame_count = 0
        self._cbuf = []
        self._surfcache = {}
        self.input = lib.st_get_input_state(self._st)
        #log_info("IRUL id {:x}".format(id(IRUL)))
        self.IR = getattr(IRUL, ui_category)

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

    def _inner_handle(self):
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
    BLIT = 2        # (cmd (x,y,w,h), interned surface_handle, invert_mid)
    SOUND = 3  # (cmd name)  one of BUTTON_PRESS; WINDOW_POPUP_{012}

    # rendering command submission
    def blit(self, dst, srcrect, handle):
        """ blit sprite (sub) image """
        self._cbuf.append((self.BLIT, dst, srcrect, handle))

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

    def clone_surface(self, handle):
        return lib.st_clone_surface(self.ptr, handle)

    def invert_surface(self, handle, inverting_mid):
        lib.st_invert_surface(self.ptr, handle, inverting_mid)

    def play_ui_sound(self, name):
        lib.st_play_ui_sound(self.ptr, name)

class ImmUIState(State):
    """ basic support for an immediate-mode ui """
    ui_cat = "aboutpypy"
    ui_id = "sysvers"
    alter_pal = False
    def __init__(self, w, h, x, y):
        super(ImmUIState, self).__init__(w,h,x,y,
                        ui_id = self.ui_id,
                        ui_category = self.ui_cat,
                        alterpal=self.alter_pal)
        self.w = w
        self.h = h
        self.hot = -1
        self.active = -1
        self.next_id = 1
        self.bg = self.get_surface(self.IR.backgroundImage)

    def get_id(self):
        self.next_id += 1
        return self.next_id

    def hit(self, rect):
        out = ((self.input.mx  < rect[0]) or
               (self.input.my  < rect[1]) or
               (self.input.my >= rect[0] + rect[2]) or
               (self.input.my >= rect[1] + rect[3]))
        return not out

    def run(self):
        """ gets called every frame  | input event """
        # reset ui state
        self.next_id = 0
        self.hot = 0
        # call the logic
        self.logick()
        # clean up ui state
        if self.input.buttons == 0:
            self.active = 0
        elif self.active == 0:
            self.active = -1 # duh! why?

    def drawframe(self, x, y, w, h, color): # 1px frame of color
            self.fill((x,     y,     w, 1), color) # top horizontal
            self.fill((x,     y+h-1, w, 1), color) # bottom horizontal
            self.fill((x,     y,     1, h), color) # left vertical
            self.fill((x+w-1, y,     1, h), color) # right vertical

    def btnborder(self, rect, dark, bright):
        # typical button border
        med = (dark+bright)//2
        x,y,w,h = rect
        self.drawframe(x,   y,   w,   h, dark)  # outer
        self.drawframe(x+1, y+1, w-2, h-2, med) # middle
        self.drawframe(x+2, y+2, w-4, h-4, bright) # inner
        self.fill((x+3, y+3, w-6, h-6), bright) # fill

    def winborder(self, rect, dark, bright, fill = None):
        # typical window border of 5 px: dark, med, bright, med, dark
        med = (dark+bright)//2
        x,y,w,h = rect
        self.drawframe(x,   y,   w,   h, dark)     # outer
        self.drawframe(x+1, y+1, w-2, h-2, med)    # middle
        self.drawframe(x+2, y+2, w-4, h-4, bright) # center
        self.drawframe(x+3, y+3, w-6, h-6, med)    # middle
        self.drawframe(x+4, y+4, w-8, h-8, dark)   # inner
        if fill is not None:
            self.fill((x+4, y+4, w-8, h-81), fill)

    def drawbutton(self, rect, color, inverted, contrast = False, geoscapeButton = True):
        # see TextButton::draw() and Surface::invert(Uint8 mid)
        mul = 2 if contrast else 1
        _color = color
        color = _color + 1 * mul
        if geoscapeButton:
            inverting_mid = _color + 2 * mul
        else:
            inverting_mid = _color + 3 * mul

        if inverted:
            def inverted(pixel):
                return pixel + 2 * (inverting_mid - pixel)
        else:
            def inverted(pixel):
                return pixel

        x,y,w,h = rect
        for i in range(5):
            self.drawframe(x, y, w, h, inverted(color))
            if i % 2 == 0:
                x += 1; y += 1
            w -= 1; h -= 1
            if i == 0:
                color = _color + 5 * mul;
                self.fill((w, 0, 1, 1), inverted(color))
            elif i == 1:
                color = _color + 2 * mul
            elif i == 2:
                color = _color + 4 * mul
                self.fill((w + 1, 1, 1, 1), inverted(color))
            elif i == 3:
                color = _color + 3 * mul
            elif i == 4:
                if geoscapeButton:
                    self.fill((0, 0, 1, 1), inverted(_color))
                    self.fill((1, 1, 1, 1), inverted(_color))
        self.fill((x,y,w,h),inverted(color))
        return inverting_mid

    def text_button(self, rect, text, color, pressed):
        """ your plain text button """
        this = self.get_id()
        # logic
        if self.hit(rect):
            self.hot = this
        if self.active == 0 and self.input.buttons !=0:
            self.active = this

        if pressed is None:
        # default behavior
            pressed = self.hit(rect) and self.mouse_b is not None
        else:
        # selector buttons of a group
            pass

        # our state
        pressed = False
        mouseover = False
        if self.hot == this:
            if self.active == this:
                # both hot and active
                pressed = True
            else:
                # just hot
                mouseover = True
        else:
            # unhot but active? the hell does this mean?
            pass

        # render
        inverting_mid = self.drawbutton(rect, color, pressed)
        x,y,w,h = rect
        if this not in self._surfcache:
            textsurf = self.get_text(w-10, h-10, text, 1, 1, False, color, 0, True)
            ts_inverted  = self.clone_surface(textsurf)
            self.invert_surface(ts_inverted, inverting_mid)
            self._surfcache[this] = (textsurf, ts_inverted)

        textsurf = self._surfcache[this][1] if pressed else self._surfcache[this][0]
        self.c_blit((x+5, y+5), (0,0,0,0), textsurf)
        # render done.
        # check if we've been clickedd
        if self.input.buttons == 0 and self.hot == this and self.active == this:
            self.play_ui_sound(b"BUTTON_PRESS")
            return True
        return False

    def image_button(self, rect, image, pressed):
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
