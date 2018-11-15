/*
 * Copyright (c) 2011 Sveriges Television AB <info@casparcg.com>
 *
 * This file is part of CasparCG (www.casparcg.com).
 *
 * CasparCG is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * CasparCG is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with CasparCG. If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Julian Waller, git@julusian.co.uk
 */

#include "x11_util.h"

#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>

bool window_always_on_top(const sf::Window& window)
{
    Display* disp = XOpenDisplay(NULL);
    if (!disp)
        return false;

    Window win = window.getSystemHandle();

    Atom   wm_state, wm_state_above;
    XEvent event;

    if ((wm_state = XInternAtom(disp, "_NET_WM_STATE", False)) != None) {
        if ((wm_state_above = XInternAtom(disp, "_NET_WM_STATE_ABOVE", False)) != None) {
            event.xclient.type         = ClientMessage;
            event.xclient.serial       = 0;
            event.xclient.send_event   = True;
            event.xclient.display      = disp;
            event.xclient.window       = win;
            event.xclient.message_type = wm_state;
            event.xclient.format       = 32;
            event.xclient.data.l[0]    = 1;
            event.xclient.data.l[1]    = wm_state_above;
            event.xclient.data.l[2]    = 0;
            event.xclient.data.l[3]    = 0;
            event.xclient.data.l[4]    = 0;

            XSendEvent(disp, DefaultRootWindow(disp), False, SubstructureRedirectMask | SubstructureNotifyMask, &event);
        }
    }

    XCloseDisplay(disp);
    return true;
}
