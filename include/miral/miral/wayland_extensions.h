/*
 * Copyright © 2018 Canonical Ltd.
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
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIRAL_WAYLAND_EXTENSIONS_H
#define MIRAL_WAYLAND_EXTENSIONS_H

#include <memory>
#include <string>

namespace mir { class Server; }

namespace miral
{
/// Add a user configuration option to Mir's option handling to select
/// the supported Wayland extensions.
/// This adds the command line option '--wayland-extensions' the corresponding
/// MIR_SERVER_WAYLAND_EXTENSIONS environment variable, and the wayland-extensions
/// config line.
/// \remark Since MirAL 2.4
class WaylandExtensions
{
public:
    /// Default extensions supported by Mir
    WaylandExtensions();

    /// Provide a custom set of default extensions (colon separated list)
    /// \note This can only be a subset of supported_extensions().
    explicit WaylandExtensions(std::string const& default_value);

    void operator()(mir::Server& server) const;

    /// All extensions extensions supported by Mir (colon separated list)
    auto supported_extensions() const -> std::string;

    ~WaylandExtensions();
    WaylandExtensions(WaylandExtensions const&);
    auto operator=(WaylandExtensions const&) -> WaylandExtensions&;

private:
    struct Self;
    std::shared_ptr<Self> self;
};
}

#endif //MIRAL_WAYLAND_EXTENSIONS_H
