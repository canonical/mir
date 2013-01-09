/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 */

#ifndef MIR_SESSIONS_SURFACE_ORGANISER_H_
#define MIR_SESSIONS_SURFACE_ORGANISER_H_

#include <memory>

namespace mir
{

namespace sessions
{
class SurfaceCreationParameters;
class Surface;

class SurfaceOrganiser
{
public:
    virtual ~SurfaceOrganiser() {}

    virtual std::shared_ptr<Surface> create_surface(const SurfaceCreationParameters& params) = 0;

    // TODO I think we can just call the Surface function
    virtual void hide_surface(std::shared_ptr<Surface> const& surface) = 0;
    // TODO I think we can just call the Surface function
    virtual void show_surface(std::shared_ptr<Surface> const& surface) = 0;

protected:
    SurfaceOrganiser() = default;
    SurfaceOrganiser(const SurfaceOrganiser&) = delete;
    SurfaceOrganiser& operator=(const SurfaceOrganiser&) = delete;
};
}
}

#endif // MIR_SESSIONS_SURFACE_ORGANISER_H_
