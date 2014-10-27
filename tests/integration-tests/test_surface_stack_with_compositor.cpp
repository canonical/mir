/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/scene/surface_creation_parameters.h"
#include "src/server/report/null_report_factory.h"
#include "src/server/scene/surface_stack.h"
#include "src/server/compositor/gl_renderer_factory.h"
#include "src/server/scene/basic_surface.h"
#include "src/server/compositor/default_display_buffer_compositor_factory.h"
#include "src/server/compositor/multi_threaded_compositor.h"
#include "mir_test/fake_shared.h"
#include "mir_test_doubles/mock_buffer_stream.h"
#include "mir_test_doubles/null_display.h"
#include "mir_test_doubles/stub_renderer.h"
#include "mir_test_doubles/stub_display_buffer.h"
#include "mir_test_doubles/stub_buffer.h"
#include "mir_test_doubles/stub_input_sender.h"
#include "mir_test_doubles/null_surface_configurator.h"

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
namespace geom = mir::geometry;

namespace
{
class StubRendererFactory : public mc::RendererFactory
{
public:
    std::unique_ptr<mc::Renderer> create_renderer_for(geom::Rectangle const&,
        mc::DestinationAlpha) override
    {
        return std::unique_ptr<mtd::StubRenderer>(new mtd::StubRenderer);
    }
};

struct CountingDisplayBuffer : public mtd::StubDisplayBuffer
{
    CountingDisplayBuffer() :
        StubDisplayBuffer({{0,0}, {10, 10}})
    {
    }

    bool post_renderables_if_optimizable(mg::RenderableList const&) override
    {
        return false;
    }

    void post_update() override
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
    StubDisplay(mg::DisplayBuffer& primary, mg::DisplayBuffer& secondary)
      : primary(primary),
        secondary(secondary)
    {
    } 
    void for_each_display_buffer(std::function<void(mg::DisplayBuffer&)> const& fn) override
    {
        fn(primary);
        fn(secondary);
    }
private:
    mg::DisplayBuffer& primary;
    mg::DisplayBuffer& secondary;
};

class BypassStubBuffer : public mtd::StubBuffer
{
public:
    bool can_bypass() const override
    {
        return true;
    }
};

struct SurfaceStackCompositor : public testing::Test
{
    SurfaceStackCompositor() :
        timeout{std::chrono::system_clock::now() + std::chrono::seconds(5)},
        mock_buffer_stream(std::make_shared<testing::NiceMock<mtd::MockBufferStream>>()),
        stub_surface{std::make_shared<ms::BasicSurface>(
            std::string("stub"),
            geom::Rectangle{{0,0},{1,1}},
            false,
            mock_buffer_stream,
            std::shared_ptr<mir::input::InputChannel>(),
            std::shared_ptr<mtd::StubInputSender>(),
            std::make_shared<mtd::NullSurfaceConfigurator>(),
            std::shared_ptr<mg::CursorImage>(),
            null_scene_report)}
    {
        using namespace testing;
        ON_CALL(*mock_buffer_stream, lock_compositor_buffer(_))
            .WillByDefault(Return(mt::fake_shared(stubbuf)));
    }
    std::shared_ptr<ms::SceneReport> null_scene_report{mr::null_scene_report()};
    ms::SurfaceStack stack{null_scene_report};
    std::shared_ptr<mc::CompositorReport> null_comp_report{mr::null_compositor_report()};
    StubRendererFactory renderer_factory;
    std::chrono::system_clock::time_point timeout;
    std::shared_ptr<mtd::MockBufferStream> mock_buffer_stream;
    std::shared_ptr<ms::BasicSurface> stub_surface;
    ms::SurfaceCreationParameters default_params;
    BypassStubBuffer stubbuf;
    CountingDisplayBuffer stub_primary_db;
    CountingDisplayBuffer stub_secondary_db;
    StubDisplay stub_display{stub_primary_db, stub_secondary_db};
    mc::DefaultDisplayBufferCompositorFactory dbc_factory{
        mt::fake_shared(renderer_factory),
        null_comp_report};
};
}

TEST_F(SurfaceStackCompositor, composes_on_start_if_told_to_in_constructor)
{
    mc::MultiThreadedCompositor mt_compositor(
        mt::fake_shared(stub_display),
        mt::fake_shared(stack),
        mt::fake_shared(dbc_factory),
        null_comp_report, true);
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
        null_comp_report, false);
    mt_compositor.start();

    EXPECT_TRUE(stub_primary_db.has_posted_at_least(0, timeout));
    EXPECT_TRUE(stub_secondary_db.has_posted_at_least(0, timeout));
}

TEST_F(SurfaceStackCompositor, adding_a_surface_that_has_been_swapped_triggers_a_composition)
{
    mc::MultiThreadedCompositor mt_compositor(
        mt::fake_shared(stub_display),
        mt::fake_shared(stack),
        mt::fake_shared(dbc_factory),
        null_comp_report, false);
    mt_compositor.start();

    stub_surface->swap_buffers(&stubbuf, [](mg::Buffer*){});
    stack.add_surface(stub_surface, default_params.depth, default_params.input_mode);

    EXPECT_TRUE(stub_primary_db.has_posted_at_least(1, timeout));
    EXPECT_TRUE(stub_secondary_db.has_posted_at_least(1, timeout));
}

