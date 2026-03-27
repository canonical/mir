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

#ifndef MIR_DEMO_SERVER_WLPROBE_H
#define MIR_DEMO_SERVER_WLPROBE_H

#include <memory>
#include <string>

namespace mir { namespace scene { class Session; }}
struct wl_display;

namespace mir::examples
{
/// An internal client that queries Wayland globals and writes them to a JSON file.
///
/// This client is compatible with miral::StartupInternalClient and
/// miral::InternalClientLauncher and can be used to generate wlprobe-like JSON
/// output
class WlProbe
{
public:
    /// Construct a WlProbe that will write to the specified file.
    ///
    /// \param output_file Path to the JSON file to create
    explicit WlProbe(std::string const& output_file);
    ~WlProbe();

    /// Called when the Wayland client is initialized and ready to query globals.
    void operator()(wl_display* display);

    /// Called when the session has connected to the server.
    void operator()(std::weak_ptr<mir::scene::Session> const& session);

private:
    struct Self;
    std::shared_ptr<Self> const self;
};
}

#endif // MIR_DEMO_SERVER_WLPROBE_H
