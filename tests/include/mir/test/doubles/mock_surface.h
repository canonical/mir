/*
 * Copyright © 2013-2014 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_MOCK_SURFACE_H_
#define MIR_TEST_DOUBLES_MOCK_SURFACE_H_

#include "src/server/scene/basic_surface.h"
#include "src/server/report/null_report_factory.h"
#include "mock_buffer_stream.h"

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

struct MockSurface : public scene::BasicSurface
{
    MockSurface() :
        scene::BasicSurface(
            {},
            {{},{}},
            mir_pointer_unconfined,
            { { std::make_shared<testing::NiceMock<MockBufferStream>>(), {0, 0}, {} } },
            {},
            mir::report::null_scene_report())
    {
        ON_CALL(*this, primary_buffer_stream())
            .WillByDefault(testing::Invoke([this]
                { return scene::BasicSurface::primary_buffer_stream(); }));
    }

    ~MockSurface() noexcept {}

    MOCK_METHOD0(hide, void());
    MOCK_METHOD0(show, void());
    MOCK_CONST_METHOD0(visible, bool());

    MOCK_METHOD0(force_requests_to_complete, void());
    MOCK_METHOD0(advance_client_buffer, std::shared_ptr<graphics::Buffer>());

    MOCK_CONST_METHOD0(size, geometry::Size());
    MOCK_CONST_METHOD0(pixel_format, MirPixelFormat());

    MOCK_METHOD2(configure, int(MirWindowAttrib, int));
    MOCK_METHOD1(add_observer, void(std::shared_ptr<scene::SurfaceObserver> const&));
    MOCK_METHOD1(remove_observer, void(std::weak_ptr<scene::SurfaceObserver> const&));
    MOCK_METHOD1(consume, void(MirEvent const*));

    MOCK_CONST_METHOD0(primary_buffer_stream, std::shared_ptr<frontend::BufferStream>());
    MOCK_METHOD1(set_streams, void(std::list<scene::StreamInfo> const&));

    MOCK_CONST_METHOD0(session, std::experimental::optional<std::shared_ptr<scene::Session>>());
    MOCK_METHOD1(set_session, void(std::shared_ptr<scene::Session> const&));

    std::shared_ptr<MockBufferStream> const stream;
};

}
}
} // namespace mir

#endif // MIR_TEST_DOUBLES_MOCK_SURFACE_H_
