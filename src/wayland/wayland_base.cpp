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

#include "mir/wayland/wayland_base.h"

namespace mw = mir::wayland;

mw::Resource::Resource()
    : destroyed{nullptr}
{
}

mw::Resource::~Resource()
{
    if (destroyed)
    {
        *destroyed = true;
    }
}

auto mw::Resource::destroyed_flag() const -> std::shared_ptr<bool>
{
    if (!destroyed)
    {
        destroyed = std::make_shared<bool>(false);
    }
    return destroyed;
}

mw::Global::Global(wl_global* global)
    : global{global}
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

void mw::internal_error_processing_request(wl_client* client, char const* method_name)
{
#if (WAYLAND_VERSION_MAJOR > 1 || (WAYLAND_VERSION_MAJOR == 1 && WAYLAND_VERSION_MINOR > 16))
    wl_client_post_implementation_error(client, "Mir internal error processing %s request", method_name);
#else
    wl_client_post_no_memory(client);
#endif
    ::mir::log(
        ::mir::logging::Severity::warning,
        "frontend:Wayland",
        std::current_exception(),
        std::string() + "Exception processing " + method_name + " request");
}
