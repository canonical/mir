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
 * Written with alot of help of Weston Xwm
 *
 */

#include "xwayland_wm.h"
#include "xwayland_log.h"
#include "xwayland_surface.h"
#include "xwayland_wm_shell.h"
#include "xwayland_surface_role.h"

#include "mir/dispatch/multiplexing_dispatchable.h"
#include "mir/dispatch/readable_fd.h"
#include "mir/fd.h"
#include "mir/terminate_with_current_exception.h"

#include <cstring>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <boost/throw_exception.hpp>

#ifndef ARRAY_LENGTH
#define ARRAY_LENGTH(a) mir::frontend::length_of(a)
#endif

#define CURSOR_ENTRY(x)        \
    {                          \
        (x), ARRAY_LENGTH((x)) \
    }

static const char *bottom_left_corners[] = {"bottom_left_corner", "sw-resize", "size_bdiag"};

static const char *bottom_right_corners[] = {"bottom_right_corner", "se-resize", "size_fdiag"};

static const char *bottom_sides[] = {"bottom_side", "s-resize", "size_ver"};

static const char *left_ptrs[] = {"left_ptr", "default", "top_left_arrow", "left-arrow"};

static const char *left_sides[] = {"left_side", "w-resize", "size_hor"};

static const char *right_sides[] = {"right_side", "e-resize", "size_hor"};

static const char *top_left_corners[] = {"top_left_corner", "nw-resize", "size_fdiag"};

static const char *top_right_corners[] = {"top_right_corner", "ne-resize", "size_bdiag"};

static const char *top_sides[] = {"top_side", "n-resize", "size_ver"};

struct cursor_alternatives
{
    const char **names;
    size_t count;
};

static const struct cursor_alternatives cursors[] = {
    CURSOR_ENTRY(top_sides),           CURSOR_ENTRY(bottom_sides),         CURSOR_ENTRY(left_sides),
    CURSOR_ENTRY(right_sides),         CURSOR_ENTRY(top_left_corners),     CURSOR_ENTRY(top_right_corners),
    CURSOR_ENTRY(bottom_left_corners), CURSOR_ENTRY(bottom_right_corners), CURSOR_ENTRY(left_ptrs)};

namespace mf = mir::frontend;

mf::XWaylandWM::XWaylandWM(std::shared_ptr<WaylandConnector> wayland_connector, wl_client* wayland_client, int fd)
    : connection{std::make_shared<XCBConnection>(fd)},
      wayland_connector(wayland_connector),
      dispatcher{std::make_shared<mir::dispatch::MultiplexingDispatchable>()},
      wayland_client{wayland_client},
      wm_shell{std::static_pointer_cast<XWaylandWMShell>(wayland_connector->get_extension("x11-support"))}
{
    wm_dispatcher =
        std::make_shared<mir::dispatch::ReadableFd>(mir::Fd{mir::IntOwnedFd{fd}}, [this]() { handle_events(); });
    dispatcher->add_watch(wm_dispatcher);

    event_thread = std::make_unique<mir::dispatch::ThreadedDispatcher>(
        "Mir/X11 WM Reader", dispatcher, []() { mir::terminate_with_current_exception(); });

    wm_get_resources();

    uint32_t const attrib_values[]{
        XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_PROPERTY_CHANGE};

    xcb_change_window_attributes(*connection, connection->root_window(), XCB_CW_EVENT_MASK, attrib_values);

    xcb_composite_redirect_subwindows(*connection, connection->root_window(), XCB_COMPOSITE_REDIRECT_MANUAL);

    xcb_atom_t const supported[]{
        connection->net_wm_moveresize,
        connection->net_wm_state,
        connection->net_wm_state_fullscreen,
        connection->net_wm_state_maximized_vert,
        connection->net_wm_state_maximized_horz,
        connection->net_active_window};

    connection->set_property<XCBType::ATOM>(connection->root_window(), connection->net_supported, supported);

    connection->set_property<XCBType::WINDOW>(
        connection->root_window(),
        connection->net_active_window,
        static_cast<xcb_window_t>(XCB_WINDOW_NONE));
    wm_selector();

    connection->flush();

    create_wm_cursor();
    set_cursor(connection->root_window(), CursorLeftPointer);

    create_wm_window();
    connection->flush();
}

