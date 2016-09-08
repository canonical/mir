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
#include "src/server/graphics/nested/host_surface_spec.h"

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
struct FunkyBuffer : mg::NativeBuffer
{
};

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

    void apply_spec(mgn::HostSurfaceSpec&) override
    {
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
    MOCK_METHOD1(apply_spec, void(mgn::HostSurfaceSpec&));
};

struct MockNestedChain : mgn::HostChain
{
    MOCK_METHOD1(submit_buffer, void(mgn::NativeBuffer&));
    MOCK_CONST_METHOD0(handle, MirPresentationChain*());
};

struct MockNestedStream : mgn::HostStream
{
    MOCK_CONST_METHOD0(handle, MirBufferStream*());
    MOCK_CONST_METHOD0(egl_native_window, EGLNativeWindowType());
};

struct StubNestedBuffer :
    mtd::StubBuffer,
    mgn::NativeBuffer,
    std::enable_shared_from_this<StubNestedBuffer>
{
    std::shared_ptr<mg::NativeBuffer> native_buffer_handle() const override
    {
        return std::const_pointer_cast<StubNestedBuffer>(shared_from_this());
    }
    MirBuffer* client_handle() const override { return nullptr; }
    void sync(MirBufferAccess, std::chrono::nanoseconds) override {}
    MirNativeBuffer* get_native_handle() override { return nullptr; }
    MirGraphicsRegion get_graphics_region() override { return MirGraphicsRegion{}; }
    geom::Size size() const override { return {}; }
    MirPixelFormat format() const override { return mir_pixel_format_invalid; }
};

struct NestedDisplayBuffer : Test
{
    NestedDisplayBuffer()
    {
        output.top_left = rectangle.top_left;
        output.current_mode_index = 0;
        output.orientation = mir_orientation_normal;
        output.current_format = mir_pixel_format_abgr_8888;
        output.modes = { { rectangle.size, 55.0f } };
    }

    auto create_display_buffer(std::shared_ptr<mgn::HostConnection> const& connection)
    {
        return std::make_shared<mgnd::DisplayBuffer>(egl_display, output, connection);
    }

    mir::geometry::Rectangle const rectangle { {0,0}, {1024, 768} };
    NiceMock<mtd::MockEGL> mock_egl;
    mgnd::EGLDisplayHandle egl_display{nullptr, std::make_shared<mtd::StubGLConfig>()};
    EventHostSurface host_surface;
    std::shared_ptr<mtd::StubHostConnection> host_connection =
        std::make_shared<mtd::StubHostConnection>(mt::fake_shared(host_surface));
    mg::DisplayConfigurationOutput output;
};

}

// Regression test for LP: #1612012
// This test is trying to reproduce a race, so it's not deterministic, but
// in practice the reproduction rate is very close to 100%.
TEST_F(NestedDisplayBuffer, event_dispatch_does_not_race_with_destruction)
{
    auto display_buffer = create_display_buffer(host_connection);
    std::thread t{
        [&]
        {
            for (int i = 0; i < 100; ++i)
                host_surface.emit_input_event();
        }};

    display_buffer.reset();
    t.join();
}


TEST_F(NestedDisplayBuffer, creates_stream_and_chain_for_passthrough)
{
    NiceMock<MockHostSurface> mock_host_surface;
    mtd::MockHostConnection mock_host_connection;

    StubNestedBuffer nested_buffer; 
    mg::RenderableList list =
        { std::make_shared<mtd::StubRenderable>(mt::fake_shared(nested_buffer), rectangle) };

    auto mock_stream = std::make_unique<NiceMock<MockNestedStream>>();
    auto mock_chain = std::make_unique<NiceMock<MockNestedChain>>();
    EXPECT_CALL(*mock_chain, submit_buffer(Ref(nested_buffer)));

    EXPECT_CALL(mock_host_connection, create_surface(_,_,_,_,_))
        .WillOnce(Return(mt::fake_shared(mock_host_surface)));
    EXPECT_CALL(mock_host_connection, create_stream(_))
        .WillOnce(InvokeWithoutArgs([&] { return std::move(mock_stream); }));
    EXPECT_CALL(mock_host_connection, create_chain())
        .WillOnce(InvokeWithoutArgs([&] { return std::move(mock_chain); }));
    EXPECT_CALL(mock_host_surface, apply_spec(_));

    auto display_buffer = create_display_buffer(mt::fake_shared(mock_host_connection));
    EXPECT_TRUE(display_buffer->post_renderables_if_optimizable(list));
}

