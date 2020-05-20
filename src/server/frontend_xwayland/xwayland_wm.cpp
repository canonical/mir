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
#include "xwayland_cursors.h"

#include "mir/dispatch/multiplexing_dispatchable.h"
#include "mir/dispatch/readable_fd.h"
#include "mir/fd.h"
#include "mir/terminate_with_current_exception.h"
#include "mir/scene/null_observer.h"
#include "mir/frontend/surface_stack.h"

#include <cstring>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <boost/throw_exception.hpp>

namespace mf = mir::frontend;
namespace ms = mir::scene;

namespace
{
auto create_wm_window(mf::XCBConnection const& connection) -> xcb_window_t
{
    std::string const wm_name{"Mir XWM"};

    xcb_window_t const wm_window = xcb_generate_id(connection);
    xcb_create_window(
        connection,
        XCB_COPY_FROM_PARENT,
        wm_window,
        connection.root_window(),
        0, 0, 10, 10, 0,
        XCB_WINDOW_CLASS_INPUT_OUTPUT,
        connection.screen()->root_visual,
        0, NULL);

    connection.set_property<mf::XCBType::WINDOW>(wm_window, connection.net_supporting_wm_check, wm_window);
    connection.set_property<mf::XCBType::UTF8_STRING>(wm_window, connection.net_wm_name, wm_name);

    connection.set_property<mf::XCBType::WINDOW>(
        connection.root_window(),
        connection.net_supporting_wm_check,
        wm_window);

    /* Claim the WM_S0 selection even though we don't support
     * the --replace functionality. */
    xcb_set_selection_owner(connection, wm_window, connection.wm_s0, XCB_TIME_CURRENT_TIME);
    xcb_set_selection_owner(connection, wm_window, connection.net_wm_cm_s0, XCB_TIME_CURRENT_TIME);

    return wm_window;
}

void check_xfixes(mf::XCBConnection const& connection)
{
    xcb_xfixes_query_version_cookie_t xfixes_cookie;
    xcb_xfixes_query_version_reply_t *xfixes_reply;

    xcb_prefetch_extension_data(connection, &xcb_xfixes_id);
    xcb_prefetch_extension_data(connection, &xcb_composite_id);

    auto const xfixes = xcb_get_extension_data(connection, &xcb_xfixes_id);
    if (!xfixes || !xfixes->present)
    {
        mir::log_warning("xfixes not available");
    }

    xfixes_cookie = xcb_xfixes_query_version(connection, XCB_XFIXES_MAJOR_VERSION, XCB_XFIXES_MINOR_VERSION);
    xfixes_reply = xcb_xfixes_query_version_reply(connection, xfixes_cookie, NULL);

    if (mir::verbose_xwayland_logging_enabled())
    {
        mir::log_debug("xfixes version: %d.%d", xfixes_reply->major_version, xfixes_reply->minor_version);
    }

    free(xfixes_reply);
}

auto focus_mode_to_string(uint32_t focus_mode) -> std::string
{
    switch (focus_mode)
    {
    case XCB_NOTIFY_MODE_NORMAL:        return "normal focus";
    case XCB_NOTIFY_MODE_GRAB:          return "focus grabbed";
    case XCB_NOTIFY_MODE_UNGRAB:        return "focus ungrabbed";
    case XCB_NOTIFY_MODE_WHILE_GRABBED: return "focus while grabbed";
    }
    return "unknown focus mode " + std::to_string(focus_mode);
}
}

class mf::XWaylandSceneObserver
    : public ms::NullObserver
{
public:
    XWaylandSceneObserver(mf::XWaylandWM* wm)
        : wm{wm}
    {
    }

    void surfaces_reordered(ms::SurfaceSet const& affected_surfaces) override
    {
        wm->surfaces_reordered(affected_surfaces);
    }

    mf::XWaylandWM* const wm;
};

mf::XWaylandWM::XWaylandWM(std::shared_ptr<WaylandConnector> wayland_connector, wl_client* wayland_client, Fd const& fd)
    : connection{std::make_shared<XCBConnection>(fd)},
      wayland_connector(wayland_connector),
      wayland_client{wayland_client},
      wm_shell{std::static_pointer_cast<XWaylandWMShell>(wayland_connector->get_extension("x11-support"))},
      cursors{std::make_unique<XWaylandCursors>(connection)},
      wm_window{create_wm_window(*connection)},
      wm_dispatcher{std::make_shared<mir::dispatch::ReadableFd>(
          fd, [this]() { handle_events(); })},
      event_thread{std::make_unique<mir::dispatch::ThreadedDispatcher>(
          "Mir/X11 WM Reader", wm_dispatcher, []() { mir::terminate_with_current_exception(); })},
      scene_observer{std::make_shared<XWaylandSceneObserver>(this)}
{
    check_xfixes(*connection);

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

    cursors->apply_default_to(connection->root_window());

    wm_shell->surface_stack->add_observer(scene_observer);
}

mf::XWaylandWM::~XWaylandWM()
{
    wm_shell->surface_stack->remove_observer(scene_observer);

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

    xcb_destroy_window(*connection, wm_window);
    connection->flush();
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

void mf::XWaylandWM::remember_scene_surface(std::weak_ptr<scene::Surface> const& scene_surface, xcb_window_t window)
{
    std::lock_guard<std::mutex> lock{mutex};
    scene_surfaces.insert(std::make_pair(scene_surface, window));
    scene_surface_set.insert(scene_surface);
}

void mf::XWaylandWM::forget_scene_surface(std::weak_ptr<scene::Surface> const& scene_surface)
{
    std::lock_guard<std::mutex> lock{mutex};
    scene_surfaces.erase(scene_surface);
    scene_surface_set.erase(scene_surface);
}

void mf::XWaylandWM::run_on_wayland_thread(std::function<void()>&& work)
{
    wayland_connector->run_on_wayland_display([work = move(work)](auto){ work(); });
}

void mf::XWaylandWM::surfaces_reordered(scene::SurfaceSet const& affected_surfaces)
{
    bool our_surfaces_affected = false;

    {
        std::lock_guard<std::mutex> lock{mutex};
        for (auto const& surface : affected_surfaces)
        {
            if (scene_surfaces.find(surface) != scene_surfaces.end())
            {
                our_surfaces_affected = true;
                break;
            }
        }
    }

    if (our_surfaces_affected)
    {
        restack_surfaces();
    }
}

void mf::XWaylandWM::restack_surfaces()
{
    std::vector<xcb_window_t> new_order;

    {
        std::lock_guard<std::mutex> lock{mutex};
        auto const new_surface_order = wm_shell->surface_stack->stacking_order_of(scene_surface_set);
        for (auto const& surface : new_surface_order)
        {
            auto const surface_window = scene_surfaces.find(surface);
            if (surface_window != scene_surfaces.end())
            {
                new_order.push_back(surface_window->second);
            }
        }
    }

    xcb_window_t window_below = 0;
    for (auto const& window : new_order)
    {
        if (window_below)
        {
            if (verbose_xwayland_logging_enabled())
            {
                log_debug(
                    "Stacking %s on top of %s",
                    connection->window_debug_string(window).c_str(),
                    connection->window_debug_string(window_below).c_str());
            }

            connection->configure_window(
                window,
                std::experimental::nullopt,
                std::experimental::nullopt,
                window_below,
                XCB_STACK_MODE_ABOVE);
        }
        window_below = window;
    }

    connection->flush();
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
        log_debug(
            "XCB_FOCUS_IN %s on %s",
            focus_mode_to_string(event->mode).c_str(),
            connection->window_debug_string(event->event).c_str());
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