mf::XWaylandWM::~XWaylandWM()
{
    // clear the surfaces map and then destroy all surfaces
    std::map<xcb_window_t, std::shared_ptr<XWaylandSurface>> local_surfaces;

    {
        std::lock_guard<std::mutex> lock{mutex};
        local_surfaces = std::move(surfaces);
        surfaces.clear();
    }

    if (verbose_xwayland_logging_enabled())
        log_debug("Closing %d XWayland surface(s)...", local_surfaces.size());

    for (auto const& surface : local_surfaces)
    {
        if (surface.second)
            surface.second->close();
    }

    local_surfaces.clear();

    if (verbose_xwayland_logging_enabled())
        log_debug("...done closing surfaces");

    if (event_thread)
    {
        dispatcher->remove_watch(wm_dispatcher);
        event_thread.reset();
    }

    // xcb_cursors == 2 when its empty
    if (xcb_cursors.size() != 2)
    {
        mir::log_info("Cleaning cursors");
        for (auto xcb_cursor : xcb_cursors)
        xcb_free_cursor(*connection, xcb_cursor);
    }
}

void mf::XWaylandWM::wm_selector()
{
    xcb_selection_request.requestor = XCB_NONE;

    uint32_t const values[]{
        XCB_EVENT_MASK_PROPERTY_CHANGE};

    xcb_selection_window = xcb_generate_id(*connection);

    xcb_create_window(
        *connection,
        XCB_COPY_FROM_PARENT,
        xcb_selection_window,
        connection->root_window(),
        0, 0, // position
        10, 10, // size
        0, // border width
        XCB_WINDOW_CLASS_INPUT_OUTPUT,
        connection->screen()->root_visual,
        XCB_CW_EVENT_MASK,
        values);

    xcb_set_selection_owner(*connection, xcb_selection_window, connection->clipboard_manager, XCB_TIME_CURRENT_TIME);

    uint32_t const mask =
        XCB_XFIXES_SELECTION_EVENT_MASK_SET_SELECTION_OWNER |
        XCB_XFIXES_SELECTION_EVENT_MASK_SELECTION_WINDOW_DESTROY |
        XCB_XFIXES_SELECTION_EVENT_MASK_SELECTION_CLIENT_CLOSE;

    xcb_xfixes_select_selection_input(*connection, xcb_selection_window, connection->clipboard, mask);
}

void mf::XWaylandWM::set_cursor(xcb_window_t id, const CursorType &cursor)
{
    if (xcb_cursor == cursor)
        return;

    xcb_cursor = cursor;
    uint32_t cursor_value_list = xcb_cursors[cursor];
    xcb_change_window_attributes(*connection, id, XCB_CW_CURSOR, &cursor_value_list);
    connection->flush();
}

void mf::XWaylandWM::create_wm_cursor()
{
    const char *name;
    int count = ARRAY_LENGTH(cursors);

    xcb_cursors.clear();
    xcb_cursors.reserve(count);

    for (int i = 0; i < count; i++)
    {
        for (size_t j = 0; j < cursors[i].count; j++)
        {
            name = cursors[i].names[j];
            xcb_cursors.push_back(xcb_cursor_library_load_cursor(name));
            if (xcb_cursors[i] != static_cast<xcb_cursor_t>(-1))
                break;
        }
    }
}

void mf::XWaylandWM::create_wm_window()
{
    std::string const wm_name{"Mir XWM"};

    wm_window = xcb_generate_id(*connection);
    xcb_create_window(
        *connection,
        XCB_COPY_FROM_PARENT,
        wm_window,
        connection->root_window(),
        0, 0, 10, 10, 0,
        XCB_WINDOW_CLASS_INPUT_OUTPUT,
        connection->screen()->root_visual,
        0, NULL);

    connection->set_property<XCBType::WINDOW>(wm_window, connection->net_supporting_wm_check, wm_window);
    connection->set_property<XCBType::UTF8_STRING>(wm_window, connection->net_wm_name, wm_name);

    connection->set_property<XCBType::WINDOW>(
        connection->root_window(),
        connection->net_supporting_wm_check,
        wm_window);

    /* Claim the WM_S0 selection even though we don't support
     * the --replace functionality. */
    xcb_set_selection_owner(*connection, wm_window, connection->wm_s0, XCB_TIME_CURRENT_TIME);

    xcb_set_selection_owner(*connection, wm_window, connection->net_wm_cm_s0, XCB_TIME_CURRENT_TIME);
}

