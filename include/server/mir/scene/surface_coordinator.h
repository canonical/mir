/*
 * Copyright © 2013-14 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */


#ifndef MIR_SCENE_SURFACE_COORDINATOR_H_
#define MIR_SCENE_SURFACE_COORDINATOR_H_

#include <memory>

namespace mir
{
namespace shell
{
struct Session;
struct SurfaceCreationParameters;
}

namespace scene
{
using shell::Session;
class Surface;
class SurfaceObserver;

class SurfaceCoordinator
{
public:
    virtual std::shared_ptr<Surface> add_surface(
        shell::SurfaceCreationParameters const& params,
        Session* session,
        std::shared_ptr<SurfaceObserver> const& observer) = 0;

    virtual void raise(std::weak_ptr<Surface> const& surface) = 0;

    virtual void remove_surface(std::weak_ptr<Surface> const& surface) = 0;
protected:
    SurfaceCoordinator() = default;
    virtual ~SurfaceCoordinator() = default;
    SurfaceCoordinator(SurfaceCoordinator const&) = delete;
    SurfaceCoordinator& operator=(SurfaceCoordinator const&) = delete;
};
}
}


#endif /* MIR_SCENE_SURFACE_COORDINATOR_H_ */
