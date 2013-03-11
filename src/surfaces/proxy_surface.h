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

#include "mir/shell/surface.h"
#include "mir/surfaces/surface.h"

namespace mir
{
namespace shell
{
class SurfaceCreationParameters;
}
namespace input
{
class InputChannel;
}

namespace surfaces
{
class SurfaceStackModel;

class BasicProxySurface : public shell::Surface
{
public:

    explicit BasicProxySurface(std::weak_ptr<mir::surfaces::Surface> const& surface,
                               std::shared_ptr<input::InputChannel> const& input_channel);

    void hide();

    void show();

    void destroy();

    void shutdown();

    geometry::Size size() const;

    geometry::PixelFormat pixel_format() const;

    void advance_client_buffer();

    std::shared_ptr<compositor::Buffer> client_buffer() const;

    bool supports_input() const;
    int client_input_fd() const;

protected:
    void destroy_surface(SurfaceStackModel* const surface_stack) const;

private:
    std::weak_ptr<mir::surfaces::Surface> const surface;
    std::shared_ptr<input::InputChannel> const input_channel;
};

class ProxySurface : public BasicProxySurface
{
public:
    ProxySurface(
        SurfaceStackModel* const surface_stack_,
        std::shared_ptr<input::InputChannel> const& input_channel,
        shell::SurfaceCreationParameters const& params);

    void destroy();

    ~ProxySurface();

private:
    SurfaceStackModel* const surface_stack;
};

}
}

#endif /* PROXY_SURFACE_H_ */
