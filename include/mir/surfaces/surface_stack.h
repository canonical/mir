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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_SURFACES_SURFACESTACK_H_
#define MIR_SURFACES_SURFACESTACK_H_

#include "scenegraph.h"
#include "surface_stack_model.h"
#include "mir/thread/all.h"

#include <memory>
#include <set>

namespace mir
{
namespace compositor
{
class BufferBundleFactory;
}

namespace surfaces
{
class Surface;
class SurfaceCreationParameters;

class SurfaceStack : public Scenegraph,
                     public SurfaceStackModel
{
 public:
    explicit SurfaceStack(compositor::BufferBundleFactory* bb_factory);
    virtual ~SurfaceStack() {}

//    // Make this a model of Lockable.
//    void lock();
//    void unlock();
//    bool try_lock();
    
    // From Scenegraph
    virtual std::shared_ptr<SurfaceCollection> get_surfaces_in(geometry::Rectangle const& display_area);
    
    // From SurfaceStackModel
    virtual std::weak_ptr<Surface> create_surface(const SurfaceCreationParameters& params);

    virtual void destroy_surface(std::weak_ptr<Surface> surface);

 private:
    SurfaceStack(const SurfaceStack&) = delete;
    SurfaceStack& operator=(const SurfaceStack&) = delete;
    std::mutex guard;
    compositor::BufferBundleFactory* const buffer_bundle_factory;
    std::set<std::shared_ptr<Surface>> surfaces;
};

}
}

#endif /* MIR_SURFACES_SURFACESTACK_H_ */
