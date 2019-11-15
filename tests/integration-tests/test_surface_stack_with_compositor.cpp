/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/compositor/display_listener.h"
#include "mir/renderer/renderer_factory.h"
#include "mir/scene/surface_creation_parameters.h"
#include "src/server/report/null_report_factory.h"
#include "src/server/scene/surface_stack.h"
#include "src/server/scene/basic_surface.h"
#include "src/server/compositor/default_display_buffer_compositor_factory.h"
#include "src/server/compositor/multi_threaded_compositor.h"
#include "src/server/compositor/stream.h"
#include "mir/test/fake_shared.h"
#include "mir/test/doubles/mock_buffer_stream.h"
#include "mir/test/doubles/null_display.h"
#include "mir/test/doubles/stub_renderer.h"
#include "mir/test/doubles/stub_display_buffer.h"
#include "mir/test/doubles/stub_buffer.h"
#include "mir/test/doubles/null_display_sync_group.h"
#include "mir/test/doubles/mock_event_sink.h"
#include "mir/test/doubles/stub_buffer_allocator.h"

#include <condition_variable>
#include <mutex>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace ms = mir::scene;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;
namespace mr = mir::report;
namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace mf = mir::frontend;
namespace geom = mir::geometry;
using namespace testing;

namespace
{
class StubRendererFactory : public mir::renderer::RendererFactory
{
public:
    std::unique_ptr<mir::renderer::Renderer>
        create_renderer_for(mg::DisplayBuffer&) override
    {
        return std::unique_ptr<mtd::StubRenderer>(new mtd::StubRenderer);
    }
};

struct CountingDisplaySyncGroup : public mtd::StubDisplaySyncGroup
{
    CountingDisplaySyncGroup() :
    mtd::StubDisplaySyncGroup({100,100})
    {
    }

    void post() override
    {
        increment_post_count();
    }

    bool has_posted_at_least(unsigned int count, std::chrono::system_clock::time_point& timeout)
    {
        std::unique_lock<decltype(mutex)> lk(mutex);
        return count_cv.wait_until(lk, timeout,
        [this, &count](){
            return post_count_ >= count;  
        });
    }

private:
    void increment_post_count()
    {
        std::unique_lock<decltype(mutex)> lk(mutex);
        ++post_count_;
        count_cv.notify_all();
    }

    std::mutex mutex;
    std::condition_variable count_cv;
    unsigned int post_count_{0};
};

struct StubDisplay : public mtd::NullDisplay
{
    StubDisplay(mg::DisplaySyncGroup& primary, mg::DisplaySyncGroup& secondary)
      : primary(primary),
        secondary(secondary)
    {
    } 
    void for_each_display_sync_group(std::function<void(mg::DisplaySyncGroup&)> const& fn) override
    {
        fn(primary);
        fn(secondary);
    }
private:
    mg::DisplaySyncGroup& primary;
    mg::DisplaySyncGroup& secondary;
};

struct StubDisplayListener : mc::DisplayListener
{
    virtual void add_display(geom::Rectangle const& /*area*/) override {}
    virtual void remove_display(geom::Rectangle const& /*area*/) override {}
};

struct SurfaceStackCompositor : public Test
{
    SurfaceStackCompositor() :
        timeout{std::chrono::system_clock::now() + std::chrono::seconds(5)},
        stream(std::make_shared<mc::Stream>(geom::Size{ 1, 1 }, mir_pixel_format_abgr_8888 )),
        mock_buffer_stream(std::make_shared<NiceMock<mtd::MockBufferStream>>()),
        streams({ { stream, {0,0}, {} } }),
        stub_surface{std::make_shared<ms::BasicSurface>(
            nullptr /* session */,
            std::string("stub"),
            geom::Rectangle{{0,0},{1,1}},
            mir_pointer_unconfined,
            streams,
            std::shared_ptr<mg::CursorImage>(),
            null_scene_report)},
        stub_buffer(std::make_shared<mtd::StubBuffer>())
    {
        ON_CALL(*mock_buffer_stream, lock_compositor_buffer(_))
            .WillByDefault(Return(mt::fake_shared(*stub_buffer)));
    }
    std::shared_ptr<ms::SceneReport> null_scene_report{mr::null_scene_report()};
    ms::SurfaceStack stack{null_scene_report};
    std::shared_ptr<mc::CompositorReport> null_comp_report{mr::null_compositor_report()};
    StubRendererFactory renderer_factory;
    std::chrono::system_clock::time_point timeout;
    std::shared_ptr<mc::Stream> stream;
    std::shared_ptr<mtd::MockBufferStream> mock_buffer_stream;
    std::list<ms::StreamInfo> const streams;
    std::shared_ptr<ms::BasicSurface> stub_surface;
    ms::SurfaceCreationParameters default_params;
    std::shared_ptr<mg::Buffer> stub_buffer;
    CountingDisplaySyncGroup stub_primary_db;
    CountingDisplaySyncGroup stub_secondary_db;
    StubDisplay stub_display{stub_primary_db, stub_secondary_db};
    StubDisplayListener stub_display_listener;
    mc::DefaultDisplayBufferCompositorFactory dbc_factory{
        mt::fake_shared(renderer_factory),
        null_comp_report};
};

std::chrono::milliseconds const default_delay{-1};

}