//test associated with lp:1290306, 1293896, 1294048, 1294051, 1294053
TEST_F(SurfaceStackCompositor, compositor_runs_until_all_surfaces_buffers_are_consumed)
{
    using namespace testing;
    ON_CALL(*mock_buffer_stream, buffers_ready_for_compositor())
        .WillByDefault(Return(5));

    mc::MultiThreadedCompositor mt_compositor(
        mt::fake_shared(stub_display),
        mt::fake_shared(stack),
        mt::fake_shared(dbc_factory),
        null_comp_report, false);
    mt_compositor.start();

    stack.add_surface(stub_surface, default_params.depth, default_params.input_mode);
    stub_surface->swap_buffers(&stubbuf, [](mg::Buffer*){});

    EXPECT_TRUE(stub_primary_db.has_posted_at_least(5, timeout));
    EXPECT_TRUE(stub_secondary_db.has_posted_at_least(5, timeout));
}

TEST_F(SurfaceStackCompositor, bypassed_compositor_runs_until_all_surfaces_buffers_are_consumed)
{
    using namespace testing;
    ON_CALL(*mock_buffer_stream, buffers_ready_for_compositor())
        .WillByDefault(Return(5));

    stub_surface->resize(geom::Size{10,10});
    stub_surface->move_to(geom::Point{0,0});
    mc::MultiThreadedCompositor mt_compositor(
        mt::fake_shared(stub_display),
        mt::fake_shared(stack),
        mt::fake_shared(dbc_factory),
        null_comp_report, false);
    mt_compositor.start();

    stack.add_surface(stub_surface, default_params.depth, default_params.input_mode);
    stub_surface->swap_buffers(&stubbuf, [](mg::Buffer*){});

    EXPECT_TRUE(stub_primary_db.has_posted_at_least(5, timeout));
    EXPECT_TRUE(stub_secondary_db.has_posted_at_least(5, timeout));
}

TEST_F(SurfaceStackCompositor, an_empty_scene_retriggers)
{
    using namespace testing;
    ON_CALL(*mock_buffer_stream, buffers_ready_for_compositor())
        .WillByDefault(Return(0));

    mc::MultiThreadedCompositor mt_compositor(
        mt::fake_shared(stub_display),
        mt::fake_shared(stack),
        mt::fake_shared(dbc_factory),
        null_comp_report, false);
    mt_compositor.start();

    stack.add_surface(stub_surface, default_params.depth, default_params.input_mode);
    stub_surface->swap_buffers(&stubbuf, [](mg::Buffer*){});

    EXPECT_TRUE(stub_primary_db.has_posted_at_least(1, timeout));
    EXPECT_TRUE(stub_secondary_db.has_posted_at_least(1, timeout));

    stack.remove_surface(stub_surface);
    
    EXPECT_TRUE(stub_primary_db.has_posted_at_least(2, timeout));
    EXPECT_TRUE(stub_secondary_db.has_posted_at_least(2, timeout));
}

TEST_F(SurfaceStackCompositor, moving_a_surface_triggers_composition)
{
    stub_surface->swap_buffers(&stubbuf, [](mg::Buffer*){});
    stack.add_surface(stub_surface, default_params.depth, default_params.input_mode);

    mc::MultiThreadedCompositor mt_compositor(
        mt::fake_shared(stub_display),
        mt::fake_shared(stack),
        mt::fake_shared(dbc_factory),
        null_comp_report, false);

    mt_compositor.start();
    stub_surface->move_to(geom::Point{1,1});

    EXPECT_TRUE(stub_primary_db.has_posted_at_least(1, timeout));
    EXPECT_TRUE(stub_secondary_db.has_posted_at_least(1, timeout));
}

TEST_F(SurfaceStackCompositor, removing_a_surface_triggers_composition)
{
    stub_surface->swap_buffers(&stubbuf, [](mg::Buffer*){});
    stack.add_surface(stub_surface, default_params.depth, default_params.input_mode);

    mc::MultiThreadedCompositor mt_compositor(
        mt::fake_shared(stub_display),
        mt::fake_shared(stack),
        mt::fake_shared(dbc_factory),
        null_comp_report, false);

    mt_compositor.start();
    stack.remove_surface(stub_surface);

    EXPECT_TRUE(stub_primary_db.has_posted_at_least(1, timeout));
    EXPECT_TRUE(stub_secondary_db.has_posted_at_least(1, timeout));
}

TEST_F(SurfaceStackCompositor, buffer_updates_trigger_composition)
{
    ON_CALL(*mock_buffer_stream, buffers_ready_for_compositor())
        .WillByDefault(testing::Return(1));
    stack.add_surface(stub_surface, default_params.depth, default_params.input_mode);
    stub_surface->swap_buffers(&stubbuf, [](mg::Buffer*){});

    mc::MultiThreadedCompositor mt_compositor(
        mt::fake_shared(stub_display),
        mt::fake_shared(stack),
        mt::fake_shared(dbc_factory),
        null_comp_report, false);

    mt_compositor.start();
    stub_surface->swap_buffers(&stubbuf, [](mg::Buffer*){});

    EXPECT_TRUE(stub_primary_db.has_posted_at_least(1, timeout));
    EXPECT_TRUE(stub_secondary_db.has_posted_at_least(1, timeout));
}
