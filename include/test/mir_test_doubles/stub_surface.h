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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_STUB_SURFACE_H_
#define MIR_TEST_DOUBLES_STUB_SURFACE_H_

#include "mir/frontend/surface.h"

namespace mir
{
namespace test
{
namespace doubles
{

class StubSurface : public frontend::ClientTrackingSurface
{
public:
    virtual ~StubSurface() = default;

    void hide() {}
    void show() {}
    void destroy() {}
    void force_requests_to_complete() {}

    geometry::Size size() const
    {
        return geometry::Size();
    }
    geometry::PixelFormat pixel_format() const
    {
        return geometry::PixelFormat();
    }

    std::shared_ptr<graphics::Buffer> advance_client_buffer()
    {
        return std::shared_ptr<graphics::Buffer>();
    }

    virtual int configure(MirSurfaceAttrib, int)
    {
        return 0;
    }

    virtual bool supports_input() const
    {
        return false;
    }

    virtual int client_input_fd() const
    {
        return 0;
    }
};

}
}
} // namespace mir

#endif // MIR_TEST_DOUBLES_STUB_SURFACE_H_
