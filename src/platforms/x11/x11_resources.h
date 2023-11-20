/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_X11_RESOURCES_H_
#define MIR_X11_RESOURCES_H_

#include "mir/geometry/forward.h"
#include "mir_toolkit/common.h"


#include <xcb/xcb.h>
#include <optional>
#include <unordered_map>
#include <functional>
#include <mutex>

typedef struct _XDisplay Display;

namespace mir
{
namespace graphics
{
struct DisplayConfigurationOutput;
}

namespace X
{
class XCBConnection
{
public:
    XCBConnection() = default;
    virtual ~XCBConnection() = default;
    XCBConnection(XCBConnection const&) = delete;
    XCBConnection& operator=(XCBConnection const&) = delete;

    virtual auto has_error() const -> int = 0;
    virtual auto get_file_descriptor() const -> int = 0;
    virtual auto poll_for_event() const -> xcb_generic_event_t* = 0;
    virtual auto screen() const -> xcb_screen_t* = 0;
    /// Synchronous for now, since that makes everything simpler
    virtual auto intern_atom(std::string const& name) const -> xcb_atom_t = 0;
    virtual auto get_output_refresh_rate() const -> double = 0;
    virtual auto get_extension_data(xcb_extension_t *ext) const -> xcb_query_extension_reply_t const* = 0;
    virtual auto generate_id() const -> uint32_t = 0;
    virtual auto default_pixel_format() const -> MirPixelFormat = 0;
    virtual void create_window(
        xcb_window_t window,
        int16_t x, int16_t y,
        uint32_t value_mask,
        const void* value_list) const = 0;
    virtual void change_property(
        xcb_window_t window,
        xcb_atom_t property_atom,
        xcb_atom_t type_atom,
        uint8_t format,
        size_t length,
        void const* data) const = 0;
    virtual void map_window(xcb_window_t window) const = 0;
    virtual void destroy_window(xcb_window_t window) const = 0;
    virtual void flush() const = 0;

    /// Note that this will not be able to return a valid connection when mocked
    virtual auto connection() const -> xcb_connection_t* = 0;
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

    static auto instance() -> std::shared_ptr<X11Resources>;

    X11Resources(std::unique_ptr<XCBConnection>&& conn, ::Display* xlib_dpy);
    virtual ~X11Resources();

    void set_set_output_for_window(xcb_window_t win, VirtualOutput* output);
    void clear_output_for_window(xcb_window_t win);
    /// X11Resources's mutex stays locked while the provided function runs
    void with_output_for_window(xcb_window_t win, std::function<void(std::optional<VirtualOutput*> output)> fn);

    std::unique_ptr<XCBConnection> const conn;
    ::Display* const xlib_dpy; ///< Needed for EGL
    xcb_atom_t const UTF8_STRING;
    xcb_atom_t const _NET_WM_NAME;
    xcb_atom_t const WM_PROTOCOLS;
    xcb_atom_t const WM_DELETE_WINDOW;

private:
    X11Resources(X11Resources const&) = delete;
    X11Resources& operator=(X11Resources const&) = delete;

    std::mutex outputs_mutex;
    std::unordered_map<xcb_window_t, VirtualOutput*> outputs;

    static std::mutex instance_mutex;
    static std::weak_ptr<X11Resources> instance_;
};

}
}
#endif /* MIR_X11_RESOURCES_H_ */
