/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alberto Aguirre <alberto.aguirre@canonical.com>
 */

#include "virtual_display.h"

namespace mga=mir::graphics::android;

mga::VirtualDisplay::VirtualDisplay(std::function<void()> enable_virtual_display,
                                    std::function<void()> disable_virtual_display)
    : enable_virtual_display{enable_virtual_display},
      disable_virtual_display{disable_virtual_display}
{
}

mga::VirtualDisplay::~VirtualDisplay()
{
    disable();
}

void mga::VirtualDisplay::enable()
{
    enable_virtual_display();
}

void mga::VirtualDisplay::disable()
{
    disable_virtual_display();
}
