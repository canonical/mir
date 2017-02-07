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
#include "mir/test/doubles/null_client_buffer.h"
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

    void set_event_handler(MirWindowEventCallback cb, void* ctx) override
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
    MirWindowEventCallback event_handler;
    void* event_context;
};

struct MockHostSurface : mgn::HostSurface
{
    MOCK_METHOD0(egl_native_window, EGLNativeWindowType());
    MOCK_METHOD2(set_event_handler, void(MirWindowEventCallback, void*));
    MOCK_METHOD1(apply_spec, void(mgn::HostSurfaceSpec&));
};

struct MockNestedChain : mgn::HostChain
{
    MOCK_METHOD1(submit_buffer, void(mgn::NativeBuffer&));
    MOCK_METHOD0(handle, MirPresentationChain*());
    MOCK_METHOD1(set_submission_mode, void(mgn::SubmissionMode));
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    MOCK_CONST_METHOD0(rs, MirRenderSurface*());
#pragma GCC diagnostic pop
};

struct MockNestedStream : mgn::HostStream
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    MOCK_CONST_METHOD0(rs, MirRenderSurface*());
#pragma GCC diagnostic pop
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

    mtd::NullClientBuffer mutable null_buffer;
    MirBuffer* client_handle() const override { return reinterpret_cast<::MirBuffer*>(&null_buffer); }
    void sync(MirBufferAccess, std::chrono::nanoseconds) override {}
    std::unique_ptr<mgn::GraphicsRegion> get_graphics_region() override { return nullptr; }
    geom::Size size() const override { return {}; }
    MirPixelFormat format() const override { return mir_pixel_format_invalid; }
    void on_ownership_notification(std::function<void()> const& f) override { fn = f; }
    MirBufferPackage* package() const override { return nullptr; }
    mir::Fd fence() const override { return mir::Fd{mir::Fd::invalid}; }
    void set_fence(mir::Fd) override {}
    void available(MirBuffer*) override {}
    std::tuple<EGLenum, EGLClientBuffer, EGLint*> egl_image_creation_hints() const override
    {
        return std::tuple<EGLenum, EGLClientBuffer, EGLint*>{};
    }
    void trigger()
    {
        if (fn) fn();
    }
    std::function<void()> fn;
};

struct MockNestedBuffer : StubNestedBuffer
{
    MOCK_METHOD1(on_ownership_notification, void(std::function<void()> const&));
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

    auto create_display_buffer(
        std::shared_ptr<mgn::HostConnection> const& connection, mgn::PassthroughOption option)
    {
        return std::make_shared<mgnd::DisplayBuffer>(egl_display, output, connection, option);
    }

