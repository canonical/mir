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
namespace mw = mir::wayland_rs;
namespace ms = mir::scene;

namespace mir
{
namespace frontend
{
struct IdleInhibitV1Ctx
{
    std::shared_ptr<mir::Executor> const wayland_executor;
    std::shared_ptr<ms::IdleHub> const idle_hub;
};
}
}

namespace
{
class IdleInhibitorV1
    : public mw::ZwpIdleInhibitorV1Impl,
      public std::enable_shared_from_this<IdleInhibitorV1>
{
public:
    IdleInhibitorV1(std::shared_ptr<mf::IdleInhibitV1Ctx> const& ctx, mf::WlSurface* surface);
    ~IdleInhibitorV1();

    void init();

private:
    class VisibilityObserver;

    void notify(std::optional<bool> visible, std::optional<bool> hidden);
    void finalize(); ///< May be safely called multiple times

    std::shared_ptr<mf::IdleInhibitV1Ctx> const ctx;
    mw::Weak<mf::WlSurface> const wl_surface;
    mw::DestroyListenerId const surface_destroyed_listener_id;
    std::shared_ptr<VisibilityObserver> visibility_observer;

    bool visible = false;
    bool hidden = false;
    bool finalized = false;
    std::shared_ptr<ms::IdleHub::WakeLock> wake_lock;
    std::weak_ptr<ms::Surface> scene_surface;
};
}

mf::IdleInhibitManagerV1::IdleInhibitManagerV1(
    std::shared_ptr<Executor> wayland_executor,
    std::shared_ptr<ms::IdleHub> idle_hub)
    : ctx{std::make_shared<IdleInhibitV1Ctx>(IdleInhibitV1Ctx{
          std::move(wayland_executor),
          std::move(idle_hub)})}
{
}

auto mf::IdleInhibitManagerV1::create_inhibitor(mw::Weak<mw::WlSurfaceImpl> const& surface)
    -> std::shared_ptr<mw::ZwpIdleInhibitorV1Impl>
{
    auto* wl_surface = &dynamic_cast<WlSurface&>(surface.value());
    auto inhibitor = std::make_shared<IdleInhibitorV1>(ctx, wl_surface);
    inhibitor->init();
    return inhibitor;
}

class IdleInhibitorV1::VisibilityObserver : public mir::scene::NullSurfaceObserver
{
public:
    VisibilityObserver(
        std::weak_ptr<IdleInhibitorV1> inhibitor,
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
                    if (auto const locked = inhibitor.lock())
                    {
                        locked->notify(value == mir_window_visibility_exposed, std::nullopt);
                    }
                });
        }
    }

    void hidden_set_to(const mir::scene::Surface*, bool hide) override
    {
        executor->spawn([inhibitor=inhibitor, hide]
            {
                if (auto const locked = inhibitor.lock())
                {
                    locked->notify(std::nullopt, hide);
                }
            });
    }

private:
    std::shared_ptr<mir::Executor> const executor;
    std::weak_ptr<IdleInhibitorV1> const inhibitor;
};

IdleInhibitorV1::IdleInhibitorV1(
    std::shared_ptr<mf::IdleInhibitV1Ctx> const& ctx,
    mf::WlSurface* wl_surface)
    : ctx{ctx},
      wl_surface{wl_surface->shared_from_this()},
      surface_destroyed_listener_id{wl_surface->add_destroy_listener([this]()
          {
              finalize();
          })}
{
}

void IdleInhibitorV1::init()
{
    visibility_observer = std::make_shared<VisibilityObserver>(weak_from_this(), ctx->wayland_executor);

    auto* surface = &wl_surface.value();
    surface->on_scene_surface_created(
        [weak_self=weak_from_this()](std::shared_ptr<ms::Surface> scene_surface)
        {
            if (auto const self = weak_self.lock())
            {
                // Use immediate_executor so uninteresting observations are processed quickly, we punt interesting
                // observations to an executor ourselves
                scene_surface->register_interest(self->visibility_observer, mir::immediate_executor);
                self->notify(
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
