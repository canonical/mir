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
#include <mir/scene/idle_hub.h>
#include <mir/scene/surface.h>
#include <mir/scene/null_surface_observer.h>

#include "wl_surface.h"

#include <boost/throw_exception.hpp>
#include <memory>

namespace mf = mir::frontend;
namespace mw = mir::wayland_rs;
namespace ms = mir::scene;

namespace
{
class IdleInhibitorV1 : public mw::IdleInhibitorV1
{
public:
    IdleInhibitorV1(
        std::shared_ptr<mw::Client> client,
        rust::Box<mw::IdleInhibitorV1Middleware> instance,
        uint32_t object_id,
        std::shared_ptr<mf::IdleInhibitV1Ctx> const& ctx,
        mf::WlSurface* surface);
    ~IdleInhibitorV1();

private:
    class VisibilityObserver;

    void notify(std::optional<bool> visible, std::optional<bool> hidden);
    void finalize(); ///< May be safely called multiple times

    std::shared_ptr<mf::IdleInhibitV1Ctx> const ctx;
    mw::Weak<mf::WlSurface> const wl_surface;
    uint64_t const surface_destroyed_listener_id;
    std::shared_ptr<VisibilityObserver> const visibility_observer;

    bool visible = false;
    bool hidden = false;
    bool finalized = false;
    std::shared_ptr<ms::IdleHub::WakeLock> wake_lock;
    std::weak_ptr<ms::Surface> scene_surface;
};
}

mf::IdleInhibitManagerV1::IdleInhibitManagerV1(
    std::shared_ptr<mw::Client> client,
    rust::Box<mw::IdleInhibitManagerV1Middleware> instance,
    uint32_t object_id,
    std::shared_ptr<IdleInhibitV1Ctx> ctx)
    : mw::IdleInhibitManagerV1{std::move(client), std::move(instance), object_id},
      ctx{std::move(ctx)}
{
}

auto mf::IdleInhibitManagerV1::create_inhibitor(
    mw::Weak<mw::Surface> const& surface,
    rust::Box<mw::IdleInhibitorV1Middleware> child_instance,
    uint32_t child_object_id) -> std::shared_ptr<mw::IdleInhibitorV1>
{
    return std::make_shared<IdleInhibitorV1>(
        client,
        std::move(child_instance),
        child_object_id,
        ctx,
        mw::Surface::from<WlSurface>(surface));
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
    std::shared_ptr<mw::Client> client,
    rust::Box<mw::IdleInhibitorV1Middleware> instance,
    uint32_t object_id,
    std::shared_ptr<mf::IdleInhibitV1Ctx> const& ctx,
    mf::WlSurface* wl_surface)
    : mw::IdleInhibitorV1{std::move(client), std::move(instance), object_id},
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
