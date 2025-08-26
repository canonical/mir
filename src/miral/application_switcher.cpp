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
struct ToplevelInfo
{
    zwlr_foreign_toplevel_handle_v1* handle;
    std::optional<std::string> app_id;
};
}

class miral::ApplicationSwitcher::Self : public WaylandApp
{
public:
    Self() : shutdown_signal(eventfd(0, EFD_CLOEXEC))
    {
        if (shutdown_signal == mir::Fd::invalid)
            mir::log_error("Failed to create shutdown notifier");
    }

    ~Self() override
    {
        if (toplevel_manager)
            zwlr_foreign_toplevel_manager_v1_destroy(toplevel_manager);
    }

    void add(zwlr_foreign_toplevel_handle_v1* toplevel)
    {
        std::lock_guard lock{mutex};
        toplevels_in_focus_order.push_back(ToplevelInfo{toplevel, std::nullopt});
        if (!tentative_focus_index)
            tentative_focus_index = 0;
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

    /// Focus the provided toplevel.
    ///
    /// This entails bringing it to the front of the focus order.
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

        std::size_t index = std::distance(std::begin(toplevels_in_focus_order), it);
        if (index == tentative_focus_index)
            tentative_focus_index = 0;
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

            if (toplevels_in_focus_order.empty())
                tentative_focus_index = std::nullopt;
            else if (!tentative_focus_index || *tentative_focus_index == toplevels_in_focus_order.size())
                tentative_focus_index = toplevels_in_focus_order.size() - 1;
        }
    }

    void run_client(wl_display* display)
    {
        wayland_init(display);

        enum FdIndices {
            display_fd = 0,
            shutdown,
            indices
        };

        pollfd fds[indices];
        fds[display_fd] = {wl_display_get_fd(display), POLLIN, 0};
        fds[shutdown] = {shutdown_signal, POLLIN, 0};

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

    void confirm()
    {
        std::lock_guard lock{mutex};
        if (tentative_focus_index)
            zwlr_foreign_toplevel_handle_v1_activate(toplevels_in_focus_order[*tentative_focus_index].handle, seat());
        stop();
    }

    void stop() const
    {
        if (eventfd_write(shutdown_signal, 1) == -1)
            mir::log_error("Failed to notify internal decoration client to shutdown");
    }

    void next_app()
    {
        std::lock_guard lock{mutex};
        if (!tentative_focus_index)
            return;

        // Find the window with the next unique application id.
        auto const start_index = *tentative_focus_index;
        auto const app_id = toplevels_in_focus_order[*tentative_focus_index].app_id;
        do
        {
            (*tentative_focus_index)++;
            if (*tentative_focus_index == toplevels_in_focus_order.size())
                *tentative_focus_index = 0;

        } while (start_index != *tentative_focus_index &&
            toplevels_in_focus_order[*tentative_focus_index].app_id == app_id);
    }

    void prev_app()
    {
        std::lock_guard lock{mutex};
        if (!tentative_focus_index)
            return;

        // Find the window with the next unique application id in reverse.
        auto const start_index = *tentative_focus_index;
        auto const app_id = toplevels_in_focus_order[*tentative_focus_index].app_id;
        do
        {
            if (*tentative_focus_index == 0)
                tentative_focus_index = toplevels_in_focus_order.size() - 1;
            else
                (*tentative_focus_index)--;

        } while (start_index != *tentative_focus_index &&
            toplevels_in_focus_order[*tentative_focus_index].app_id == app_id);
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
            toplevel_manager = static_cast<zwlr_foreign_toplevel_manager_v1*>(
                wl_registry_bind(registry, id, &zwlr_foreign_toplevel_manager_v1_interface, 2));
            zwlr_foreign_toplevel_manager_v1_add_listener(toplevel_manager, &toplevel_manager_listener, this);
        }
    }

private:
    static void handle_toplevel(void* data, zwlr_foreign_toplevel_manager_v1*, zwlr_foreign_toplevel_handle_v1* toplevel);
    static void handle_finished(void* data, zwlr_foreign_toplevel_manager_v1*)
    {
        auto const self = static_cast<Self*>(data);
        (void)self;
    }
    zwlr_foreign_toplevel_manager_v1_listener toplevel_manager_listener = {
        handle_toplevel,
        handle_finished
    };

    static void handle_title(void*, zwlr_foreign_toplevel_handle_v1*, char const*) {}
    static void handle_output_enter(void*, zwlr_foreign_toplevel_handle_v1*, wl_output*) {}
    static void handle_output_leave(void*, zwlr_foreign_toplevel_handle_v1*, wl_output*) {}
    static void handle_toplevel_done(void*, zwlr_foreign_toplevel_handle_v1*) {}
    static void handle_app_id(void* data, zwlr_foreign_toplevel_handle_v1* handle, char const* app_id)
    {
        auto const self = static_cast<Self*>(data);
        self->app_id(handle, app_id);
    }

    static void handle_state(void* data, zwlr_foreign_toplevel_handle_v1* handle, wl_array* states)
    {
        auto const self = static_cast<Self*>(data);
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

    static void handle_closed(void* data, zwlr_foreign_toplevel_handle_v1* handle)
    {
        auto const self = static_cast<Self*>(data);
        self->remove(handle);
    }

    zwlr_foreign_toplevel_handle_v1_listener toplevel_handle_listener = {
        .title = handle_title,
        .app_id = handle_app_id,
        .output_enter = handle_output_enter,
        .output_leave = handle_output_leave,
        .state = handle_state,
        .done = handle_toplevel_done,
        .closed = handle_closed
    };

    mir::Fd const shutdown_signal;
    std::mutex mutex;
    zwlr_foreign_toplevel_manager_v1* toplevel_manager = nullptr;
    std::vector<ToplevelInfo> toplevels_in_focus_order;
    std::optional<size_t> tentative_focus_index;
};

void miral::ApplicationSwitcher::Self::handle_toplevel(void* data, zwlr_foreign_toplevel_manager_v1*, zwlr_foreign_toplevel_handle_v1* toplevel)
{
    auto const self = static_cast<Self*>(data);
    self->add(toplevel);
    zwlr_foreign_toplevel_handle_v1_add_listener(toplevel, &self->toplevel_handle_listener, self);
}

miral::ApplicationSwitcher::ApplicationSwitcher()
    : self(std::make_shared<Self>())
{
}

void miral::ApplicationSwitcher::operator()(wl_display* display)
{
    self->run_client(display);
}

void miral::ApplicationSwitcher::operator()(std::weak_ptr<mir::scene::Session> const&) const
{
}

void miral::ApplicationSwitcher::next_app() const
{
    self->next_app();
}

void miral::ApplicationSwitcher::prev_app() const
{
    self->prev_app();
}

void miral::ApplicationSwitcher::confirm() const
{
    self->confirm();
}

void miral::ApplicationSwitcher::stop() const
{
    self->stop();
}


