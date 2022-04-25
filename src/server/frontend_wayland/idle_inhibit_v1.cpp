/*
 * Copyright © 2022 Canonical Ltd.
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
 *
 * Authored by: Grayson Guarino <grayson.guarino@canonical.com>
 */

#include "idle_inhibit_v1.h"

#include "mir/log.h"
#include "mir/executor.h"
#include "mir/scene/surface.h"
#include "mir/scene/null_surface_observer.h"

#include "wl_surface.h"

#include <boost/throw_exception.hpp>
#include <memory>

namespace mf = mir::frontend;
namespace mw = mir::wayland;
namespace ms = mir::scene;

namespace mir
{
namespace frontend
{
struct IdleInhibitV1Ctx
{
    class SurfaceObserver : ms::NullSurfaceObserver
    {
    public:
        SurfaceObserver()
        {
            mir::log_info("SurfaceObserver created!");
        }

        ~SurfaceObserver()
        {
            mir::log_info("SurfaceObserver destroyed!");
        }

        void attrib_changed(const ms::Surface *surf, MirWindowAttrib attrib, int value) override
        {
            (void)surf;
            mir::log_info("SurfaceObserver::attrib_changed() called.");
            if (attrib == mir_window_attrib_focus && value == mir_window_focus_state_unfocused)
                mir::log_info("Unfocused...");
            else if (attrib == mir_window_attrib_focus)
                mir::log_info("Focused...");
        };
    };

    std::shared_ptr<Executor> const wayland_executor;
    std::shared_ptr<ms::IdleHub> const idle_hub;
    std::shared_ptr<ms::SurfaceObserver> surface_observer;
};

class IdleInhibitManagerV1Global
    : public wayland::IdleInhibitManagerV1::Global
{
public:
    IdleInhibitManagerV1Global(
            wl_display* display,
            std::shared_ptr<IdleInhibitV1Ctx> const& ctx);

private:
    void bind(wl_resource* new_resource) override;

    std::shared_ptr<IdleInhibitV1Ctx> const ctx;
};

class IdleInhibitManagerV1
    : public wayland::IdleInhibitManagerV1
{
public:
    IdleInhibitManagerV1(
            wl_resource* resource,
            std::shared_ptr<IdleInhibitV1Ctx> const& ctx);


private:
    void create_inhibitor(struct wl_resource* id, struct wl_resource* surface) override;

    std::shared_ptr<IdleInhibitV1Ctx> const& ctx;
};

class IdleInhibitorV1
    : public wayland::IdleInhibitorV1
{
public:
    IdleInhibitorV1(
            wl_resource *resource,
            std::shared_ptr<IdleInhibitV1Ctx> const& ctx);

    ~IdleInhibitorV1();

private:
    // Not sure why, but removing this unused StateObserver stops the functionality of the IdleInhibit.
    // It seems unlikely that it will ever actually be used.
    struct StateObserver : ms::IdleStateObserver
    {
        StateObserver(IdleInhibitorV1* idle_inhibitor)
                : idle_inhibitor{idle_inhibitor}
        {
        }

        void idle() override;

        void active() override;

        IdleInhibitorV1* const idle_inhibitor;
    };

    std::shared_ptr<IdleInhibitV1Ctx> const ctx;
    std::shared_ptr<StateObserver> const state_observer;
    std::shared_ptr<ms::IdleHub::WakeLock> const wake_lock;
};

    void IdleInhibitorV1::StateObserver::idle()
    {
    }

    void IdleInhibitorV1::StateObserver::active()
    {
    }
}
}

auto mf::create_idle_inhibit_manager_v1(
        wl_display* display,
        std::shared_ptr<Executor> const& wayland_executor,
        std::shared_ptr<scene::IdleHub> const& idle_hub)
-> std::shared_ptr<mw::IdleInhibitManagerV1::Global>
{
    auto surface_observer = std::make_shared<scene::NullSurfaceObserver>();
    auto ctx = std::make_shared<IdleInhibitV1Ctx>(IdleInhibitV1Ctx{
                                                                wayland_executor,
                                                                idle_hub,
                                                                surface_observer});

    return std::make_shared<IdleInhibitManagerV1Global>(display, std::move(ctx));
}

mf::IdleInhibitManagerV1Global::IdleInhibitManagerV1Global(
      wl_display *display,
      std::shared_ptr<IdleInhibitV1Ctx> const& ctx)
    : Global{display, Version<1>()},
      ctx{ctx}
{
    // TODO - remove
    mir::log_info("Client asking for IdleInhibitManagerV1Global");
}

void mf::IdleInhibitManagerV1Global::bind(wl_resource* new_resource)
{
    // TODO - remove
    mir::log_info("Client calling IdleInhibitManagerV1Global::bind()");
    new IdleInhibitManagerV1{new_resource, ctx};
}

mf::IdleInhibitManagerV1::IdleInhibitManagerV1(
        wl_resource *resource,
        std::shared_ptr<IdleInhibitV1Ctx> const& ctx)
        : wayland::IdleInhibitManagerV1{resource, Version<1>()},
          ctx{ctx}
{
    mir::log_info("Client asking for idle inhibit! (Manager)");
}

void mf::IdleInhibitManagerV1::create_inhibitor(struct wl_resource* id, struct wl_resource* surface)
{
    mir::log_info("Client calling IdleInhibitManagerV1::create_inhibitor()");

    auto wl_surface = WlSurface::from(surface);

    if (auto const scene_surface = wl_surface->scene_surface(); scene_surface)
    {
        if (scene_surface.value()->focus_state() != mir_window_focus_state_unfocused)
        {
            mir::log_info("Creating IdleInhibitorV1");
            new IdleInhibitorV1{id, ctx};
            scene_surface.value()->add_observer(ctx->surface_observer);
        }
        else
        {
            mir::log_info("focus_state is wrong. Not creating IdleInhibitorV1.");
        }
    }
}

mf::IdleInhibitorV1::IdleInhibitorV1(wl_resource *resource, std::shared_ptr<IdleInhibitV1Ctx> const& ctx)
        : wayland::IdleInhibitorV1{resource, Version<1>()},
          ctx{ctx},
          state_observer{std::make_shared<StateObserver>(this)},
          wake_lock{ctx->idle_hub->inhibit_idle()}
{
    mir::log_info("IdleInhibitorV1 created!");
    ctx->idle_hub->inhibit_idle();
    ctx->idle_hub->register_interest(state_observer, time::Duration(0));
}

void mf::IdleInhibitorV1::active()
{
    // Currently unused
}

void mf::IdleInhibitorV1::idle()
{
    // Currently unused
}

mf::IdleInhibitorV1::~IdleInhibitorV1()
{
    mir::log_info("IdleInhibitorV1 destroyed!");
    ctx->idle_hub->resume_idle();
    ctx->idle_hub->unregister_interest(*state_observer);
}
