/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "miral/application_switcher.h"
#include "wayland_app.h"
#include "wlr-foreign-toplevel-management-unstable-v1.h"

#include <memory>
#include <mir/fd.h>
#include <mir/log.h>
#include <mutex>
#include <sys/eventfd.h>
#include <poll.h>

namespace
{
void handle_toplevel(void*, zwlr_foreign_toplevel_manager_v1*, zwlr_foreign_toplevel_handle_v1*);
void handle_finished(void*, zwlr_foreign_toplevel_manager_v1*);
zwlr_foreign_toplevel_manager_v1_listener constexpr toplevel_manager_listener = {
    handle_toplevel,
    handle_finished
};

void handle_title(void*, zwlr_foreign_toplevel_handle_v1*, char const*) {}
void handle_app_id(void*, zwlr_foreign_toplevel_handle_v1*, char const* app_id);
void handle_output_enter(void*, zwlr_foreign_toplevel_handle_v1*, wl_output*) {}
void handle_output_leave(void*, zwlr_foreign_toplevel_handle_v1*, wl_output*) {}
void handle_state(void*, zwlr_foreign_toplevel_handle_v1*, wl_array*);
void handle_toplevel_done(void*, zwlr_foreign_toplevel_handle_v1*) {}
void handle_closed(void*, zwlr_foreign_toplevel_handle_v1*);
zwlr_foreign_toplevel_handle_v1_listener constexpr toplevel_handle_listener = {
    .title = handle_title,
    .app_id = handle_app_id,
    .output_enter = handle_output_enter,
    .output_leave = handle_output_leave,
    .state = handle_state,
    .done = handle_toplevel_done,
    .closed = handle_closed
};

struct ToplevelInfo
{
    zwlr_foreign_toplevel_handle_v1* handle;
    std::optional<std::string> app_id;
};

class ApplicationSwitcherClient : public WaylandApp
{
public:
    explicit ApplicationSwitcherClient(wl_display* display)
        : WaylandApp(display)
    {
    }

    ~ApplicationSwitcherClient() override
    {
        if (toplevel_manager)
            zwlr_foreign_toplevel_manager_v1_destroy(toplevel_manager);
    }

    void set_toplevel_manager(zwlr_foreign_toplevel_manager_v1* new_toplevel_manager)
    {
        toplevel_manager = new_toplevel_manager;
        zwlr_foreign_toplevel_manager_v1_add_listener(toplevel_manager, &toplevel_manager_listener, this);
    }

    void add(zwlr_foreign_toplevel_handle_v1* toplevel)
    {
        std::lock_guard lock{mutex};
        toplevels_in_focus_order.push_back(ToplevelInfo{toplevel, std::nullopt});
    }

    void app_id(zwlr_foreign_toplevel_handle_v1* toplevel, char const* app_id)
    {
        std::lock_guard lock{mutex};
        auto const it = std::ranges::find_if(toplevels_in_focus_order, [toplevel](auto const& element)
        {
            return element.handle == toplevel;
        });
        if (it != toplevels_in_focus_order.end())
        {
            it->app_id = app_id;
        }
    }

    void focus(zwlr_foreign_toplevel_handle_v1* toplevel)
    {
        auto const it = std::ranges::find_if(toplevels_in_focus_order, [toplevel](auto const& element)
        {
            return element.handle == toplevel;
        });

        if (it != toplevels_in_focus_order.end())
        {
            auto const element = *it;
            toplevels_in_focus_order.erase(it);
            toplevels_in_focus_order.insert(toplevels_in_focus_order.begin(), element);
        }
        else
        {
            toplevels_in_focus_order.insert(toplevels_in_focus_order.begin(), ToplevelInfo{toplevel, std::nullopt});
        }
    }

