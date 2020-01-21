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

#ifndef MIR_FRONTEND_XCB_ATOMS_H
#define MIR_FRONTEND_XCB_ATOMS_H

#include <xcb/xcb.h>
#include <string>
#include <experimental/optional>

namespace mir
{
namespace frontend
{
class XCBAtoms
{
private:
    struct Context
    {
        xcb_connection_t* const xcb_connection;
    } const context;

public:
    XCBAtoms(xcb_connection_t* xcb_connection);

    class Atom
    {
    public:
        /// Context should outlive the atom
        Atom(std::string const& name, Context const& context);
        operator xcb_atom_t() const;
        auto name() const -> std::string;

    private:
        Atom(Atom&) = delete;
        Atom(Atom&&) = delete;
        Atom& operator=(Atom&) = delete;

        Context const& context;
        std::string const name_;
        xcb_intern_atom_cookie_t const cookie;
        std::experimental::optional<xcb_atom_t> mutable atom;
    };

    Atom const wm_protocols;
    Atom const wm_normal_hints;
    Atom const wm_take_focus;
    Atom const wm_delete_window;
    Atom const wm_state;
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
    XCBAtoms(XCBAtoms&) = delete;
    XCBAtoms(XCBAtoms&&) = delete;
    XCBAtoms& operator=(XCBAtoms&) = delete;
};
}
}

#endif // MIR_FRONTEND_XCB_ATOMS_H