auto mf::XWaylandWM::get_wm_surface(
    xcb_window_t xcb_window) -> std::experimental::optional<std::shared_ptr<XWaylandSurface>>
{
    std::lock_guard<std::mutex> lock{mutex};

    auto const surface = surfaces.find(xcb_window);
    if (surface == surfaces.end() || !surface->second)
        return std::experimental::nullopt;
    else
        return surface->second;
}

auto mf::XWaylandWM::get_focused_window() -> std::experimental::optional<xcb_window_t>
{
    std::lock_guard<std::mutex> lock{mutex};
    return focused_window;
}

void mf::XWaylandWM::set_focus(xcb_window_t xcb_window, bool should_be_focused)
{
    {
        std::lock_guard<std::mutex> lock{mutex};
        bool const was_focused = (focused_window && focused_window.value() == xcb_window);

        if (verbose_xwayland_logging_enabled())
        {
            log_debug(
                "%s %s %s...",
                should_be_focused ? "Focusing" : "Unfocusing",
                was_focused ? "focused" : "unfocused",
                connection->window_debug_string(xcb_window).c_str());
        }

        if (should_be_focused == was_focused)
            return;

        if (should_be_focused)
            focused_window = xcb_window;
        else
            focused_window = std::experimental::nullopt;
    }

    if (should_be_focused)
    {
        connection->set_property<XCBType::WINDOW>(
            connection->root_window(),
            connection->net_active_window,
            xcb_window);

        if (auto const surface = get_wm_surface(xcb_window))
        {
            surface.value()->take_focus();
        }
    }
    else
    {
        connection->set_property<XCBType::WINDOW>(
            connection->root_window(),
            connection->net_active_window,
            static_cast<xcb_window_t>(XCB_WINDOW_NONE));

        xcb_set_input_focus_checked(
            *connection,
            XCB_INPUT_FOCUS_POINTER_ROOT,
            XCB_NONE,
            XCB_CURRENT_TIME);
    }

    connection->flush();
}

void mf::XWaylandWM::run_on_wayland_thread(std::function<void()>&& work)
{
    wayland_connector->run_on_wayland_display([work = move(work)](auto){ work(); });
}

/* Events */
void mf::XWaylandWM::handle_events()
{
    bool got_events = false;

    while (xcb_generic_event_t* const event = xcb_poll_for_event(*connection))
    {
        try
        {
            handle_event(event);
        }
        catch (...)
        {
            log(
                logging::Severity::warning,
                MIR_LOG_COMPONENT,
                std::current_exception(),
                "Failed to handle xcb event.");
        }
        free(event);
        got_events = true;
    }

    if (got_events)
    {
        connection->flush();
    }
}

