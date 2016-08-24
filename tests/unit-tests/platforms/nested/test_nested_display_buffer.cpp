/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "src/server/graphics/nested/display_buffer.h"
#include "src/server/graphics/nested/native_buffer.h"
#include "src/server/graphics/nested/host_stream.h"
#include "src/server/graphics/nested/host_chain.h"

#include "mir/events/event_builders.h"

#include "mir/test/doubles/mock_egl.h"
#include "mir/test/doubles/stub_gl_config.h"
#include "mir/test/doubles/stub_buffer.h"
#include "mir/test/doubles/stub_host_connection.h"
#include "mir/test/doubles/stub_renderable.h"
#include "mir/test/fake_shared.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mg = mir::graphics;
namespace mgn = mir::graphics::nested;
namespace mgnd = mgn::detail;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;
namespace geom = mir::geometry;
using namespace testing;

namespace
{

class EventHostSurface : public mgn::HostSurface
{
public:
    EGLNativeWindowType egl_native_window() override { return {}; }

    void set_event_handler(mir_surface_event_callback cb, void* ctx) override
    {
        std::lock_guard<std::mutex> lock{event_mutex};
        event_handler = cb;
        event_context = ctx;
    }

    void emit_input_event()
    {
        auto const ev = mir::events::make_event(
            MirInputDeviceId(), {}, std::vector<uint8_t>{},
            MirKeyboardAction(), 0, 0, MirInputEventModifiers());

        std::lock_guard<std::mutex> lock{event_mutex};
        // Normally we shouldn't call external code under lock, but, for our
        // test use case, doing so is safe and simplifies the test code.
        if (event_handler)
            event_handler(nullptr, ev.get(), event_context);
    }

private:
    std::mutex event_mutex;
    mir_surface_event_callback event_handler;
    void* event_context;
};

struct MockHostSurface : mgn::HostSurface
{
    MOCK_METHOD0(egl_native_window, EGLNativeWindowType());
    MOCK_METHOD2(set_event_handler, void(mir_surface_event_callback, void*));
    MOCK_METHOD1(set_content, void(int));
};

struct MockNestedChain : mgn::HostChain
{
    MOCK_METHOD1(submit_buffer, void(MirBuffer*));
};

struct StubNestedBuffer :
    mtd::StubBuffer,
    mg::NativeBuffer,
    std::enable_shared_from_this<StubNestedBuffer>
{
    int const fake_buffer = 15122235;
    MirBuffer* fake_mir_buffer = const_cast<MirBuffer*>(reinterpret_cast<MirBuffer const*>(&fake_mir_buffer));
    std::shared_ptr<mg::NativeBuffer> native_buffer_handle() const override
    {
        return std::const_pointer_cast<StubNestedBuffer>(shared_from_this());
    }

    MirBuffer* client_handle() const override
    {
        return fake_mir_buffer;
    }
};

struct NestedDisplayBuffer : Test
{
    auto create_display_buffer(std::shared_ptr<mgn::HostSurface> const& surface)
    {
        return std::make_shared<mgnd::DisplayBuffer>(
            egl_display,
            surface,
            rectangle,
            MirPixelFormat{},
            host_connection);
    }

    mir::geometry::Rectangle const rectangle { {0,0}, {1024, 768} };
    NiceMock<mtd::MockEGL> mock_egl;
    mgnd::EGLDisplayHandle egl_display{nullptr, std::make_shared<mtd::StubGLConfig>()};
    EventHostSurface host_surface;
    MockHostSurface mock_host_surface;
    
    std::shared_ptr<mtd::MockHostConnection> host_connection = std::make_shared<mtd::MockHostConnection>();
    
};

}

// Regression test for LP: #1612012
// This test is trying to reproduce a race, so it's not deterministic, but
// in practice the reproduction rate is very close to 100%.
TEST_F(NestedDisplayBuffer, event_dispatch_does_not_race_with_destruction)
{
    auto display_buffer = create_display_buffer(mt::fake_shared(host_surface));
    std::thread t{
        [&]
        {
            for (int i = 0; i < 100; ++i)
                host_surface.emit_input_event();
        }};

    display_buffer.reset();
    t.join();
}


TEST_F(NestedDisplayBuffer, creates_stream_for_passthrough)
{
    StubNestedBuffer nested_buffer; 
    mg::RenderableList list =
        { std::make_shared<mtd::StubRenderable>(mt::fake_shared(nested_buffer), rectangle) };

    auto mock_chain = std::make_shared<MockNestedChain>();

    EXPECT_CALL(*host_connection, create_chain())
        .WillOnce(Return(mock_chain));
    EXPECT_CALL(*mock_chain, submit_buffer(nested_buffer.fake_mir_buffer));
    EXPECT_CALL(mock_host_surface, set_content(_));

    auto display_buffer = create_display_buffer(mt::fake_shared(mock_host_surface));
    EXPECT_TRUE(display_buffer->post_renderables_if_optimizable(list));
}

TEST_F(NestedDisplayBuffer, rejects_list_containing_unknown_buffers)
{
    mtd::StubBuffer nested_buffer; 
    mg::RenderableList list =
        { std::make_shared<mtd::StubRenderable>(mt::fake_shared(nested_buffer), rectangle) };

    auto display_buffer = create_display_buffer(mt::fake_shared(mock_host_surface));
    EXPECT_FALSE(display_buffer->post_renderables_if_optimizable(list));
}

TEST_F(NestedDisplayBuffer, rejects_list_containing_buffers_with_different_size_from_output)
{
    StubNestedBuffer nested_buffer; 
    geom::Size different_size{rectangle.size.width, rectangle.size.height + geom::DeltaY{1}};
    mg::RenderableList list = {
        std::make_shared<mtd::StubRenderable>(
            mt::fake_shared(nested_buffer),
            geom::Rectangle{rectangle.top_left, different_size})
    };

    auto display_buffer = create_display_buffer(mt::fake_shared(mock_host_surface));
    EXPECT_FALSE(display_buffer->post_renderables_if_optimizable(list));
}

/* Once we have synchronous MirSurface scene updates, we can probably
 * passthrough more than one renderable if needed
 */
TEST_F(NestedDisplayBuffer, rejects_list_containing_multiple_onscreen_renderables)
{
    StubNestedBuffer nested_buffer; 
    geom::Rectangle small_rect { {0, 0}, { 5, 5 }};
    mg::RenderableList list = {
        std::make_shared<mtd::StubRenderable>(mt::fake_shared(nested_buffer), rectangle),
        std::make_shared<mtd::StubRenderable>(mt::fake_shared(nested_buffer), small_rect) };

    auto display_buffer = create_display_buffer(mt::fake_shared(mock_host_surface));
    EXPECT_FALSE(display_buffer->post_renderables_if_optimizable(list));
}

TEST_F(NestedDisplayBuffer, accepts_list_containing_multiple_renderables_with_fullscreen_on_top)
{
    StubNestedBuffer nested_buffer; 
    geom::Rectangle small_rect { {0, 0}, { 5, 5 }};
    mg::RenderableList list = {
        std::make_shared<mtd::StubRenderable>(mt::fake_shared(nested_buffer), small_rect),
        std::make_shared<mtd::StubRenderable>(mt::fake_shared(nested_buffer), rectangle) };

    auto display_buffer = create_display_buffer(mt::fake_shared(mock_host_surface));
    EXPECT_TRUE(display_buffer->post_renderables_if_optimizable(list));
}