    void remove(zwlr_foreign_toplevel_handle_v1* toplevel)
    {
        std::lock_guard lock{mutex};
        auto const it = std::ranges::find_if(toplevels_in_focus_order, [toplevel](auto const& element)
        {
            return element.handle == toplevel;
        });
        if (it != toplevels_in_focus_order.end())
        {
            toplevels_in_focus_order.erase(it);
        }
    }

protected:
    void new_global(
        wl_registry* registry,
        uint32_t id,
        char const* interface,
        uint32_t) override
    {
        if (std::string_view(interface) == "zwlr_foreign_toplevel_manager_v1")
        {
            set_toplevel_manager(
                static_cast<zwlr_foreign_toplevel_manager_v1*>(wl_registry_bind(registry, id, &zwlr_foreign_toplevel_manager_v1_interface, 2)));
        }
    }

private:
    zwlr_foreign_toplevel_manager_v1* toplevel_manager = nullptr;
    std::mutex mutex;
    std::vector<ToplevelInfo> toplevels_in_focus_order;
};

void handle_toplevel(void* data, zwlr_foreign_toplevel_manager_v1*, zwlr_foreign_toplevel_handle_v1* toplevel)
{
    auto const self = static_cast<ApplicationSwitcherClient*>(data);
    self->add(toplevel);

    zwlr_foreign_toplevel_handle_v1_add_listener(toplevel, &toplevel_handle_listener, self);
}

void handle_finished(void* data, zwlr_foreign_toplevel_manager_v1*)
{
    auto const self = static_cast<ApplicationSwitcherClient*>(data);
    (void)self;
}

void handle_app_id(void* data, zwlr_foreign_toplevel_handle_v1* handle, char const* app_id)
{
    auto const self = static_cast<ApplicationSwitcherClient*>(data);
    self->app_id(handle, app_id);
}

void handle_state(void* data, zwlr_foreign_toplevel_handle_v1* handle, wl_array* states)
{
    auto const self = static_cast<ApplicationSwitcherClient*>(data);
    auto const* states_casted = static_cast<zwlr_foreign_toplevel_handle_v1_state*>(states->data);
    for (size_t i = 0; i < states->size / sizeof(zwlr_foreign_toplevel_handle_v1_state); i++)
    {
        if (states_casted[i] == ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_ACTIVATED)
        {
            self->focus(handle);
            break;
        }
    }
}

void handle_closed(void* data, zwlr_foreign_toplevel_handle_v1* handle)
{
    auto const self = static_cast<ApplicationSwitcherClient*>(data);
    self->remove(handle);
}
}

struct miral::ApplicationSwitcher::Self
{
    Self() : shutdown_signal(eventfd(0, EFD_CLOEXEC))
    {
        if (shutdown_signal == mir::Fd::invalid)
            mir::log_error("Failed to create shutdown notifier");
    }

    void stop() const
    {
        if (eventfd_write(shutdown_signal, 1) == -1)
            mir::log_error("Failed to notify internal decoration client to shutdown");
    }

    mir::Fd const shutdown_signal;
};

miral::ApplicationSwitcher::ApplicationSwitcher()
    : self(std::make_shared<Self>())
{
}

void miral::ApplicationSwitcher::operator()(struct wl_display* display)
{
    ApplicationSwitcherClient client(display);

    enum FdIndices {
        display_fd = 0,
        shutdown,
        indices
    };

    pollfd fds[indices];
    fds[display_fd] = {wl_display_get_fd(display), POLLIN, 0};
    fds[shutdown] = {self->shutdown_signal, POLLIN, 0};

    while (!(fds[shutdown].revents & (POLLIN | POLLERR)))
    {
        while (wl_display_prepare_read(display) != 0)
        {
            if (wl_display_dispatch_pending(display) == -1)
                mir::log_error("Failed to dispatch Wayland events");
        }

        // self.events_dispatched();

        if (poll(fds, indices, -1) == -1)
            mir::log_error("Failed to wait for Wayland events");

        if (fds[display_fd].revents & (POLLIN | POLLERR))
        {
            if (wl_display_read_events(display))
                mir::log_error("Failed to read Wayland events");
        }
        else
        {
            wl_display_cancel_read(display);
        }
    }
}

void miral::ApplicationSwitcher::operator()(std::weak_ptr<mir::scene::Session> const&) const
{
}

void miral::ApplicationSwitcher::next_app()
{
}

void miral::ApplicationSwitcher::prev_app()
{
}

void miral::ApplicationSwitcher::confirm() const
{
}

void miral::ApplicationSwitcher::stop() const
{
    self->stop();
}


