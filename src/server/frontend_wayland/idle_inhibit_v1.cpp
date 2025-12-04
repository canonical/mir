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

#include "idle_inhibit_v1.h"

#include <mir/executor.h>
#include <mir/scene/surface.h>
#include <mir/scene/null_surface_observer.h>

#include "wl_surface.h"

#include <boost/throw_exception.hpp>
#include <memory>

namespace mf = mir::frontend;
namespace mw = mir::wayland;
namespace ms = mir::scene;

namespace
{
struct IdleInhibitV1Ctx
{
    std::shared_ptr<mir::Executor> const wayland_executor;
    std::shared_ptr<ms::IdleHub> const idle_hub;
};

class IdleInhibitManagerV1Global : public mw::IdleInhibitManagerV1::Global
{
public:
    IdleInhibitManagerV1Global(wl_display* display, std::shared_ptr<IdleInhibitV1Ctx> ctx);

private:
    void bind(wl_resource* new_resource) override;

    std::shared_ptr<IdleInhibitV1Ctx> const ctx;
};

class IdleInhibitManagerV1 : public mw::IdleInhibitManagerV1
{
public:
    IdleInhibitManagerV1(wl_resource* resource, std::shared_ptr<IdleInhibitV1Ctx>  ctx);

private:
    void create_inhibitor(struct wl_resource* id, struct wl_resource* surface) override;

    std::shared_ptr<IdleInhibitV1Ctx> const ctx;
};

class IdleInhibitorV1 : public mw::IdleInhibitorV1
{
public:
    IdleInhibitorV1(wl_resource *resource, std::shared_ptr<IdleInhibitV1Ctx> const& ctx, mf::WlSurface* surface);
    ~IdleInhibitorV1();

private:
    class VisibilityObserver;

    void notify(std::optional<bool> visible, std::optional<bool> hidden);
    void finalize(); ///< May be safely called multiple times

    std::shared_ptr<IdleInhibitV1Ctx> const ctx;
    mw::Weak<mf::WlSurface> const wl_surface;
    mw::DestroyListenerId const surface_destroyed_listener_id;
    std::shared_ptr<VisibilityObserver> const visibility_observer;

    bool visible = false;
    bool hidden = false;
    bool finalized = false;
    std::shared_ptr<ms::IdleHub::WakeLock> wake_lock;
    std::weak_ptr<ms::Surface> scene_surface;
};
}

auto mf::create_idle_inhibit_manager_v1(
    wl_display* display,
    std::shared_ptr<Executor> wayland_executor,
    std::shared_ptr<ms::IdleHub> idle_hub)
-> std::shared_ptr<mw::IdleInhibitManagerV1::Global>
{
    auto ctx = std::make_shared<IdleInhibitV1Ctx>(IdleInhibitV1Ctx{
        std::move(wayland_executor),
        std::move(idle_hub)});
    return std::make_shared<IdleInhibitManagerV1Global>(display, std::move(ctx));
}

IdleInhibitManagerV1Global::IdleInhibitManagerV1Global(
    wl_display *display,
    std::shared_ptr<IdleInhibitV1Ctx> const ctx)
    : Global{display, Version<1>()},
      ctx{ctx}
{
}

void IdleInhibitManagerV1Global::bind(wl_resource* new_resource)
{
    new IdleInhibitManagerV1{new_resource, ctx};
}

IdleInhibitManagerV1::IdleInhibitManagerV1(
        wl_resource *resource,
        std::shared_ptr<IdleInhibitV1Ctx> ctx)
        : mw::IdleInhibitManagerV1{resource, Version<1>()},
          ctx{std::move(ctx)}
{
}

void IdleInhibitManagerV1::create_inhibitor(struct wl_resource* id, struct wl_resource* surface)
{
    new IdleInhibitorV1{id, ctx, mf::WlSurface::from(surface)};
}

class IdleInhibitorV1::VisibilityObserver : public mir::scene::NullSurfaceObserver
{
public:
    VisibilityObserver(
        mw::Weak<IdleInhibitorV1> inhibitor,
        std::shared_ptr<mir::Executor> const executor)
        : executor{std::move(executor)},
          inhibitor{std::move(inhibitor)}
    {
    }

    void attrib_changed(mir::scene::Surface const*, MirWindowAttrib attrib, int value) override
    {
        if (attrib == mir_window_attrib_visibility)
        {
            executor->spawn([inhibitor=inhibitor, value]
                {
                    if (inhibitor)
                    {
                        inhibitor.value().notify(value == mir_window_visibility_exposed, std::nullopt);
                    }
                });
        }
    }

    void hidden_set_to(const mir::scene::Surface*, bool hide) override
    {
        executor->spawn([inhibitor=inhibitor, hide]
            {
                if (inhibitor)
                {
                    inhibitor.value().notify(std::nullopt, hide);
                }
            });
    }

private:
    std::shared_ptr<mir::Executor> const executor;
    mw::Weak<IdleInhibitorV1> const inhibitor;
};

IdleInhibitorV1::IdleInhibitorV1(
    wl_resource *resource,
    std::shared_ptr<IdleInhibitV1Ctx> const& ctx,
    mf::WlSurface* wl_surface)
    : mw::IdleInhibitorV1{resource, Version<1>()},
      ctx{ctx},
      wl_surface{wl_surface},
      surface_destroyed_listener_id{wl_surface->add_destroy_listener([this]()
          {
              finalize();
          })},
      visibility_observer{std::make_shared<VisibilityObserver>(mw::make_weak(this), this->ctx->wayland_executor)}
{
    wl_surface->on_scene_surface_created(
        [weak_self=mw::make_weak(this)](std::shared_ptr<ms::Surface> scene_surface)
        {
            if (weak_self)
            {
                // Use immediate_executor so uninteresting observations are processed quickly, we punt interesting
                // observations to an executor ourselves
                scene_surface->register_interest(weak_self.value().visibility_observer, mir::immediate_executor);
                weak_self.value().notify(
                    scene_surface->query(mir_window_attrib_visibility) == mir_window_visibility_exposed,
                    std::nullopt);
            }
        });
}

IdleInhibitorV1::~IdleInhibitorV1()
{
    finalize();
}

void IdleInhibitorV1::notify(std::optional<bool> new_visible, std::optional<bool> new_hidden)
{
    if (finalized)
    {
        return;
    }
    visible = new_visible.value_or(visible);
    hidden = new_hidden.value_or(this->hidden);
    if (visible && !hidden)
    {
        if (!wake_lock)
        {
            wake_lock = ctx->idle_hub->inhibit_idle();
        }
    }
    else
    {
        wake_lock.reset();
    }
}

void IdleInhibitorV1::finalize()
{
    finalized = true;
    if (auto const surface = scene_surface.lock())
    {
        surface->unregister_interest(*visibility_observer);
    }
    scene_surface.reset();
    if (wl_surface)
    {
        wl_surface.value().remove_destroy_listener(surface_destroyed_listener_id);
    }
    wake_lock.reset();
}
