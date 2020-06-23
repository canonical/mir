/*
 * Copyright (C) 2018 Marius Gripsgard <marius@ubports.com>
 * Copyright (C) 2020 Canonical Ltd.
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
 */

#ifndef MIR_FRONTEND_XCB_CONNECTION_H
#define MIR_FRONTEND_XCB_CONNECTION_H

#include "mir/geometry/point.h"
#include "mir/geometry/size.h"
#include "mir/fatal.h"
#include "mir/fd.h"

#include <xcb/xcb.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <mutex>
#include <atomic>
#include <experimental/optional>

namespace mir
{
namespace frontend
{
/// Types for window properties
enum class XCBType
{
    ATOM,
    WINDOW,
    CARDINAL32, ///< also known as an unsigned int
    STRING,
    UTF8_STRING,
    WM_STATE,
    WM_PROTOCOLS,
};

template<XCBType T> struct NativeXCBType                { typedef void type;};
template<> struct NativeXCBType<XCBType::ATOM>          { typedef xcb_atom_t type; };
template<> struct NativeXCBType<XCBType::WINDOW>        { typedef xcb_window_t type; };
template<> struct NativeXCBType<XCBType::CARDINAL32>    { typedef uint32_t type; };
template<> struct NativeXCBType<XCBType::STRING>        { typedef char type; };
template<> struct NativeXCBType<XCBType::UTF8_STRING>   { typedef char type; };
template<> struct NativeXCBType<XCBType::WM_STATE>      { typedef uint32_t type; };
template<> struct NativeXCBType<XCBType::WM_PROTOCOLS>  { typedef uint32_t type; };

class XCBConnection
{
private:
    Fd const fd;
    xcb_connection_t* const xcb_connection;
    xcb_screen_t* const xcb_screen;

    std::mutex mutable atom_name_cache_mutex;
    std::unordered_map<xcb_atom_t, std::string> mutable atom_name_cache;

public:
    class Atom
    {
    public:
        /// Context should outlive the atom
        Atom(std::string const& name, XCBConnection* connection);
        operator xcb_atom_t() const;

    private:
        Atom(Atom&) = delete;
        Atom& operator=(Atom&) = delete;

        XCBConnection* const connection;
        std::string const name_;
        xcb_intern_atom_cookie_t const cookie;
        std::mutex mutable mutex;

        /// Accessed locklessly, but only set under lock
        /// Once set to a value other than XCB_ATOM_NONE, not changed again
        std::atomic<xcb_atom_t> mutable atom{XCB_ATOM_NONE};
    };

    explicit XCBConnection(Fd const& fd);
    ~XCBConnection();

    /// Throws if the connection has been shut down
    void verify_not_in_error_state() const;

    operator xcb_connection_t*() const { return xcb_connection; }
    auto screen() const -> xcb_screen_t* { return xcb_screen; }
    auto root_window() const -> xcb_window_t { return xcb_screen->root; }

    /// Looks up an atom's name, or requests it from the X server if it is not already cached
    auto query_name(xcb_atom_t atom) const -> std::string;
    auto reply_contains_string_data(xcb_get_property_reply_t const* reply) const -> bool;
    auto string_from(xcb_get_property_reply_t const* reply) const -> std::string;

    /// If the window was created by us
    auto is_ours(xcb_window_t window) const -> bool;

    /// Read a single property of various types from the window
    /// Returns a function that will wait on the reply before calling action()
    /// @{
    auto read_property(
        xcb_window_t window,
        xcb_atom_t prop,
        std::function<void(xcb_get_property_reply_t*)> action,
        std::function<void()> on_error) const -> std::function<void()>;

    auto read_property(
        xcb_window_t window,
        xcb_atom_t prop,
        std::function<void(std::string const&)> action,
        std::function<void()> on_error = [](){}) const -> std::function<void()>;

    auto read_property(
        xcb_window_t window,
        xcb_atom_t prop,
        std::function<void(uint32_t)> action,
        std::function<void()> on_error = [](){}) const -> std::function<void()>;

    auto read_property(
        xcb_window_t window,
        xcb_atom_t prop,
        std::function<void(std::vector<uint32_t>)> action,
        std::function<void()> on_error = [](){}) const -> std::function<void()>;
    /// @}

    /// Set X11 window properties
    /// Safer and more fun than the C-style function provided by XCB
    /// @{
    template<XCBType type, typename T>
    inline void set_property(xcb_window_t window, xcb_atom_t property, T data) const
    {
        set_property<type>(window, property, 1, &data);
    }