void mf::XWaylandWM::handle_event(xcb_generic_event_t* event)
{
    int type = event->response_type & ~0x80;
    switch (type)
    {
    case XCB_BUTTON_PRESS:
    case XCB_BUTTON_RELEASE:
        if (verbose_xwayland_logging_enabled())
            log_debug("XCB_BUTTON_RELEASE");
        //(reinterpret_cast<xcb_button_press_event_t *>(event));
        break;
    case XCB_ENTER_NOTIFY:
        if (verbose_xwayland_logging_enabled())
            log_debug("XCB_ENTER_NOTIFY");
        //(reinterpret_cast<xcb_enter_notify_event_t *>(event));
        break;
    case XCB_LEAVE_NOTIFY:
        if (verbose_xwayland_logging_enabled())
            log_debug("XCB_LEAVE_NOTIFY");
        //(reinterpret_cast<xcb_leave_notify_event_t *>(event));
        break;
    case XCB_MOTION_NOTIFY:
        handle_motion_notify(reinterpret_cast<xcb_motion_notify_event_t *>(event));
        //(reinterpret_cast<xcb_motion_notify_event_t *>(event));
        break;
    case XCB_CREATE_NOTIFY:
        handle_create_notify(reinterpret_cast<xcb_create_notify_event_t *>(event));
        break;
    case XCB_MAP_REQUEST:
        handle_map_request(reinterpret_cast<xcb_map_request_event_t *>(event));
        break;
    case XCB_MAP_NOTIFY:
        if (verbose_xwayland_logging_enabled())
            log_debug("XCB_MAP_NOTIFY");
        //(reinterpret_cast<xcb_map_notify_event_t *>(event));
        break;
    case XCB_UNMAP_NOTIFY:
        handle_unmap_notify(reinterpret_cast<xcb_unmap_notify_event_t *>(event));
        break;
    case XCB_REPARENT_NOTIFY:
        if (verbose_xwayland_logging_enabled())
            log_debug("XCB_REPARENT_NOTIFY");
        //(reinterpret_cast<xcb_reparent_notify_event_t *>(event));
        break;
    case XCB_CONFIGURE_REQUEST:
        handle_configure_request(reinterpret_cast<xcb_configure_request_event_t *>(event));
        break;
    case XCB_CONFIGURE_NOTIFY:
        handle_configure_notify(reinterpret_cast<xcb_configure_notify_event_t *>(event));
        break;
    case XCB_DESTROY_NOTIFY:
        handle_destroy_notify(reinterpret_cast<xcb_destroy_notify_event_t *>(event));
        break;
    case XCB_MAPPING_NOTIFY:
        if (verbose_xwayland_logging_enabled())
            log_debug("XCB_MAPPING_NOTIFY");
        break;
    case XCB_PROPERTY_NOTIFY:
        handle_property_notify(reinterpret_cast<xcb_property_notify_event_t *>(event));
        break;
    case XCB_CLIENT_MESSAGE:
        handle_client_message(reinterpret_cast<xcb_client_message_event_t *>(event));
        break;
    case XCB_FOCUS_IN:
        handle_focus_in(reinterpret_cast<xcb_focus_in_event_t*>(event));
    default:
        break;
    }
}

void mf::XWaylandWM::handle_property_notify(xcb_property_notify_event_t *event)
{
    if (verbose_xwayland_logging_enabled())
    {
        if (event->state == XCB_PROPERTY_DELETE)
        {
            log_debug(
                "XCB_PROPERTY_NOTIFY (%s).%s: deleted",
                connection->window_debug_string(event->window).c_str(),
                connection->query_name(event->atom).c_str());
        }
        else
        {
            auto const log_prop = [this, event](std::string const& value)
                {
                    auto const prop_name = connection->query_name(event->atom);
                    log_debug(
                        "XCB_PROPERTY_NOTIFY (%s).%s: %s",
                        connection->window_debug_string(event->window).c_str(),
                        prop_name.c_str(),
                        value.c_str());
                };

            auto const reply_function = connection->read_property(
                event->window,
                event->atom,
                [this, log_prop](xcb_get_property_reply_t* reply)
                {
                    auto const reply_str = connection->reply_debug_string(reply);
                    log_prop(reply_str);
                },
                [log_prop]()
                {
                    log_prop("Error getting value");
                });

            reply_function();
        }
    }

    if (auto const surface = get_wm_surface(event->window))
    {
        surface.value()->property_notify(event->atom);
    }
}

void mf::XWaylandWM::handle_create_notify(xcb_create_notify_event_t *event)
{
    if (verbose_xwayland_logging_enabled())
    {
        log_debug("XCB_CREATE_NOTIFY parent: %s", connection->window_debug_string(event->parent).c_str());
        log_debug("                  window: %s", connection->window_debug_string(event->window).c_str());
        log_debug("                  position: %d, %d", event->x, event->y);
        log_debug("                  size: %dx%d", event->width, event->height);
        log_debug("                  override_redirect: %s", event->override_redirect ? "yes" : "no");

        if (event->border_width)
            log_warning("border width unsupported (border width %d)", event->border_width);

        auto const props_cookie = xcb_list_properties(*connection, event->window);
        auto const props_reply = xcb_list_properties_reply(*connection, props_cookie, nullptr);
        if (props_reply)
        {
            std::vector<std::function<void()>> functions;
            int const prop_count = xcb_list_properties_atoms_length(props_reply);
            for (int i = 0; i < prop_count; i++)
            {
                auto const atom = xcb_list_properties_atoms(props_reply)[i];

                auto const log_prop = [this, atom](std::string const& value)
                    {
                        auto const prop_name = connection->query_name(atom);
                        log_debug(
                            "                  | %s: %s",
                            prop_name.c_str(),
                            value.c_str());
                    };

                functions.push_back(connection->read_property(
                    event->window,
                    atom,
                    [this, log_prop](xcb_get_property_reply_t* reply)
                    {
                        auto const reply_str = connection->reply_debug_string(reply);
                        log_prop(reply_str);
                    },
                    [log_prop]()
                    {
                        log_prop("Error getting value");
                    }));
            }
            free(props_reply);

            log_debug("                  initial properties: %d", prop_count);
            for (auto const& f : functions)
                f();
        }
        else
        {
            log_debug("                  initial properties: failed to load");
        }
    }

    if (!connection->is_ours(event->window))
    {
        auto const surface = std::make_shared<XWaylandSurface>(
            this,
            connection,
            wm_shell->seat,
            wm_shell->shell,
            event);

        {
            std::lock_guard<std::mutex> lock{mutex};

            if (surfaces.find(event->window) != surfaces.end())
                BOOST_THROW_EXCEPTION(
                    std::runtime_error("X11 window " + std::to_string(event->window) + " created, but already known"));

            surfaces[event->window] = surface;
        }
    }
}

