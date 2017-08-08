/*
 * Copyright © 2013 Canonical Ltd.
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
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_MOCK_FRONTEND_SURFACE_H_
#define MIR_TEST_DOUBLES_MOCK_FRONTEND_SURFACE_H_

#include "mir/frontend/surface.h"

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{
struct MockFrontendSurface : public frontend::Surface
{
    MockFrontendSurface()
    {
        using namespace testing;
        ON_CALL(*this, client_size())
            .WillByDefault(Return(geometry::Size()));
        ON_CALL(*this, pixel_format())
            .WillByDefault(Return(MirPixelFormat()));
    }
    MockFrontendSurface(std::shared_ptr<graphics::Buffer> const&)
        : MockFrontendSurface()
    {}

    ~MockFrontendSurface() noexcept {}

    MOCK_METHOD0(destroy, void());
    MOCK_METHOD0(force_requests_to_complete, void());

    MOCK_CONST_METHOD0(client_size, geometry::Size());
    MOCK_CONST_METHOD0(pixel_format, MirPixelFormat());

    MOCK_METHOD1(set_cursor_image, void(std::shared_ptr<graphics::CursorImage> const&));
    MOCK_METHOD2(set_cursor_stream, void(std::shared_ptr<frontend::BufferStream> const&, geometry::Displacement const&));

    MOCK_METHOD2(configure, int(MirWindowAttrib, int));

    MOCK_CONST_METHOD1(query, int(MirWindowAttrib));

    MOCK_METHOD1(add_observer, void(std::shared_ptr<scene::SurfaceObserver> const&));
    MOCK_METHOD1(remove_observer, void(std::weak_ptr<scene::SurfaceObserver> const&));

    MOCK_CONST_METHOD0(primary_buffer_stream, std::shared_ptr<frontend::BufferStream>());    
};
}
}
} // namespace mir
#endif // MIR_TEST_DOUBLES_MOCK_FRONTEND_SURFACE_H_