    template<XCBType type, typename T, size_t length>
    inline void set_property(xcb_window_t window, xcb_atom_t property, T(&data)[length]) const
    {
        set_property<type>(window, property, length, data);
    }

    template<XCBType type, typename T>
    inline void set_property(xcb_window_t window, xcb_atom_t property, std::vector<T> const& data) const
    {
        set_property<type>(window, property, data.size(), data.data());
    }

    template<XCBType type>
    inline void set_property(xcb_window_t window, xcb_atom_t property, std::string const& data) const
    {
        static_assert(
            type == XCBType::STRING || type == XCBType::UTF8_STRING,
            "String data should only be used with a string type");
        set_property<type>(window, property, data.size(), data.c_str());
    }
    /// @}

    /// Delete an X11 window property
    inline void delete_property(xcb_window_t window, xcb_atom_t property) const
    {
        xcb_delete_property(xcb_connection, window, property);
    }

    /// Wrapper around xcb_configure_window
    void configure_window(
        xcb_window_t window,
        std::experimental::optional<geometry::Point> position,
        std::experimental::optional<geometry::Size> size,
        std::experimental::optional<xcb_window_t> sibling,
        std::experimental::optional<xcb_stack_mode_t> stack_mode) const;

    /// Send client message
    /// @{
    template<XCBType type, typename T, size_t length>
    void send_client_message(xcb_window_t window, uint32_t event_mask, T(&data)[length]) const
    {
        static_assert(length * sizeof(T) <= sizeof(xcb_client_message_data_t), "Too much data");
        send_client_message<type>(window, event_mask, length, data);
    }
    /// @}

    inline void flush() const
    {
        xcb_flush(xcb_connection);
    }

    /// Create strings for debug messages
    /// @{
    auto reply_debug_string(xcb_get_property_reply_t* reply) const -> std::string;
    auto client_message_debug_string(xcb_client_message_event_t* event) const -> std::string;
    auto window_debug_string(xcb_window_t window) const -> std::string;
    auto error_debug_string(xcb_generic_error_t* error) const -> std::string;
    /// @}

    Atom const wm_protocols{"WM_PROTOCOLS", this};
    Atom const wm_take_focus{"WM_TAKE_FOCUS", this};
    Atom const wm_delete_window{"WM_DELETE_WINDOW", this};
    Atom const wm_state{"WM_STATE", this};
    Atom const wm_change_state{"WM_CHANGE_STATE", this};
    Atom const wm_s0{"WM_S0", this};
    Atom const net_wm_cm_s0{"_NET_WM_CM_S0", this};
    Atom const net_wm_name{"_NET_WM_NAME", this};
    Atom const net_wm_pid{"_NET_WM_PID", this};
    Atom const net_wm_state{"_NET_WM_STATE", this};
    Atom const net_wm_state_maximized_vert{"_NET_WM_STATE_MAXIMIZED_VERT", this};
    Atom const net_wm_state_maximized_horz{"_NET_WM_STATE_MAXIMIZED_HORZ", this};
    Atom const net_wm_state_hidden{"_NET_WM_STATE_HIDDEN", this};
    Atom const net_wm_state_fullscreen{"_NET_WM_STATE_FULLSCREEN", this};
    Atom const net_wm_user_time{"_NET_WM_USER_TIME", this};
    Atom const net_wm_icon_name{"_NET_WM_ICON_NAME", this};
    Atom const net_wm_desktop{"_NET_WM_DESKTOP", this};
    Atom const net_wm_window_type{"_NET_WM_WINDOW_TYPE", this};
    Atom const net_wm_window_type_desktop{"_NET_WM_WINDOW_TYPE_DESKTOP", this};
    Atom const net_wm_window_type_dock{"_NET_WM_WINDOW_TYPE_DOCK", this};
    Atom const net_wm_window_type_toolbar{"_NET_WM_WINDOW_TYPE_TOOLBAR", this};
    Atom const net_wm_window_type_menu{"_NET_WM_WINDOW_TYPE_MENU", this};
    Atom const net_wm_window_type_utility{"_NET_WM_WINDOW_TYPE_UTILITY", this};
    Atom const net_wm_window_type_splash{"_NET_WM_WINDOW_TYPE_SPLASH", this};
    Atom const net_wm_window_type_dialog{"_NET_WM_WINDOW_TYPE_DIALOG", this};
    Atom const net_wm_window_type_dropdown{"_NET_WM_WINDOW_TYPE_DROPDOWN_MENU", this};
    Atom const net_wm_window_type_popup{"_NET_WM_WINDOW_TYPE_POPUP_MENU", this};
    Atom const net_wm_window_type_tooltip{"_NET_WM_WINDOW_TYPE_TOOLTIP", this};
    Atom const net_wm_window_type_notification{"_NET_WM_WINDOW_TYPE_NOTIFICATION", this};
    Atom const net_wm_window_type_combo{"_NET_WM_WINDOW_TYPE_COMBO", this};
    Atom const net_wm_window_type_dnd{"_NET_WM_WINDOW_TYPE_DND", this};
    Atom const net_wm_window_type_normal{"_NET_WM_WINDOW_TYPE_NORMAL", this};
    Atom const net_wm_moveresize{"_NET_WM_MOVERESIZE", this};
    Atom const net_supporting_wm_check{"_NET_SUPPORTING_WM_CHECK", this};
    Atom const net_supported{"_NET_SUPPORTED", this};
    Atom const net_active_window{"_NET_ACTIVE_WINDOW", this};
    Atom const motif_wm_hints{"_MOTIF_WM_HINTS", this};
    Atom const clipboard{"CLIPBOARD", this};
    Atom const clipboard_manager{"CLIPBOARD_MANAGER", this};
    Atom const utf8_string{"UTF8_STRING", this};
    Atom const compound_text{"COMPOUND_TEXT", this};
    Atom const wl_surface_id{"WL_SURFACE_ID", this};

private:
    XCBConnection(XCBConnection&) = delete;
    XCBConnection& operator=(XCBConnection&) = delete;

