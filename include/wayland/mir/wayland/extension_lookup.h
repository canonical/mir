/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_WAYLAND_EXTENSION_LOOKUP_H_
#define MIR_WAYLAND_EXTENSION_LOOKUP_H_

#include <string>
#include <vector>

namespace mir
{
    class Server;
}

namespace mir::wayland
{
struct ExtensionLookup
{
    ExtensionLookup(std::vector<std::string> const& extensions);
    auto is_wayland_extension_enabled(std::string const& name) const -> bool;
private:
    std::vector<std::string> const* const extensions;
};
}

#endif
