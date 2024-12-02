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

#ifndef MIRAL_EXTENSION_LOOKUP
#define MIRAL_EXTENSION_LOOKUP

#include <memory>
#include <string>

namespace mir
{
    class Server;
}

namespace miral
{
struct ExtensionLookup
{
    ExtensionLookup();
    void operator()(mir::Server& server);
    auto is_wayland_extension_enabled(std::string const& name) const -> bool;
private:
    struct Self;
    std::shared_ptr<Self> self;
};
}

#endif