    auto xcb_type_atom(XCBType type) const -> xcb_atom_t;

    template<XCBType type>
    static inline constexpr uint8_t xcb_type_format()
    {
        static_assert(!std::is_same<typename NativeXCBType<type>::type, void>::value, "Missing Native specialization");
        auto const format = sizeof(typename NativeXCBType<type>::type) * 8;
        static_assert(format == 8 || format == 16 || format == 32, "Invalid format");
        return format;
    }

    template<XCBType type, typename T>
    static inline void validate_xcb_type()
    {
        // Accidentally sending a pointer instead of the actual data is an easy error to make
        static_assert(!std::is_pointer<T>::value, "Data type should not be a pointer");
        static_assert(!std::is_same<typename NativeXCBType<type>::type, void>::value, "Missing Native specialization");
        static_assert(std::is_same<typename NativeXCBType<type>::type, T>::value, "Specified type does not match data type");
    }

    template<XCBType type, typename T>
    inline void set_property(
        xcb_window_t window,
        xcb_atom_t property,
        size_t length,
        T const* data) const
    {
        validate_xcb_type<type, T>();
        xcb_change_property(
            xcb_connection,
            XCB_PROP_MODE_REPLACE,
            window,
            property,
            xcb_type_atom(type),
            xcb_type_format<type>(),
            length,
            data);
    }

    template<typename T, size_t dest_len>
    static inline void copy_into_array(T(&dest)[dest_len], size_t src_len, T const* src)
    {
        if (src_len > dest_len)
        {
            fatal_error("%u elements of data does not fit in %u element long array", src_len, dest_len);
        }
        for (unsigned i = 0; i < src_len; i++)
        {
            dest[i] = src[i];
        }
    }

    static inline void set_client_message_data(xcb_client_message_data_t& dest, size_t length, uint8_t const* source)
    {
        copy_into_array(dest.data8, length, source);
    }

    static inline void set_client_message_data(xcb_client_message_data_t& dest, size_t length, uint16_t const* source)
    {
        copy_into_array(dest.data16, length, source);
    }

    static inline void set_client_message_data(xcb_client_message_data_t& dest, size_t length, uint32_t const* source)
    {
        copy_into_array(dest.data32, length, source);
    }

    template<XCBType type, typename T>
    void send_client_message(xcb_window_t window, uint32_t event_mask, size_t length, T const* data) const
    {
        validate_xcb_type<type, T>();
        xcb_client_message_event_t event;
        event.response_type = XCB_CLIENT_MESSAGE;
        event.format = xcb_type_format<type>();
        event.sequence = 0; // Sequence is apparently ignored
        event.window = window;
        event.type = xcb_type_atom(type);
        event.data = {{uint32_t{0}, uint32_t{0}, uint32_t{0}, uint32_t{0}, uint32_t{0}}};

        set_client_message_data(event.data, length, data);
        xcb_send_event(
            xcb_connection,
            0, // Don't propagate
            window,
            event_mask,
            reinterpret_cast<const char*>(&event));
    }
};
}
}

#endif // MIR_FRONTEND_XCB_CONNECTION_H