TEST_F(SurfaceStackCompositor, composes_on_start_if_told_to_in_constructor)
{
    mc::MultiThreadedCompositor mt_compositor(
        mt::fake_shared(stub_display),
        mt::fake_shared(stack),
        mt::fake_shared(dbc_factory),
        mt::fake_shared(stub_display_listener),
        null_comp_report, default_delay, true);
    mt_compositor.start();

    EXPECT_TRUE(stub_primary_db.has_posted_at_least(1, timeout));
    EXPECT_TRUE(stub_secondary_db.has_posted_at_least(1, timeout));
}

TEST_F(SurfaceStackCompositor, does_not_composes_on_start_if_told_not_to_in_constructor)
{
    mc::MultiThreadedCompositor mt_compositor(
        mt::fake_shared(stub_display),
        mt::fake_shared(stack),
        mt::fake_shared(dbc_factory),
        mt::fake_shared(stub_display_listener),
        null_comp_report, default_delay, false);
    mt_compositor.start();

    EXPECT_TRUE(stub_primary_db.has_posted_at_least(0, timeout));
    EXPECT_TRUE(stub_secondary_db.has_posted_at_least(0, timeout));
}

TEST_F(SurfaceStackCompositor, swapping_a_surface_that_has_been_added_triggers_a_composition)
{
    mc::MultiThreadedCompositor mt_compositor(
        mt::fake_shared(stub_display),
        mt::fake_shared(stack),
        mt::fake_shared(dbc_factory),
        mt::fake_shared(stub_display_listener),
        null_comp_report, default_delay, false);
    mt_compositor.start();

    stack.add_surface(stub_surface, default_params.input_mode);
    stream->submit_buffer(stub_buffer);

    EXPECT_TRUE(stub_primary_db.has_posted_at_least(1, timeout));
    EXPECT_TRUE(stub_secondary_db.has_posted_at_least(1, timeout));
}

//test associated with lp:1290306, 1293896, 1294048, 1294051, 1294053
TEST_F(SurfaceStackCompositor, compositor_runs_until_all_surfaces_buffers_are_consumed)
{
    std::function<void(mir::geometry::Size const&)> frame_callback;
    ON_CALL(*mock_buffer_stream, buffers_ready_for_compositor(_))
        .WillByDefault(Return(5));
    EXPECT_CALL(*mock_buffer_stream, set_frame_posted_callback(_))
        .WillOnce(SaveArg<0>(&frame_callback));
    stub_surface->set_streams(std::list<ms::StreamInfo>{ { mock_buffer_stream, {0,0}, geom::Size{100, 100} } });

    mc::MultiThreadedCompositor mt_compositor(
        mt::fake_shared(stub_display),
        mt::fake_shared(stack),
        mt::fake_shared(dbc_factory),
        mt::fake_shared(stub_display_listener),
        null_comp_report, default_delay, false);
    mt_compositor.start();

    stack.add_surface(stub_surface, default_params.input_mode);
    ASSERT_THAT(frame_callback, Ne(nullptr));
    frame_callback({ 100, 100 });

    EXPECT_TRUE(stub_primary_db.has_posted_at_least(5, timeout));
    EXPECT_TRUE(stub_secondary_db.has_posted_at_least(5, timeout));
}

