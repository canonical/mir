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

#include "virtual_output.h"

namespace mga=mir::graphics::android;

mga::VirtualOutput::VirtualOutput(std::function<void()> enable_virtual_output,
                                  std::function<void()> disable_virtual_output)
    : enable_virtual_output{enable_virtual_output},
      disable_virtual_output{disable_virtual_output}
{
}

mga::VirtualOutput::~VirtualOutput()
{
    disable();
}

void mga::VirtualOutput::enable()
{
    enable_virtual_output();
}

void mga::VirtualOutput::disable()
{
    disable_virtual_output();
}