void mf::XWaylandWM::handle_motion_notify(xcb_motion_notify_event_t *event)
{
    if (verbose_xwayland_logging_enabled())
    {
        log_debug("XCB_MOTION_NOTIFY root: %s", connection->window_debug_string(event->root).c_str());
        log_debug("                  event: %s", connection->window_debug_string(event->event).c_str());
        log_debug("                  child: %s", connection->window_debug_string(event->child).c_str());
        log_debug("                  root pos: %d, %d", event->root_x, event->root_y);
        log_debug("                  event pos: %d, %d", event->event_x, event->event_y);
    }
}

void mf::XWaylandWM::handle_destroy_notify(xcb_destroy_notify_event_t *event)
{
    if (verbose_xwayland_logging_enabled())
    {
        log_debug(
            "XCB_DESTROY_NOTIFY window: %s, event: %s",
            connection->window_debug_string(event->window).c_str(),
            connection->window_debug_string(event->event).c_str());
    }

    std::shared_ptr<XWaylandSurface> surface{nullptr};

    {
        std::lock_guard<std::mutex> lock{mutex};
        auto iter = surfaces.find(event->window);
        if (iter != surfaces.end())
        {
            surface = iter->second;
            surfaces.erase(iter);
        }
    }

    if (surface)
        surface->close();
}

void mf::XWaylandWM::handle_map_request(xcb_map_request_event_t *event)
{
    if (verbose_xwayland_logging_enabled())
    {
        log_debug(
            "XCB_MAP_REQUEST %s with parent %s",
            connection->window_debug_string(event->window).c_str(),
            connection->window_debug_string(event->parent).c_str());
    }

    if (auto const surface = get_wm_surface(event->window))
    {
        surface.value()->map();
    }
}

void mf::XWaylandWM::handle_unmap_notify(xcb_unmap_notify_event_t *event)
{
    if (verbose_xwayland_logging_enabled())
    {
        log_debug(
            "XCB_UNMAP_NOTIFY %s with event %s",
            connection->window_debug_string(event->window).c_str(),
            connection->window_debug_string(event->event).c_str());
    }

    if (connection->is_ours(event->window))
        return;

    // We just ignore the ICCCM 4.1.4 synthetic unmap notify
    // as it may come in after we've destroyed the window
    if (event->response_type & ~0x80)
        return;

    if (auto const surface = get_wm_surface(event->window))
    {
        surface.value()->close();
    }
}

void mf::XWaylandWM::handle_client_message(xcb_client_message_event_t *event)
{
    if (verbose_xwayland_logging_enabled())
    {
        log_debug(
            "XCB_CLIENT_MESSAGE %s on %s: %s",
            connection->query_name(event->type).c_str(),
            connection->window_debug_string(event->window).c_str(),
            connection->client_message_debug_string(event).c_str());
    }

    if (auto const surface = get_wm_surface(event->window))
    {
        // TODO: net_active_window?
        if (event->type == connection->net_wm_moveresize)
            handle_move_resize(surface.value(), event);
        else if (event->type == connection->net_wm_state)
            surface.value()->net_wm_state_client_message(event->data.data32);
        else if (event->type == connection->wm_change_state)
            surface.value()->wm_change_state_client_message(event->data.data32);
        else if (event->type == connection->wl_surface_id)
            handle_surface_id(surface.value(), event);
    }
}

