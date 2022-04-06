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
#include "mir/scene/idle_hub.h"

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
    std::shared_ptr<Executor> const wayland_executor;
    std::shared_ptr<scene::IdleHub> const idle_hub;
};

class IdleInhibitManagerV1Global
    : public wayland::IdleInhibitManagerV1::Global
{
public:
    IdleInhibitManagerV1Global(wl_display* display, std::shared_ptr<IdleInhibitV1Ctx> const& ctx);

private:
    void bind(wl_resource* new_resource) override;

    std::shared_ptr<IdleInhibitV1Ctx> const ctx;
};

class IdleInhibitManagerV1
    : public wayland::IdleInhibitManagerV1
{
public:
    IdleInhibitManagerV1(wl_resource* resource, std::shared_ptr<IdleInhibitV1Ctx> const& ctx);

private:
    void create_inhibitor(struct wl_resource* id, struct wl_resource* surface) override;

    std::shared_ptr<IdleInhibitV1Ctx> const ctx;
};

class IdleInhibitorV1
    : public wayland::IdleInhibitorV1
{
public:
    IdleInhibitorV1(wl_resource* resource, std::shared_ptr<IdleInhibitV1Ctx> const& ctx);
    ~IdleInhibitorV1();

private:
    struct StateObserver : ms::IdleStateObserver
    {
        StateObserver(IdleInhibitorV1* idle_inhibitor)
                : idle_inhibitor{idle_inhibitor}
        {
        }

        void idle() override
        {
           // TODO
        }

        void active() override
        {
            // TODO
        }

        IdleInhibitorV1* const idle_inhibitor;
    };

    std::shared_ptr<IdleInhibitV1Ctx> const ctx;
    std::shared_ptr<StateObserver> const state_observer;

    /// Called by the state observer
    /// @{
    void active();
    void idle();
    /// @}


    // wayland::Weak<WlSurface> current_surface; // To be used when detecting active surface
};
}
}

auto mf::create_idle_inhibit_manager_v1(
        wl_display* display,
        std::shared_ptr<Executor> const& wayland_executor,
        std::shared_ptr<scene::IdleHub> const& idle_hub)
-> std::shared_ptr<mw::IdleInhibitManagerV1::Global>
{
    auto ctx = std::shared_ptr<IdleInhibitV1Ctx>{new IdleInhibitV1Ctx{wayland_executor, idle_hub}};
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
    // TODO - remove log and snarky comment
    mir::log_info("Client calling IdleInhibitManagerV1::create_inhibitor()");
    (void)surface;

    // If you're wondering what I'm doing here, so am I...
    // The surface should be watched with a SurfaceObserver and only inhibit if it's visible
    // Chris: Should it only inhibit the output that the surface is visible on?
//    auto const wl_surface = surface;
//    if (!wl_surface)
//    {
//        BOOST_THROW_EXCEPTION(std::runtime_error("failed to resolve WlSurface creating TextInputV2"));
//    }
    new IdleInhibitorV1{id, ctx};
}

mf::IdleInhibitorV1::IdleInhibitorV1(wl_resource *resource, std::shared_ptr<IdleInhibitV1Ctx> const& ctx)
        : wayland::IdleInhibitorV1{resource, Version<1>()},
          ctx{ctx},
          state_observer{std::make_shared<StateObserver>(this)}
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
}