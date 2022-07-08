/*
 * Copyright © 2015 Canonical Ltd.
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
 */

#include "mir/shell/surface_ready_registrar.h"
#include "mir/scene/null_surface_observer.h"
#include "mir/scene/surface.h"
#include "mir/executor.h"

namespace msh = mir::shell;
namespace ms = mir::scene;

struct msh::SurfaceReadyRegistrar::Observer
    : ms::NullSurfaceObserver,
      std::enable_shared_from_this<ms::SurfaceObserver>
{
public:
    Observer(
        SurfaceReadyRegistrar& registrar,
        std::shared_ptr<scene::Surface> const& surface)
        : registrar{registrar},
          surface{surface}
    {
    }

    void frame_posted(scene::Surface const*, int, geometry::Rectangle const&) override
    {
        std::unique_lock<std::mutex> lock{registrar.mutex};
        registrar.observers.erase(surface.get());
        lock.unlock();
        surface->remove_observer(shared_from_this());
        registrar.surface_ready(surface);
    }

    SurfaceReadyRegistrar& registrar;
    std::shared_ptr<scene::Surface> const surface;
};

msh::SurfaceReadyRegistrar::SurfaceReadyRegistrar(ActivateFunction const& surface_ready)
    : surface_ready{surface_ready}
{
}

msh::SurfaceReadyRegistrar::~SurfaceReadyRegistrar()
{
    clear();
}

void msh::SurfaceReadyRegistrar::register_surface(std::shared_ptr<ms::Surface> const& surface)
{
    std::lock_guard<std::mutex> lock{mutex};
    if (observers.find(surface.get()) != observers.end())
    {
        return;
    }
    auto const observer = std::make_shared<Observer>(*this, surface);
    observers[surface.get()] = observer;
    surface->add_observer(observer);
}

void msh::SurfaceReadyRegistrar::unregister_surface(ms::Surface& surface)
{
    std::lock_guard<std::mutex> lock{mutex};
    auto const iter = observers.find(&surface);
    if (iter != observers.end())
    {
        surface.remove_observer(iter->second);
        observers.erase(iter);
    }
}

void msh::SurfaceReadyRegistrar::clear()
{
    std::lock_guard<std::mutex> lock{mutex};
    observers.clear();
}
