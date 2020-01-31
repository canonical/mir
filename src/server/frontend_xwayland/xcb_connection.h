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

#include <xcb/xcb.h>
#include <string>
#include <vector>
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
};

template<XCBType T> struct NativeXCBType                { typedef void type;};
template<> struct NativeXCBType<XCBType::ATOM>          { typedef xcb_atom_t type; };
template<> struct NativeXCBType<XCBType::WINDOW>        { typedef xcb_window_t type; };
template<> struct NativeXCBType<XCBType::CARDINAL32>    { typedef uint32_t type; };
template<> struct NativeXCBType<XCBType::STRING>        { typedef char type; };
template<> struct NativeXCBType<XCBType::UTF8_STRING>   { typedef char type; };
template<> struct NativeXCBType<XCBType::WM_STATE>      { typedef uint32_t type; };

class XCBConnection
{
private:
    xcb_connection_t* const xcb_connection;

public:
    class Atom
    {
    public:
        /// Context should outlive the atom
        Atom(std::string const& name, XCBConnection* connection);
        operator xcb_atom_t() const;
        auto name() const -> std::string;

    private:
        Atom(Atom&) = delete;
        Atom(Atom&&) = delete;
        Atom& operator=(Atom&) = delete;

        XCBConnection* const connection;
        std::string const name_;
        xcb_intern_atom_cookie_t const cookie;
        std::experimental::optional<xcb_atom_t> mutable atom;
    };

    explicit XCBConnection(int fd);
    ~XCBConnection();

    operator xcb_connection_t*() const;

    /// Set X11 window properties
    /// Safer and more fun than the C-style function provided by XCB
    /// @{
    template<XCBType type, typename T>
    inline void set_property(xcb_window_t window, xcb_atom_t property, T data)
    {
        set_property<type>(window, property, 1, &data);
    }

    template<XCBType type, typename T, size_t length>
    inline void set_property(xcb_window_t window, xcb_atom_t property, T(&data)[length])
    {
        set_property<type>(window, property, length, data);
    }

    template<XCBType type, typename T>
    inline void set_property(xcb_window_t window, xcb_atom_t property, std::vector<T> const& data)
    {
        set_property<type>(window, property, data.size(), data.data());
    }

    template<XCBType type>
    inline void set_property(xcb_window_t window, xcb_atom_t property, std::string const& data)
    {
        static_assert(
            type == XCBType::STRING || type == XCBType::UTF8_STRING,
            "String data should only be used with a string type");
        set_property<type>(window, property, data.size(), data.c_str());
    }
    /// @}

    /// Delete an X11 window property
    inline void delete_property(xcb_window_t window, xcb_atom_t property)
    {
        xcb_delete_property(xcb_connection, window, property);
    }

    Atom const wm_protocols;
    Atom const wm_normal_hints;
    Atom const wm_take_focus;
    Atom const wm_delete_window;
    Atom const wm_state;
    Atom const wm_change_state;
    Atom const wm_s0;
    Atom const wm_client_machine;
    Atom const net_wm_cm_s0;
    Atom const net_wm_name;
    Atom const net_wm_pid;
    Atom const net_wm_icon;
    Atom const net_wm_state;
    Atom const net_wm_state_maximized_vert;
    Atom const net_wm_state_maximized_horz;
    Atom const net_wm_state_hidden;
    Atom const net_wm_state_fullscreen;
    Atom const net_wm_user_time;
    Atom const net_wm_icon_name;
    Atom const net_wm_desktop;
    Atom const net_wm_window_type;
    Atom const net_wm_window_type_desktop;
    Atom const net_wm_window_type_dock;
    Atom const net_wm_window_type_toolbar;
    Atom const net_wm_window_type_menu;
    Atom const net_wm_window_type_utility;
    Atom const net_wm_window_type_splash;
    Atom const net_wm_window_type_dialog;
    Atom const net_wm_window_type_dropdown;
    Atom const net_wm_window_type_popup;
    Atom const net_wm_window_type_tooltip;
    Atom const net_wm_window_type_notification;
    Atom const net_wm_window_type_combo;
    Atom const net_wm_window_type_dnd;
    Atom const net_wm_window_type_normal;
    Atom const net_wm_moveresize;
    Atom const net_supporting_wm_check;
    Atom const net_supported;
    Atom const net_active_window;
    Atom const motif_wm_hints;
    Atom const clipboard;
    Atom const clipboard_manager;
    Atom const targets;
    Atom const utf8_string;
    Atom const wl_selection;
    Atom const incr;
    Atom const timestamp;
    Atom const multiple;
    Atom const compound_text;
    Atom const text;
    Atom const string;
    Atom const window;
    Atom const text_plain_utf8;
    Atom const text_plain;
    Atom const xdnd_selection;
    Atom const xdnd_aware;
    Atom const xdnd_enter;
    Atom const xdnd_leave;
    Atom const xdnd_drop;
    Atom const xdnd_status;
    Atom const xdnd_finished;
    Atom const xdnd_type_list;
    Atom const xdnd_action_copy;
    Atom const wl_surface_id;
    Atom const allow_commits;

private:
    XCBConnection(XCBConnection&) = delete;
    XCBConnection(XCBConnection&&) = delete;
    XCBConnection& operator=(XCBConnection&) = delete;

    auto xcb_type_atom(XCBType type) const -> xcb_atom_t;

    template<XCBType type, typename T>
    inline void set_property(
        xcb_window_t window,
        xcb_atom_t property,
        size_t length,
        T const* data)
    {
        // Accidentally sending a pointer instead of the actual data is an easy error to make
        static_assert(!std::is_pointer<T>::value, "Data type should not be a pointer");
        static_assert(!std::is_same<typename NativeXCBType<type>::type, void>::value, "Missing Native specialization");
        static_assert(std::is_same<typename NativeXCBType<type>::type, T>::value, "Specified type does not match data type");
        auto const format = sizeof(typename NativeXCBType<type>::type) * 8;
        static_assert(format == 8 || format == 16 || format == 32, "Invalid format");
        xcb_change_property(
            xcb_connection,
            XCB_PROP_MODE_REPLACE,
            window,
            property,
            xcb_type_atom(type),
            format,
            length,
            data);
    }
};
}
}

#endif // MIR_FRONTEND_XCB_CONNECTION_H