void mf::XWaylandWM::handle_move_resize(std::shared_ptr<XWaylandSurface> surface, xcb_client_message_event_t *event)
{
    if (!surface || !event)
        return;

    int detail = event->data.data32[2];
    surface->move_resize(detail);
}

void mf::XWaylandWM::handle_surface_id(
    std::weak_ptr<XWaylandSurface> const& weak_surface,
    xcb_client_message_event_t *event)
{
    uint32_t id = event->data.data32[0];

    wayland_connector->run_on_wayland_display([
            wayland_connector = wayland_connector,
            client=wayland_client,
            id,
            weak_surface,
            weak_shell = std::weak_ptr<shell::Shell>{wm_shell->shell}](auto)
        {
            wayland_connector->on_surface_created(client, id, [weak_surface, weak_shell](WlSurface* wl_surface)
                {
                    auto const surface = weak_surface.lock();
                    auto const shell = weak_shell.lock();
                    if (surface && shell)
                    {
                        surface->attach_wl_surface(wl_surface);

                        // Will destroy itself
                        new XWaylandSurfaceRole{shell, surface, wl_surface};
                    }
                    else
                    {
                        if (verbose_xwayland_logging_enabled())
                        {
                            log_debug(
                                "wl_surface@%d created but surface or shell has been destroyed",
                                wl_resource_get_id(wl_surface->resource));
                        }
                    }
                });
        });
}

void mf::XWaylandWM::handle_configure_request(xcb_configure_request_event_t *event)
{
    if (verbose_xwayland_logging_enabled())
    {
        log_debug("XCB_CONFIGURE_REQUEST parent: %s", connection->window_debug_string(event->parent).c_str());
        log_debug("                      window: %s", connection->window_debug_string(event->window).c_str());
        log_debug("                      sibling: %s", connection->window_debug_string(event->sibling).c_str());
        log_debug("                      position: %d, %d", event->x, event->y);
        log_debug("                      size: %dx%d", event->width, event->height);

        if (event->border_width)
            log_warning("border width unsupported (border width %d)", event->border_width);
    }

    if (auto const surface = get_wm_surface(event->window))
    {
        surface.value()->configure_request(event);
    }
}

void mf::XWaylandWM::handle_configure_notify(xcb_configure_notify_event_t *event)
{
    if (verbose_xwayland_logging_enabled())
    {
        log_debug("XCB_CONFIGURE_NOTIFY event: %s", connection->window_debug_string(event->event).c_str());
        log_debug("                     window: %s", connection->window_debug_string(event->window).c_str());
        log_debug("                     above_sibling: %s", connection->window_debug_string(event->above_sibling).c_str());
        log_debug("                     position: %d, %d", event->x, event->y);
        log_debug("                     size: %dx%d", event->width, event->height);
        log_debug("                     override_redirect: %s", event->override_redirect ? "yes" : "no");

        if (event->border_width)
            log_warning("border width unsupported (border width %d)", event->border_width);
    }

    if (auto const surface = get_wm_surface(event->window))
    {
        surface.value()->configure_notify(event);
    }
}

void mf::XWaylandWM::handle_focus_in(xcb_focus_in_event_t* event)
{
    if (verbose_xwayland_logging_enabled())
    {
        std::string mode_str;
        switch (event->mode)
        {
        case XCB_NOTIFY_MODE_NORMAL:        mode_str = "normal focus"; break;
        case XCB_NOTIFY_MODE_GRAB:          mode_str = "focus grabbed"; break;
        case XCB_NOTIFY_MODE_UNGRAB:        mode_str = "focus ungrabbed"; break;
        case XCB_NOTIFY_MODE_WHILE_GRABBED: mode_str = "focus while grabbed"; break;
        default: mode_str = "unknown focus mode " + std::to_string(event->mode); break;
        }
        log_debug("XCB_FOCUS_IN %s on %s", mode_str.c_str(), connection->window_debug_string(event->event).c_str());
    }

    // Ignore grabs
    if (event->mode != XCB_NOTIFY_MODE_GRAB && event->mode != XCB_NOTIFY_MODE_UNGRAB)
    {
        std::lock_guard<std::mutex> lock{mutex};
        focused_window = event->event;
        // We might want to keep X11 focus and Mir focus in sync
        // (either by requesting a focus change in Mir, reverting this X11 focus change or both)
    }
}

