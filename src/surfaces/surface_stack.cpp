/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 *   Alan Griffiths <alan@octopull.co.uk>
 *   Thomas Voss <thomas.voss@canonical.com>
 */

#include "mir/compositor/buffer_bundle.h"
#include "mir/compositor/buffer_bundle_factory.h"
#include "mir/compositor/buffer_texture_binder.h"
#include "mir/graphics/renderer.h"
#include "mir/surfaces/surface.h"
#include "mir/surfaces/surface_stack.h"

#include <algorithm>
#include <cassert>
#include <functional>
#include <memory>
#include <set>

namespace ms = mir::surfaces;
namespace mc = mir::compositor;
namespace mg = mir::graphics;

namespace
{

template<typename Lockable>
struct LockGuardDeleter
{
    LockGuardDeleter(Lockable& lockable) : lockable(lockable)
    {
        lockable.lock();
    }
    
    template<typename T>
    void operator()(T* t)
    {
        lockable.unlock();
        delete t;
    }

    Lockable& lockable;
};

struct SurfaceStackSurfaceCollection : public ms::SurfaceCollection
{
public:
    void invoke_for_each_surface(ms::SurfaceEnumerator& f)
    {
        std::for_each(surfaces.begin(), surfaces.end(), std::ref(f));
    }

    std::set<std::shared_ptr<ms::Surface>> surfaces;
};
}

ms::SurfaceStack::SurfaceStack(mc::BufferBundleFactory* bb_factory) : buffer_bundle_factory(bb_factory)
{
    assert(buffer_bundle_factory);
}

void ms::SurfaceStack::lock()
{
    guard.lock();
}

void ms::SurfaceStack::unlock()
{
    guard.unlock();
}

bool ms::SurfaceStack::try_lock()
{
    return guard.try_lock();
}

std::shared_ptr<ms::SurfaceCollection> ms::SurfaceStack::get_surfaces_in(geometry::Rectangle const& /*display_area*/)
{
    // TODO: Ensure locking with the help of a customer deleter here.
    LockGuardDeleter<std::mutex> lgd(guard);
    SurfaceStackSurfaceCollection* view = new SurfaceStackSurfaceCollection();
    view->surfaces = surfaces;
    return std::shared_ptr<ms::SurfaceCollection>(view, lgd);
}

std::weak_ptr<ms::Surface> ms::SurfaceStack::create_surface(const ms::SurfaceCreationParameters& params)
{
    std::lock_guard<std::mutex> lg(guard);
    
    std::shared_ptr<ms::Surface> surface(
        new ms::Surface(
            params,
            std::dynamic_pointer_cast<mc::BufferTextureBinder>(buffer_bundle_factory->create_buffer_bundle(
                params.width,
                params.height,
                mc::PixelFormat::rgba_8888))));

    surfaces.insert(surface);
    return surface;
}

void ms::SurfaceStack::destroy_surface(std::weak_ptr<ms::Surface> /*surface*/)
{
}

