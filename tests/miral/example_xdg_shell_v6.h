/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef MIR_EXAMPLE_XDG_SHELL_V6_H
#define MIR_EXAMPLE_XDG_SHELL_V6_H

#include <miral/wayland_extensions.h>
#include <wayland-server-core.h>

#include <functional>
#include <vector>
#include <memory>

namespace mir
{
namespace examples
{

using XdgShellV6CreateCallback =
    std::function<void(wl_client* client, wl_resource* surface)>;

auto xdg_shell_v6_extension() -> miral::WaylandExtensions::Builder;

auto xdg_shell_v6_extension(XdgShellV6CreateCallback callback)
    -> miral::WaylandExtensions::Builder;

}
}

#endif
