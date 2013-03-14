/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */


#ifndef PROXY_SURFACE_H_
#define PROXY_SURFACE_H_

#include "mir/frontend/surface.h"
#include "mir/surfaces/surface.h"

#include <functional>

namespace mir
{
namespace frontend
{
struct SurfaceCreationParameters;
}

namespace surfaces
{
class SurfaceStackModel;

class BasicProxySurface : public frontend::Surface
{
public:

    explicit BasicProxySurface(std::weak_ptr<mir::surfaces::Surface> const& surface);
    BasicProxySurface(std::weak_ptr<mir::surfaces::Surface> const& surface, std::function<void(std::weak_ptr<mir::surfaces::Surface> const&)> const& deleter);
    ~BasicProxySurface();

    void hide();

    void show();

    void destroy();

    void shutdown();

    geometry::Size size() const;

    geometry::PixelFormat pixel_format() const;

    void advance_client_buffer();

    std::shared_ptr<compositor::Buffer> client_buffer() const;

private:
    std::weak_ptr<mir::surfaces::Surface> const surface;
    std::function<void(std::weak_ptr<mir::surfaces::Surface> const&)> const deleter;
};

}
}

#endif /* PROXY_SURFACE_H_ */