TEST_F(NestedDisplayBuffer, toggles_back_to_gl)
{
    NiceMock<MockHostSurface> mock_host_surface;
    mtd::StubHostConnection host_connection(mt::fake_shared(mock_host_surface));

    StubNestedBuffer nested_buffer; 
    mg::RenderableList list =
        { std::make_shared<mtd::StubRenderable>(mt::fake_shared(nested_buffer), rectangle) };

    EXPECT_CALL(mock_host_surface, apply_spec(_))
        .Times(2);

    auto display_buffer = create_display_buffer(mt::fake_shared(host_connection));
    EXPECT_TRUE(display_buffer->post_renderables_if_optimizable(list));
    display_buffer->swap_buffers();
}

TEST_F(NestedDisplayBuffer, only_applies_spec_once_per_mode_toggle)
{
    NiceMock<MockHostSurface> mock_host_surface;
    mtd::StubHostConnection host_connection(mt::fake_shared(mock_host_surface));

    StubNestedBuffer nested_buffer; 
    mg::RenderableList list =
          { std::make_shared<mtd::StubRenderable>(mt::fake_shared(nested_buffer), rectangle) };

    auto display_buffer = create_display_buffer(mt::fake_shared(host_connection));

    EXPECT_CALL(mock_host_surface, apply_spec(_))
        .Times(2);
    EXPECT_TRUE(display_buffer->post_renderables_if_optimizable(list));
    EXPECT_TRUE(display_buffer->post_renderables_if_optimizable(list));
    display_buffer->swap_buffers();
    display_buffer->swap_buffers();
}

TEST_F(NestedDisplayBuffer, rejects_list_containing_unknown_buffers)
{
    mtd::StubBuffer foreign_buffer(std::make_shared<FunkyBuffer>());
 
    mg::RenderableList list =
        { std::make_shared<mtd::StubRenderable>(mt::fake_shared(foreign_buffer), rectangle) };

    auto display_buffer = create_display_buffer(host_connection);
    EXPECT_FALSE(display_buffer->post_renderables_if_optimizable(list));
}

TEST_F(NestedDisplayBuffer, rejects_list_containing_rotated_buffers)
{
    StubNestedBuffer nested_buffer; 
    mg::RenderableList list =
        { std::make_shared<mtd::StubTransformedRenderable>(mt::fake_shared(nested_buffer), rectangle) };

    auto display_buffer = create_display_buffer(host_connection);
    EXPECT_FALSE(display_buffer->post_renderables_if_optimizable(list));
}

TEST_F(NestedDisplayBuffer, accepts_list_with_unshown_unknown_buffers)
{
    StubNestedBuffer nested_buffer; 
    mtd::StubBuffer foreign_buffer(std::make_shared<FunkyBuffer>());
    mg::RenderableList list =
        { std::make_shared<mtd::StubRenderable>(mt::fake_shared(foreign_buffer), rectangle),
          std::make_shared<mtd::StubRenderable>(mt::fake_shared(nested_buffer), rectangle) };

    auto display_buffer = create_display_buffer(host_connection);
    EXPECT_TRUE(display_buffer->post_renderables_if_optimizable(list));
}

TEST_F(NestedDisplayBuffer, rejects_empty_list)
{
    mg::RenderableList list;
    auto display_buffer = create_display_buffer(host_connection);
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

    auto display_buffer = create_display_buffer(host_connection);
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

    auto display_buffer = create_display_buffer(host_connection);
    EXPECT_FALSE(display_buffer->post_renderables_if_optimizable(list));
}

TEST_F(NestedDisplayBuffer, accepts_list_containing_multiple_renderables_with_fullscreen_on_top)
{
    StubNestedBuffer nested_buffer; 
    geom::Rectangle small_rect { {0, 0}, { 5, 5 }};
    mg::RenderableList list = {
        std::make_shared<mtd::StubRenderable>(mt::fake_shared(nested_buffer), small_rect),
        std::make_shared<mtd::StubRenderable>(mt::fake_shared(nested_buffer), rectangle) };

    auto display_buffer = create_display_buffer(host_connection);
    EXPECT_TRUE(display_buffer->post_renderables_if_optimizable(list));
}