TEST_F(SurfaceStackCompositor, bypassed_compositor_runs_until_all_surfaces_buffers_are_consumed)
{
    std::function<void(mir::geometry::Size const&)> frame_callback;
    ON_CALL(*mock_buffer_stream, buffers_ready_for_compositor(_))
        .WillByDefault(Return(5));
    ON_CALL(*mock_buffer_stream, lock_compositor_buffer(_))
        .WillByDefault(Return(mt::fake_shared(*stub_buffer)));
    EXPECT_CALL(*mock_buffer_stream, set_frame_posted_callback(_))
        .WillOnce(SaveArg<0>(&frame_callback));
    stub_surface->set_streams(std::list<ms::StreamInfo>{ { mock_buffer_stream, {0,0}, geom::Size{100, 100} } });

    stub_surface->resize(geom::Size{10,10});
    stub_surface->move_to(geom::Point{0,0});
    mc::MultiThreadedCompositor mt_compositor(
        mt::fake_shared(stub_display),
        mt::fake_shared(stack),
        mt::fake_shared(dbc_factory),
        mt::fake_shared(stub_display_listener),
        null_comp_report, default_delay, false);
    mt_compositor.start();

    stack.add_surface(stub_surface, default_params.input_mode);
    ASSERT_THAT(frame_callback, Ne(nullptr));
    frame_callback({ 100, 100 });

    EXPECT_TRUE(stub_primary_db.has_posted_at_least(5, timeout));
    EXPECT_TRUE(stub_secondary_db.has_posted_at_least(5, timeout));
}

TEST_F(SurfaceStackCompositor, an_empty_scene_retriggers)
{
    mc::MultiThreadedCompositor mt_compositor(
        mt::fake_shared(stub_display),
        mt::fake_shared(stack),
        mt::fake_shared(dbc_factory),
        mt::fake_shared(stub_display_listener),
        null_comp_report, default_delay, false);
    mt_compositor.start();

    stack.add_surface(stub_surface, default_params.input_mode);
    streams.front().stream->submit_buffer(stub_buffer);

    EXPECT_TRUE(stub_primary_db.has_posted_at_least(1, timeout));
    EXPECT_TRUE(stub_secondary_db.has_posted_at_least(1, timeout));

    stack.remove_surface(stub_surface);
    
    EXPECT_TRUE(stub_primary_db.has_posted_at_least(2, timeout));
    EXPECT_TRUE(stub_secondary_db.has_posted_at_least(2, timeout));
}

TEST_F(SurfaceStackCompositor, moving_a_surface_triggers_composition)
{
    streams.front().stream->submit_buffer(stub_buffer);
    stack.add_surface(stub_surface, default_params.input_mode);

    mc::MultiThreadedCompositor mt_compositor(
        mt::fake_shared(stub_display),
        mt::fake_shared(stack),
        mt::fake_shared(dbc_factory),
        mt::fake_shared(stub_display_listener),
        null_comp_report, default_delay, false);

    mt_compositor.start();
    stub_surface->move_to(geom::Point{1,1});

    EXPECT_TRUE(stub_primary_db.has_posted_at_least(1, timeout));
    EXPECT_TRUE(stub_secondary_db.has_posted_at_least(1, timeout));
}

TEST_F(SurfaceStackCompositor, removing_a_surface_triggers_composition)
{
    streams.front().stream->submit_buffer(stub_buffer);
    stack.add_surface(stub_surface, default_params.input_mode);

    mc::MultiThreadedCompositor mt_compositor(
        mt::fake_shared(stub_display),
        mt::fake_shared(stack),
        mt::fake_shared(dbc_factory),
        mt::fake_shared(stub_display_listener),
        null_comp_report, default_delay, false);

    mt_compositor.start();
    stack.remove_surface(stub_surface);

    EXPECT_TRUE(stub_primary_db.has_posted_at_least(1, timeout));
    EXPECT_TRUE(stub_secondary_db.has_posted_at_least(1, timeout));
}

TEST_F(SurfaceStackCompositor, buffer_updates_trigger_composition)
{
    stack.add_surface(stub_surface, default_params.input_mode);
    streams.front().stream->submit_buffer(stub_buffer);

    mc::MultiThreadedCompositor mt_compositor(
        mt::fake_shared(stub_display),
        mt::fake_shared(stack),
        mt::fake_shared(dbc_factory),
        mt::fake_shared(stub_display_listener),
        null_comp_report, default_delay, false);

    mt_compositor.start();
    streams.front().stream->submit_buffer(stub_buffer);

    EXPECT_TRUE(stub_primary_db.has_posted_at_least(1, timeout));
    EXPECT_TRUE(stub_secondary_db.has_posted_at_least(1, timeout));
}
