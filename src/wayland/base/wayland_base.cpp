/*
 * Copyright © 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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
 * Authored By: William Wold <william.wold@canonical.com>
 */

#include <boost/throw_exception.hpp>
#include <wayland-server-core.h>
#include "mir/log.h"

#include "wayland_base.h"

namespace mw = mir::wayland;

mw::Resource::Resource()
{
}

mw::Global::Global(wl_global* global, uint32_t max_version)
    : global{global},
      max_version{max_version}
{
    if (global == nullptr)
    {
        BOOST_THROW_EXCEPTION((std::bad_alloc{}));
    }
}

mw::Global::~Global()
{
    wl_global_destroy(global);
}