// Cursor
xcb_cursor_t mf::XWaylandWM::xcb_cursor_image_load_cursor(const XcursorImage *img)
{
    xcb_connection_t *c = *connection;
    xcb_screen_iterator_t s = xcb_setup_roots_iterator(xcb_get_setup(c));
    xcb_screen_t *screen = s.data;
    xcb_gcontext_t gc;
    xcb_pixmap_t pix;
    xcb_render_picture_t pic;
    xcb_cursor_t cursor;
    int stride = img->width * 4;

    pix = xcb_generate_id(c);
    xcb_create_pixmap(c, 32, pix, screen->root, img->width, img->height);

    pic = xcb_generate_id(c);
    xcb_render_create_picture(c, pic, pix, xcb_format_rgba.id, 0, 0);

    gc = xcb_generate_id(c);
    xcb_create_gc(c, gc, pix, 0, 0);

    xcb_put_image(c, XCB_IMAGE_FORMAT_Z_PIXMAP, pix, gc, img->width, img->height, 0, 0, 0, 32, stride * img->height,
                  (uint8_t *)img->pixels);
    xcb_free_gc(c, gc);

    cursor = xcb_generate_id(c);
    xcb_render_create_cursor(c, cursor, pic, img->xhot, img->yhot);

    xcb_render_free_picture(c, pic);
    xcb_free_pixmap(c, pix);

    return cursor;
}

xcb_cursor_t mf::XWaylandWM::xcb_cursor_images_load_cursor(const XcursorImages *images)
{
    if (images->nimage != 1)
        return -1;

    return xcb_cursor_image_load_cursor(images->images[0]);
}

xcb_cursor_t mf::XWaylandWM::xcb_cursor_library_load_cursor(const char *file)
{
    xcb_cursor_t cursor;
    XcursorImages *images;
    char *v = NULL;
    int size = 0;

    if (!file)
        return 0;

    v = getenv("XCURSOR_SIZE");
    if (v)
        size = atoi(v);

    if (!size)
        size = 32;

    images = XcursorLibraryLoadImages(file, NULL, size);
    if (!images)
        return -1;

    cursor = xcb_cursor_images_load_cursor(images);
    XcursorImagesDestroy(images);

    return cursor;
}

void mf::XWaylandWM::wm_get_resources()
{
    xcb_xfixes_query_version_cookie_t xfixes_cookie;
    xcb_xfixes_query_version_reply_t *xfixes_reply;
    xcb_render_query_pict_formats_reply_t *formats_reply;
    xcb_render_query_pict_formats_cookie_t formats_cookie;
    xcb_render_pictforminfo_t *formats;
    uint32_t i;

    xcb_prefetch_extension_data(*connection, &xcb_xfixes_id);
    xcb_prefetch_extension_data(*connection, &xcb_composite_id);

    formats_cookie = xcb_render_query_pict_formats(*connection);

    xfixes = xcb_get_extension_data(*connection, &xcb_xfixes_id);
    if (!xfixes || !xfixes->present)
        log_warning("xfixes not available");

    xfixes_cookie = xcb_xfixes_query_version(*connection, XCB_XFIXES_MAJOR_VERSION, XCB_XFIXES_MINOR_VERSION);
    xfixes_reply = xcb_xfixes_query_version_reply(*connection, xfixes_cookie, NULL);

    if (verbose_xwayland_logging_enabled())
        log_debug("xfixes version: %d.%d", xfixes_reply->major_version, xfixes_reply->minor_version);

    free(xfixes_reply);

    formats_reply = xcb_render_query_pict_formats_reply(*connection, formats_cookie, 0);
    if (formats_reply == NULL)
        return;

    formats = xcb_render_query_pict_formats_formats(formats_reply);
    for (i = 0; i < formats_reply->num_formats; i++)
    {
        if (formats[i].direct.red_mask != 0xff && formats[i].direct.red_shift != 16)
            continue;
        if (formats[i].type == XCB_RENDER_PICT_TYPE_DIRECT && formats[i].depth == 24)
            xcb_format_rgb = formats[i];
        if (formats[i].type == XCB_RENDER_PICT_TYPE_DIRECT && formats[i].depth == 32 &&
            formats[i].direct.alpha_mask == 0xff && formats[i].direct.alpha_shift == 24)
            xcb_format_rgba = formats[i];
    }

    free(formats_reply);
}
