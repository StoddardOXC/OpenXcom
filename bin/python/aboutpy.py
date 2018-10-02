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

#
# about-pypy state/popup
#

# this lists state objects the module exports
__states__ = ['AboutPyPy']#, 'ExecutiveSummary', 'EnterpriseResourceManagement']

# this gets base classes and stuff
from api import *

import sys, pprint

class ImmUITest(State):
    ui_cat = "saveMenus"
    ui_id = "window"
    def __init__(self, w = 300, h = 180, x = 42, y = 23):
        super(ImmUITest, self).__init__(w,h,x,y,
                        ui_id = self.ui_id, ui_category = self.ui_cat)

        self.w = w
        self.h = h
        self.hot = None
        self.active = None

        self.command_buffer = []
        self.next_id = -1



        #self.some_text = self.gget_text(w, h, text, ha, va, wrap, prim, seco, small)
        print("ImmUITest init")
        self.bg = self.get_surface("BACK01.SCR")
        self.some_text = self.get_text(100, 40, "yoba", 0, 1, False, 23, 42, False)

    def get_id(self):
        self.next_id += 1
        return self.next_id

    def regionhit(self, rect):
        out = ((self.input.mx  < rect[0]) or
               (self.input.my  < rect[1]) or
               (self.input.my >= rect[0] + rect[2]) or
               (self.input.my >= rect[1] + rect[3]))
        return not out

    def run(self):
        """ gets called every frame  | input event """
        # reset ui state
        self.hot = None
        # call the logic
        self.logick()
        # clean up ui state
        if self.input.mx == -1:
            self.active = None
        elif self.active is None:
            self.active = -1 # duh! why?

    def border(self, rect, dark, bright, fill = None):
        """ draws teh fancy border on the target
            dark: dark color idx
            bright: bright color idx
            fill: fill with this color if not None.
        """
        def drawframe(x, y, w, h, color): # 1px frame of color
            self.fill((x,   y,   w, 1), color) # top horizontal
            self.fill((x,   y+h, w, 1), color) # bottom horizontal
            self.fill((x,   y,   1, h), color) # left vertical
            self.fill((x+w, y,   1, h), color) # right vertical
        x,y,w,h = rect
        drawframe(x,   y,   w,   h, dark)               # outer
        drawframe(x+1, y+1, w-2, h-2, bright)      # middle
        drawframe(x+2, y+2, w-4, h-4, dark)        # inner
        if fill is not None:
            self.fill((x+3, y+3, w-6, h-6), fill)

    def text_button(self, rect, text, pressed):
        """ your plain text button """
        this = self.get_id()
        # logic
        rv = None
        if self.hit(rect):
            self.hot = this
        if self.active is None and self.mouse_b is not None:
            self.active = this

        if pressed is None:
        # default behavior
            pressed = self.hit(rect) and self.mouse_b is not None
        else:
        # selector buttons of a group
            pass
        # render
        cbuf = []
        cbuf.append("ohohoho")
        self.submit(cbuf)
        return rv

    def image_button(self, rect, image, pressed):
        pass

    def logick(self):
        if self.input.keysym == 27:
            self.pop()
        self.blit((0,0), (0,0,0,0), self.bg)
        self.border((0,0,self.w, self.h), 3, 8)
        self.blit((10,10), (0,0,0,0), self.some_text)

AboutPyPy = ImmUITest

class AboutPyPyX(State):
    ui_category = "mainMenu"
    def __init__(self, parent):
        st_w = 300
        btn_w = 60
        super(AboutPyPy, self).__init__(parent,
            w = st_w, h = 180, x = 10, y = 10,
            popmode = POPUP_BOTH, ui_category = self.ui_category, bg = "BACK01.SCR")

        version = "Python " + sys.version.replace(' (', ' \n(' )
        self.add_text(280, 140, 20, 20, "text", self.ui_category, ALIGN_CENTER, ALIGN_MIDDLE, False, False, version)
        btn = TextButton(self, btn_w, 20, st_w/2 - btn_w/2, 160, "button", "saveMenus", self.tr("STR_OK"));
        btn.onMouseClick(self.btnOk, 1)
        btn.onKeyboardPress(self.btnOk) # no keysym = any key
        self.add(btn)

    def btnOk(self, *args):
        self.pop()

class _ExecutiveSummary(State):
    ui_category = "geoscapeMenu"
    def __init__(self, parent):
        st_w = 640
        st_h = 400
        btn_w = 60
        super(ExecutiveSummary, self).__init__(parent,
            w = st_w, h = st_h, x = 0, y = 0,
            popmode = POPUP_BOTH, ui_category = self.ui_category, bg = "PicardFacepalm")

        tl = TextList(self, st_w - 40, st_h - 40 - 20 - 10 - 20, 20, 20, "text", self.ui_category)
        tl.set_columns(80, 160, 80)
        #tl.set_arrow_column(182, ARROW_VERTICAL);

        #tl.set_selectable(False);
        tl.set_background(self.window); # hmm. why?
        #tl.set_margin(2);

        btn = TextButton(self, btn_w, 20, st_w/2 - btn_w/2, st_h - 20 - 10, "button", self.ui_category, self.tr("STR_OK"));
        btn.onMouseClick(self.btnOk, 1)
        btn.onKeyboardPress(self.btnOk) # no keysym = any key
        self.add(tl)
        self.add(btn)

        bases = Game.get_bases(self)
        if bases is None or len(bases) == 0:
            log_info("no bases")
            return

        tm = 0
        for base in bases:
            #log_info("{}:\n {}".format(base['name'], '\n  '.join(f['type'] for f in base['facilities'])))
            tl.add_row("{}: ic={}".format(base['name'], base['inventory_count']), "facility", "maintenance")
            m = 0
            for fac in base['facilities']:
                tl.add_row("", fac['type'], fac["maintenance"])
                m += fac["maintenance"]
            tl.add_row("Base total", "", m)
            tm += m
            tl.add_row()
        tl.add_row()
        tl.add_row("TOTAL", "", tm)

    def btnOk(self, *args):
        self.pop()