    auto create_display_buffer(std::shared_ptr<mgn::HostConnection> const& connection)
    {
        return create_display_buffer(connection, mgn::PassthroughOption::enabled);
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

TEST_F(NestedDisplayBuffer, respects_passthrough_option)
{
    StubNestedBuffer nested_buffer; 
    mg::RenderableList optimizable_list =
        { std::make_shared<mtd::StubRenderable>(mt::fake_shared(nested_buffer), rectangle) };

    auto enabled_display_buffer = create_display_buffer(host_connection, mgn::PassthroughOption::enabled);
    auto disabled_display_buffer = create_display_buffer(host_connection, mgn::PassthroughOption::disabled);

    EXPECT_TRUE(enabled_display_buffer->overlay(optimizable_list));
    EXPECT_FALSE(disabled_display_buffer->overlay(optimizable_list));
}

TEST_F(NestedDisplayBuffer, creates_stream_and_chain_for_passthrough)
{
    NiceMock<MockHostSurface> mock_host_surface;
    mtd::MockHostConnection mock_host_connection;
    NiceMock<MockNestedBuffer> nested_buffer; 
    mg::RenderableList list =
        { std::make_shared<mtd::StubRenderable>(mt::fake_shared(nested_buffer), rectangle) };

    auto mock_stream = std::make_unique<NiceMock<MockNestedStream>>();
    auto mock_chain = std::make_unique<NiceMock<MockNestedChain>>();
    EXPECT_CALL(nested_buffer, on_ownership_notification(_));
    EXPECT_CALL(*mock_chain, submit_buffer(Ref(nested_buffer)));

    EXPECT_CALL(mock_host_connection, create_surface(_,_,_,_,_))
        .WillOnce(Return(mt::fake_shared(mock_host_surface)));
    EXPECT_CALL(mock_host_connection, create_stream(_))
        .WillOnce(InvokeWithoutArgs([&] { return std::move(mock_stream); }));
    EXPECT_CALL(mock_host_connection, create_chain())
        .WillOnce(InvokeWithoutArgs([&] { return std::move(mock_chain); }));
    EXPECT_CALL(mock_host_surface, apply_spec(_));

    auto display_buffer = create_display_buffer(mt::fake_shared(mock_host_connection));
    EXPECT_TRUE(display_buffer->overlay(list));
    Mock::VerifyAndClearExpectations(&nested_buffer);
}

TEST_F(NestedDisplayBuffer, accepts_double_submissions_of_same_buffer_after_buffer_triggered)
{
    StubNestedBuffer nested_buffer1; 
    StubNestedBuffer nested_buffer2; 
    mg::RenderableList list1 =
        { std::make_shared<mtd::StubRenderable>(mt::fake_shared(nested_buffer1), rectangle) };
    mg::RenderableList list2 =
        { std::make_shared<mtd::StubRenderable>(mt::fake_shared(nested_buffer2), rectangle) };

    auto display_buffer = create_display_buffer(host_connection);
    EXPECT_TRUE(display_buffer->overlay(list1));
    EXPECT_TRUE(display_buffer->overlay(list2));
    nested_buffer1.trigger();
    EXPECT_TRUE(display_buffer->overlay(list1));
}

TEST_F(NestedDisplayBuffer, throws_on_interleaving_without_trigger)
{
    StubNestedBuffer nested_buffer1; 
    StubNestedBuffer nested_buffer2; 
    mg::RenderableList list1 =
        { std::make_shared<mtd::StubRenderable>(mt::fake_shared(nested_buffer1), rectangle) };
    mg::RenderableList list2 =
        { std::make_shared<mtd::StubRenderable>(mt::fake_shared(nested_buffer2), rectangle) };

    auto display_buffer = create_display_buffer(host_connection);
    EXPECT_TRUE(display_buffer->overlay(list1));
    EXPECT_TRUE(display_buffer->overlay(list2));
    EXPECT_THROW({
        EXPECT_TRUE(display_buffer->overlay(list1));
    }, std::logic_error);
}

TEST_F(NestedDisplayBuffer, holds_buffer_until_host_says_its_done_using_it)
{
    auto nested_buffer = std::make_shared<StubNestedBuffer>();
    mg::RenderableList list =
        { std::make_shared<mtd::StubRenderable>(nested_buffer, rectangle) };

    auto display_buffer = create_display_buffer(host_connection);

    auto use_count = nested_buffer.use_count(); 
    EXPECT_TRUE(display_buffer->overlay(list));
    EXPECT_THAT(nested_buffer.use_count(), Eq(use_count + 1));
    nested_buffer->trigger();
    EXPECT_THAT(nested_buffer.use_count(), Eq(use_count));
}

TEST_F(NestedDisplayBuffer, toggles_back_to_gl)
{
    NiceMock<MockHostSurface> mock_host_surface;
    mtd::StubHostConnection host_connection(mt::fake_shared(mock_host_surface));

    StubNestedBuffer nested_buffer; 
    mg::RenderableList list =
        { std::make_shared<mtd::StubRenderable>(mt::fake_shared(nested_buffer), rectangle) };

    InSequence seq;
    EXPECT_CALL(mock_host_surface, apply_spec(_));
    EXPECT_CALL(mock_egl, eglSwapBuffers(_,_));
    EXPECT_CALL(mock_host_surface, apply_spec(_));

    auto display_buffer = create_display_buffer(mt::fake_shared(host_connection));
    EXPECT_TRUE(display_buffer->overlay(list));
    display_buffer->swap_buffers();
}

//If we don't release the HostChain, then one of the client's buffers will get
//reserved in the host, but not shown onscreen. This will degrade the client's
//available buffers by one. Releasing the chain will result in that buffer
//getting returned to the client.
TEST_F(NestedDisplayBuffer, recreates_chain)
{
    int fake_chain_handle = 4;
    NiceMock<MockHostSurface> mock_host_surface;
    mtd::MockHostConnection mock_host_connection;
    NiceMock<MockNestedBuffer> nested_buffer; 
    mg::RenderableList list =
        { std::make_shared<mtd::StubRenderable>(mt::fake_shared(nested_buffer), rectangle) };

    auto mock_stream = std::make_unique<NiceMock<MockNestedStream>>();
    auto mock_chain = std::make_unique<NiceMock<MockNestedChain>>();
    auto mock_chain2 = std::make_unique<NiceMock<MockNestedChain>>();
    EXPECT_CALL(nested_buffer, on_ownership_notification(_))
        .Times(2);
    EXPECT_CALL(*mock_chain, submit_buffer(Ref(nested_buffer)));
    ON_CALL(*mock_chain, handle())
        .WillByDefault(Return(reinterpret_cast<MirPresentationChain*>(&fake_chain_handle)));
    EXPECT_CALL(*mock_chain2, submit_buffer(Ref(nested_buffer)));

    EXPECT_CALL(mock_host_connection, create_surface(_,_,_,_,_))
        .WillOnce(Return(mt::fake_shared(mock_host_surface)));
    EXPECT_CALL(mock_host_connection, create_stream(_))
        .WillOnce(InvokeWithoutArgs([&] { return std::move(mock_stream); }));
    EXPECT_CALL(mock_host_connection, create_chain())
        .Times(2)
        .WillOnce(InvokeWithoutArgs([&] { return std::move(mock_chain); }))
        .WillOnce(InvokeWithoutArgs([&] { return std::move(mock_chain2); }));

    auto display_buffer = create_display_buffer(mt::fake_shared(mock_host_connection));
    EXPECT_TRUE(display_buffer->overlay(list));
    display_buffer->swap_buffers();
    EXPECT_TRUE(display_buffer->overlay(list));
    Mock::VerifyAndClearExpectations(&nested_buffer);
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
    EXPECT_TRUE(display_buffer->overlay(list));
    EXPECT_TRUE(display_buffer->overlay(list));
    display_buffer->swap_buffers();
    display_buffer->swap_buffers();
}

TEST_F(NestedDisplayBuffer, rejects_list_containing_unknown_buffers)
{
    mtd::StubBuffer foreign_buffer(std::make_shared<FunkyBuffer>());
 
    mg::RenderableList list =
        { std::make_shared<mtd::StubRenderable>(mt::fake_shared(foreign_buffer), rectangle) };

    auto display_buffer = create_display_buffer(host_connection);
    EXPECT_FALSE(display_buffer->overlay(list));
}

TEST_F(NestedDisplayBuffer, rejects_list_containing_rotated_buffers)
{
    StubNestedBuffer nested_buffer; 
    mg::RenderableList list =
        { std::make_shared<mtd::StubTransformedRenderable>(mt::fake_shared(nested_buffer), rectangle) };

    auto display_buffer = create_display_buffer(host_connection);
    EXPECT_FALSE(display_buffer->overlay(list));
}

TEST_F(NestedDisplayBuffer, accepts_list_with_unshown_unknown_buffers)
{
    StubNestedBuffer nested_buffer; 
    mtd::StubBuffer foreign_buffer(std::make_shared<FunkyBuffer>());
    mg::RenderableList list =
        { std::make_shared<mtd::StubRenderable>(mt::fake_shared(foreign_buffer), rectangle),
          std::make_shared<mtd::StubRenderable>(mt::fake_shared(nested_buffer), rectangle) };

    auto display_buffer = create_display_buffer(host_connection);
    EXPECT_TRUE(display_buffer->overlay(list));
}

TEST_F(NestedDisplayBuffer, rejects_empty_list)
{
    mg::RenderableList list;
    auto display_buffer = create_display_buffer(host_connection);
    EXPECT_FALSE(display_buffer->overlay(list));
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
    EXPECT_FALSE(display_buffer->overlay(list));
}

/* Once we have synchronous MirWindow scene updates, we can probably
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
    EXPECT_FALSE(display_buffer->overlay(list));
}

TEST_F(NestedDisplayBuffer, accepts_list_containing_multiple_renderables_with_fullscreen_on_top)
{
    StubNestedBuffer nested_buffer; 
    geom::Rectangle small_rect { {0, 0}, { 5, 5 }};
    mg::RenderableList list = {
        std::make_shared<mtd::StubRenderable>(mt::fake_shared(nested_buffer), small_rect),
        std::make_shared<mtd::StubRenderable>(mt::fake_shared(nested_buffer), rectangle) };

    auto display_buffer = create_display_buffer(host_connection);
    EXPECT_TRUE(display_buffer->overlay(list));
}

//bit subtle, but if a swapinterval 0 nested-client is submitting its buffers to 
//a swapinterval-1 host chain, then its spare buffers will end up being owned by
//the host, and stop swapinterval 0 from working.
TEST_F(NestedDisplayBuffer, coordinates_clients_interval_setting_with_host)
{
    NiceMock<MockHostSurface> mock_host_surface;
    auto mock_chain = std::make_unique<NiceMock<MockNestedChain>>();
    auto mock_stream = std::make_unique<NiceMock<MockNestedStream>>();

    Sequence seq;
    EXPECT_CALL(*mock_chain, set_submission_mode(mgn::SubmissionMode::dropping))
        .InSequence(seq);
    EXPECT_CALL(*mock_chain, set_submission_mode(mgn::SubmissionMode::queueing))
        .InSequence(seq);
    EXPECT_CALL(*mock_chain, set_submission_mode(mgn::SubmissionMode::dropping))
        .InSequence(seq);

    mtd::MockHostConnection mock_host_connection;
    EXPECT_CALL(mock_host_connection, create_surface(_,_,_,_,_))
        .WillOnce(Return(mt::fake_shared(mock_host_surface)));

    EXPECT_CALL(mock_host_connection, create_stream(_))
        .WillOnce(InvokeWithoutArgs([&] { return std::move(mock_stream); }));
    EXPECT_CALL(mock_host_connection, create_chain())
        .WillOnce(InvokeWithoutArgs([&] { return std::move(mock_chain); }));
    auto display_buffer = create_display_buffer(mt::fake_shared(mock_host_connection));

    auto nested_buffer1 = std::make_shared<StubNestedBuffer>();
    auto nested_buffer2 = std::make_shared<StubNestedBuffer>();
    auto nested_buffer3 = std::make_shared<StubNestedBuffer>();
    EXPECT_TRUE(display_buffer->overlay(
      { std::make_shared<mtd::IntervalZeroRenderable>(std::make_shared<StubNestedBuffer>(), rectangle) }));
    EXPECT_TRUE(display_buffer->overlay(
      { std::make_shared<mtd::StubRenderable>(std::make_shared<StubNestedBuffer>(), rectangle) }));
    EXPECT_TRUE(display_buffer->overlay(
      { std::make_shared<mtd::IntervalZeroRenderable>(std::make_shared<StubNestedBuffer>(), rectangle) }));
}
