/*
 * Copyright © 2013 Canonical Ltd.
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


#ifndef MIR_SHELL_SURFACE_BUILDER_H_
#define MIR_SHELL_SURFACE_BUILDER_H_

#include <memory>

namespace mir
{
namespace surfaces { class Surface; }

namespace shell
{
class Session;
struct SurfaceCreationParameters;

class SurfaceBuilder
{
public:
    virtual std::weak_ptr<surfaces::Surface> create_surface(Session* session, SurfaceCreationParameters const& params) = 0;

    virtual void destroy_surface(std::weak_ptr<surfaces::Surface> const& surface) = 0;
protected:
    SurfaceBuilder() = default;
    virtual ~SurfaceBuilder() {} // should be "= default;" but that causes "noexcept" spread
    SurfaceBuilder(SurfaceBuilder const&) = delete;
    SurfaceBuilder& operator=(SurfaceBuilder const&) = delete;
};
}
}


#endif /* MIR_SHELL_SURFACE_BUILDER_H_ */
