/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_TEST_DOUBLES_MOCK_SURFACE_H_
#define MIR_TEST_DOUBLES_MOCK_SURFACE_H_

#include "src/server/scene/basic_surface.h"
#include "src/server/report/null_report_factory.h"
#include "mock_buffer_stream.h"
#include "mir/test/doubles/fake_display_configuration_observer_registrar.h"

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
            {},
            {},
            {{},{}},
            mir_pointer_unconfined,
            { { std::make_shared<testing::NiceMock<MockBufferStream>>(), {0, 0}} },
            {},
            mir::report::null_scene_report(),
            std::make_shared<FakeDisplayConfigurationObserverRegistrar>())
    {
        ON_CALL(*this, primary_buffer_stream())
            .WillByDefault(testing::Invoke([this]
                { return scene::BasicSurface::primary_buffer_stream(); }));
        ON_CALL(*this, parent())
            .WillByDefault(testing::Return(nullptr));
        ON_CALL(*this, set_focus_state(testing::_))
            .WillByDefault(testing::Invoke([this](MirWindowFocusState focus_state)
                {
                    BasicSurface::set_focus_state(focus_state);
                }));
        ON_CALL(*this, move_to(testing::_))
            .WillByDefault(testing::Invoke([this](geometry::Point const& top_left)
               {
                    BasicSurface::move_to(top_left);
               }));
    }

    ~MockSurface() noexcept {}

    MOCK_METHOD(MirWindowType, type, (), (const));
    MOCK_METHOD(void, hide, ());
    MOCK_METHOD(void, show, ());
    MOCK_METHOD(bool, visible, (), (const));
    MOCK_METHOD(void, move_to, (geometry::Point const& ));

    MOCK_METHOD(void, force_requests_to_complete, ());
    MOCK_METHOD(std::shared_ptr<graphics::Buffer>, advance_client_buffer, ());

    MOCK_METHOD(geometry::Size, size, (), (const));
    MOCK_METHOD(MirPixelFormat, pixel_format, (), (const));

    MOCK_METHOD(void, request_client_surface_close, ());
    MOCK_METHOD(std::shared_ptr<scene::Surface>, parent, (), (const));
    MOCK_METHOD(int, configure, (MirWindowAttrib, int));
    MOCK_METHOD(void, register_interest, (std::weak_ptr<scene::SurfaceObserver> const&));
    MOCK_METHOD(void, unregister_interest, (scene::SurfaceObserver const&));
    MOCK_METHOD(void, consume, (std::shared_ptr<MirEvent const> const& event));

    MOCK_METHOD(std::shared_ptr<frontend::BufferStream>, primary_buffer_stream, (), (const));
    MOCK_METHOD(void, set_streams, (std::list<scene::StreamInfo> const&));

    MOCK_METHOD(void, set_focus_state, (MirWindowFocusState));
    MOCK_METHOD(std::string, application_id, (), (const));
    MOCK_METHOD(std::weak_ptr<scene::Session>, session, (), (const));

    std::shared_ptr<MockBufferStream> const stream;
};

}
}
} // namespace mir

#endif // MIR_TEST_DOUBLES_MOCK_SURFACE_H_
