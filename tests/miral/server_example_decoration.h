/*
 * Copyright © 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_SERVER_EXAMPLE_DECORATION_H
#define MIR_SERVER_EXAMPLE_DECORATION_H

#include <miral/wayland_extensions.h>

namespace mir
{
namespace examples
{
auto server_decoration_extension(miral::WaylandExtensions::Context const* context)
-> std::shared_ptr<void>;

extern std::string const server_decoration_extension_name;
}
}

#endif //MIR_SERVER_EXAMPLE_DECORATION_H
