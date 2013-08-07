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

#ifndef MIR_TEST_DOUBLES_MOCK_SURFACE_H_
#define MIR_TEST_DOUBLES_MOCK_SURFACE_H_

#include "mir/shell/surface.h"

#include "mir/shell/surface_creation_parameters.h"
#include "null_event_sink.h"
#include "null_surface_configurator.h"

#include <gmock/gmock.h>

#include <memory>

namespace mir
{
namespace test
{
namespace doubles
{

struct MockSurface : public shell::Surface
{
    MockSurface(std::shared_ptr<shell::SurfaceBuilder> const& builder) :
        shell::Surface(builder, std::make_shared<NullSurfaceConfigurator>(), shell::a_surface(), 
            frontend::SurfaceId{}, std::make_shared<NullEventSink>())
    {
    }

    ~MockSurface() noexcept {}

    MOCK_METHOD0(hide, void());
    MOCK_METHOD0(show, void());
    MOCK_METHOD0(visible, bool());
    MOCK_METHOD1(raise, void(std::shared_ptr<shell::SurfaceController> const&));

    MOCK_METHOD0(destroy, void());
    MOCK_METHOD0(force_requests_to_complete, void());
    MOCK_METHOD0(advance_client_buffer, std::shared_ptr<graphics::Buffer>());

    MOCK_CONST_METHOD0(name, std::string());
    MOCK_CONST_METHOD0(size, geometry::Size());
    MOCK_CONST_METHOD0(pixel_format, geometry::PixelFormat());

    MOCK_CONST_METHOD0(supports_input, bool());
    MOCK_CONST_METHOD0(client_input_fd, int());

    MOCK_METHOD2(configure, int(MirSurfaceAttrib, int));
    MOCK_METHOD1(take_input_focus, void(std::shared_ptr<shell::InputTargeter> const&));
};

}
}
} // namespace mir

#endif // MIR_TEST_DOUBLES_MOCK_SURFACE_H_
