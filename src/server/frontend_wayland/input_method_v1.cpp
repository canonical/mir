/*
 * Copyright © Canonical Ltd.
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
 */

#include "input_method_v1.h"

namespace mf = mir::frontend;

class mf::InputMethodV1::Instance : wayland::InputMethodV1
{
public:
    Instance(wl_resource* new_resource, mf::InputMethodV1* /*method*/)
        : InputMethodV1{new_resource, Version<1>()}
    {}
};

mf::InputMethodV1::InputMethodV1(wl_display *display, std::shared_ptr<Executor> wayland_executor)
    : Global(display, Version<1>()), display(display), wayland_executor(wayland_executor)
{
}

void mf::InputMethodV1::bind(wl_resource *new_resource)
{
    new Instance{new_resource, this};
}
