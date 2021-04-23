/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Cemil Azizoglu <cemil.azizoglu@canonical.com>
 */

#ifndef MIR_X11_RESOURCES_H_
#define MIR_X11_RESOURCES_H_

#include <xcb/xcb.h>
#include <optional>
#include <unordered_map>
#include <functional>
#include <mutex>

typedef struct _XDisplay Display;

namespace mir
{
namespace geometry
{
struct Size;
}
namespace graphics
{
struct DisplayConfigurationOutput;
}

namespace X
{
class X11Connection
{
public:
    X11Connection(xcb_connection_t* conn, ::Display* xlib_dpy);
    X11Connection(X11Connection const&) = delete;
    ~X11Connection();

    xcb_connection_t* const conn; ///< Used for most things
    ::Display* const xlib_dpy; ///< Needed for EGL
};

class X11Resources
{
public:
    class VirtualOutput
    {
    public:
        VirtualOutput() = default;
        virtual ~VirtualOutput() = default;
        VirtualOutput(VirtualOutput const&) = delete;
        VirtualOutput& operator=(VirtualOutput const&) = delete;

        virtual auto configuration() const -> graphics::DisplayConfigurationOutput const& = 0;
        virtual void set_size(geometry::Size const& size) = 0;
    };

    auto get_conn() -> std::shared_ptr<X11Connection>;

    void set_set_output_for_window(xcb_window_t win, VirtualOutput* output);
    void clear_output_for_window(xcb_window_t win);

    /// X11Resources's mutex stays locked while the provided function runs
    void with_output_for_window(xcb_window_t win, std::function<void(std::optional<VirtualOutput*> output)> fn);

    static X11Resources instance;

private:
    std::mutex mutex;
    std::weak_ptr<X11Connection> connection;
    std::unordered_map<xcb_window_t, VirtualOutput*> outputs;
};

}
}
#endif /* MIR_X11_RESOURCES_H_ */
