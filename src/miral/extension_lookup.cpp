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

#include "miral/extension_lookup.h"
#include "mir/server.h"
#include <memory>

struct miral::ExtensionLookup::Self
{
    mir::Server* server;
};

void miral::ExtensionLookup::operator()(mir::Server& server)
{
    self->server = &server;
}

auto miral::ExtensionLookup::is_wayland_extension_enabled(std::string const& name) const -> bool
{
    return self->server->is_wayland_extension_enabled(name);
}

miral::ExtensionLookup::ExtensionLookup()
    : self{std::make_shared<Self>()}
{
}
