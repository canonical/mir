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

#include "xcb_connection.h"
#include "boost/throw_exception.hpp"

namespace mf = mir::frontend;

mf::XCBConnection::Atom::Atom(std::string const& name, Context const& context)
    : context{context},
      name_{name},
      cookie{xcb_intern_atom(context.xcb_connection, 0, name_.size(), name_.c_str())}
{
}

mf::XCBConnection::Atom::operator xcb_atom_t() const
{
    if (!atom)
    {
        auto const reply = xcb_intern_atom_reply(context.xcb_connection, cookie, nullptr);
        if (!reply)
            BOOST_THROW_EXCEPTION(std::runtime_error("Failed to look up atom " + name_));
        atom = reply->atom;
        free(reply);
    }
    return atom.value();
}

auto mf::XCBConnection::Atom::name() const -> std::string
{
    return name_;
}

mf::XCBConnection::XCBConnection(xcb_connection_t* xcb_connection)
    : context{xcb_connection},
      wm_protocols{"WM_PROTOCOLS", context},
      wm_normal_hints{"WM_NORMAL_HINTS", context},
      wm_take_focus{"WM_TAKE_FOCUS", context},
      wm_delete_window{"WM_DELETE_WINDOW", context},
      wm_state{"WM_STATE", context},
      wm_change_state{"WM_CHANGE_STATE", context},
      wm_s0{"WM_S0", context},
      wm_client_machine{"WM_CLIENT_MACHINE", context},
      net_wm_cm_s0{"_NET_WM_CM_S0", context},
      net_wm_name{"_NET_WM_NAME", context},
      net_wm_pid{"_NET_WM_PID", context},
      net_wm_icon{"_NET_WM_ICON", context},
      net_wm_state{"_NET_WM_STATE", context},
      net_wm_state_maximized_vert{"_NET_WM_STATE_MAXIMIZED_VERT", context},
      net_wm_state_maximized_horz{"_NET_WM_STATE_MAXIMIZED_HORZ", context},
      net_wm_state_hidden{"_NET_WM_STATE_HIDDEN", context},
      net_wm_state_fullscreen{"_NET_WM_STATE_FULLSCREEN", context},
      net_wm_user_time{"_NET_WM_USER_TIME", context},
      net_wm_icon_name{"_NET_WM_ICON_NAME", context},
      net_wm_desktop{"_NET_WM_DESKTOP", context},
      net_wm_window_type{"_NET_WM_WINDOW_TYPE", context},
      net_wm_window_type_desktop{"_NET_WM_WINDOW_TYPE_DESKTOP", context},
      net_wm_window_type_dock{"_NET_WM_WINDOW_TYPE_DOCK", context},
      net_wm_window_type_toolbar{"_NET_WM_WINDOW_TYPE_TOOLBAR", context},
      net_wm_window_type_menu{"_NET_WM_WINDOW_TYPE_MENU", context},
      net_wm_window_type_utility{"_NET_WM_WINDOW_TYPE_UTILITY", context},
      net_wm_window_type_splash{"_NET_WM_WINDOW_TYPE_SPLASH", context},
      net_wm_window_type_dialog{"_NET_WM_WINDOW_TYPE_DIALOG", context},
      net_wm_window_type_dropdown{"_NET_WM_WINDOW_TYPE_DROPDOWN_MENU", context},
      net_wm_window_type_popup{"_NET_WM_WINDOW_TYPE_POPUP_MENU", context},
      net_wm_window_type_tooltip{"_NET_WM_WINDOW_TYPE_TOOLTIP", context},
      net_wm_window_type_notification{"_NET_WM_WINDOW_TYPE_NOTIFICATION", context},
      net_wm_window_type_combo{"_NET_WM_WINDOW_TYPE_COMBO", context},
      net_wm_window_type_dnd{"_NET_WM_WINDOW_TYPE_DND", context},
      net_wm_window_type_normal{"_NET_WM_WINDOW_TYPE_NORMAL", context},
      net_wm_moveresize{"_NET_WM_MOVERESIZE", context},
      net_supporting_wm_check{"_NET_SUPPORTING_WM_CHECK", context},
      net_supported{"_NET_SUPPORTED", context},
      net_active_window{"_NET_ACTIVE_WINDOW", context},
      motif_wm_hints{"_MOTIF_WM_HINTS", context},
      clipboard{"CLIPBOARD", context},
      clipboard_manager{"CLIPBOARD_MANAGER", context},
      targets{"TARGETS", context},
      utf8_string{"UTF8_STRING", context},
      wl_selection{"_WL_SELECTION", context},
      incr{"INCR", context},
      timestamp{"TIMESTAMP", context},
      multiple{"MULTIPLE", context},
      compound_text{"COMPOUND_TEXT", context},
      text{"TEXT", context},
      string{"STRING", context},
      window{"WINDOW", context},
      text_plain_utf8{"text/plain;charset=utf-8", context},
      text_plain{"text/plain", context},
      xdnd_selection{"XdndSelection", context},
      xdnd_aware{"XdndAware", context},
      xdnd_enter{"XdndEnter", context},
      xdnd_leave{"XdndLeave", context},
      xdnd_drop{"XdndDrop", context},
      xdnd_status{"XdndStatus", context},
      xdnd_finished{"XdndFinished", context},
      xdnd_type_list{"XdndTypeList", context},
      xdnd_action_copy{"XdndActionCopy", context},
      wl_surface_id{"WL_SURFACE_ID", context},
      allow_commits{"_XWAYLAND_ALLOW_COMMITS", context}
{
}