class _ERMConfirm(State):
    ui_category = "sellMenu"
    bg_image = "BACK13.SCR"
    def __init__(self, parent, itemset, title, okcallable, nocallable):
        super(ERMConfirmTransfer, self).__init__(parent,
            w = 320, h = 200, x = 160, y = 100,
            popmode = POPUP_BOTH, ui_category = self.ui_category, bg = self.bg_image)


"""
what we _actually_ need is a TextList with buttons in rows.
"""
class _EnterpriseResourceManagement(State):
    ui_category = "sellMenu"
    def __init__(self, parent):
        st_w = 640
        st_h = 400
        btn_w = 60
        super(EnterpriseResourceManagement, self).__init__(parent,
            w = st_w, h = st_h, x = 0, y = 0,
            popmode = POPUP_BOTH, ui_category = self.ui_category, bg = "PicardFacepalm")

        tl = TextList(self, st_w - 40 - 8, st_h - 40 - 20 - 10 - 20 , 20, 20, "text", self.ui_category)
        tl.set_columns(140, 50, 50, 50, 50, 50, 50)
        tl.set_arrow_column(st_w - 160, ARROW_VERTICAL);
        tl.set_selectable(True);
        tl.set_background(self.window); # hmm. why?
        tl.set_margin(2);

        # buttons:
        #   Transfer
        #   Sell
        #   Clear  - reset all amounts entered
        #   Dismiss
        #
        button_step = btn_w + 16
        button_row_y = st_h - 16 - 18
        button_x = st_w - btn_w - 16

        btn_dismiss  = TextButton(self, btn_w, 20, button_x - 0 * button_step, button_row_y, "button", self.ui_category, self.tr("STR_DISMISS"));
        btn_clear    = TextButton(self, btn_w, 20, button_x - 1 * button_step, button_row_y, "button", self.ui_category, self.tr("STR_CLEAR"));
        btn_sell     = TextButton(self, btn_w, 20, button_x - 2 * button_step, button_row_y, "button", self.ui_category, self.tr("STR_SELL_LC"));
        btn_transfer = TextButton(self, btn_w, 20, button_x - 3 * button_step, button_row_y, "button", self.ui_category, self.tr("STR_TRANSFER"));

        btn_dismiss.onMouseClick(self.dismiss, 1)
        btn_clear.onMouseClick(self.clear_amounts, 1)
        btn_sell.onMouseClick(self.confim_sale, 1)
        btn_transfer.onMouseClick(self.confirm_transfer, 1)
        #btn.onKeyboardPress(self.btnOk) # no keysym = any key
        self.add(tl)
        self.add(btn_dismiss)
        self.add(btn_clear)
        self.add(btn_sell)
        self.add(btn_transfer)

        bases = Game.get_bases(self)
        if bases is None or len(bases) == 0:
            log_info("no bases")
            return

        BOX_NAMES = {
            -1: "STR_STORES", # BOX_STORES
            -2: "STR_LABORATORIES", # BOX_LABS
            -3: "STR_WORKSHOPS_IN", # BOX_SHOP_INPUT
            -4: "STR_WORKSHOPS_OUT", # BOX_SHOP_OUTPUT
            -5: "STR_OUT", # BOX_OUTSIDE
            -6: "STR_TRANSFER", # BOX_TRANSFERS
        }

        def count_parents(idx):
            rv = 1
            box_idx = inv[idx]['box_idx']
            while box_idx >= 0:
                box_idx = inv[box_idx]['box_idx']
                rv += 1
            return rv

        base_idx = 0
        for base in bases:
            tl.add_row(base['name'], "amount", "sell", "rent", "box_idx", "idx", "indent")
            # de fuck. is it fguaranteed that shit in a contained goes after
            # teh container and before any other container? Currently so EXCEPT
            # for teh stdboxes.
            inv = Game.get_base_inventory(self, base_idx, bases[0]['inventory_count'] + 23)
            if 0:
                print(base_idx)
                idx = 0
                for item in inv:
                    if item['box_idx'] == -6:
                        print(idx, count_parents(idx), repr(item))
                    idx += 1

            for box_idx in BOX_NAMES.keys():
                header_added = False
                skipping_mode = False
                idx = -1
                for item in inv:
                    idx += 1
                    if item['box_idx'] < 0:
                        if item['box_idx'] != box_idx:
                            # skip other top containers
                            skipping_mode = True
                            continue
                        else:
                            # that's ours, handle it.
                            skipping_mode = False
                    elif skipping_mode:
                        # skip contents of cans that are in other top can
                        continue

                    indent = count_parents(idx)
                    if not header_added:
                        tl.add_row(BOX_NAMES[box_idx])
                        header_added = True
                    name = Game.tr(self, item['id']) if item['name'] is None else item['name']
                    tl.add_row('  ' * indent + name,
                            item['amount'],
                            item['sell_price'],
                            item['rent_price'],
                            item['box_idx'], idx, indent)

            tl.add_row()
            base_idx += 1

    def dismiss(self, *args):
        self.pop()

    def clear_amounts(self, *args):
        pass

    def confirm_transfer(self, *args):
        pass

    def confim_sale(self, *args):
        pass

