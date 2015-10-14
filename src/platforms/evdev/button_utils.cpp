/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "button_utils.h"
#include "boost/throw_exception.hpp"

#include "linux/input.h"

#include <stdexcept>

MirPointerButton mir::input::evdev::to_pointer_button(int button)
{
    switch(button)
    {
    case BTN_LEFT: return mir_pointer_button_primary;
    case BTN_RIGHT: return mir_pointer_button_secondary;
    case BTN_MIDDLE: return mir_pointer_button_tertiary;
    case BTN_BACK: return mir_pointer_button_back;
    case BTN_FORWARD: return mir_pointer_button_forward;
    }
    BOOST_THROW_EXCEPTION(std::runtime_error("Invalid mouse button"));
}
