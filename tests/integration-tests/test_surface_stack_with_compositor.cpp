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

#include "mir_test_doubles/stub_input_registrar.h"
#include "src/server/report/null_report_factory.h"
#include "src/server/scene/surface_stack.h"
#include "src/server/compositor/gl_renderer_factory.h"
#include "mir/shell/surface_creation_parameters.h"
#include "src/server/compositor/default_display_buffer_compositor_factory.h"
#include "src/server/compositor/multi_threaded_compositor.h"
#include "mir_test_doubles/null_display.h"
#include "mir_test_doubles/stub_renderer.h"
#include "mir_test_doubles/stub_display_buffer.h"
#include "mir_test_doubles/stub_buffer.h"
#include "mir_test/fake_shared.h"


#include "mir_test_doubles/mock_buffer_stream.h"
#include "src/server/scene/basic_surface.h"
//#include "mir/input/input_channel_factory.h"
//#include "mir_test_doubles/stub_input_registrar.h"
//#include "mir_test_doubles/stub_input_channel.h"
//#include "mir_test_doubles/mock_input_registrar.h"



#include <mutex>
#include <condition_variable>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace ms = mir::scene;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;
namespace mr = mir::report;
namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace geom = mir::geometry;
namespace msh = mir::shell;

namespace
{
class StubRendererFactory : public mc::RendererFactory
{
public:
    std::unique_ptr<mc::Renderer> create_renderer_for(geom::Rectangle const&) override
    {
        return std::unique_ptr<mtd::StubRenderer>(new mtd::StubRenderer);
    }
};

struct CountingDisplayBuffer : public mtd::StubDisplayBuffer
{
    CountingDisplayBuffer(unsigned int expected_count)
     : StubDisplayBuffer({{0,0}, {10, 10}}),
       expected(expected_count)
    {
    }

    void render_and_post_update(
        mg::RenderableList const&,
        std::function<void(mg::Renderable const&)> const&) override
    {
        increment_post_count();
    }

    void post_update() override
    {
        increment_post_count();
    }

    void post_update(std::shared_ptr<mg::Buffer>) override
    {
        increment_post_count();
    }

    bool reaches_expected_count_before(std::chrono::system_clock::time_point& timeout)
    {
        std::unique_lock<decltype(mutex)> lk(mutex);

        return count_cv.wait_until(lk, timeout,
        [this](){
            return post_count_ == expected;  
        });
    }

private:
    void increment_post_count()
    {
        std::unique_lock<decltype(mutex)> lk(mutex);
        if (++post_count_ > expected)
            throw std::logic_error("post was called more times than expected.");;
        count_cv.notify_all();
    }
    std::mutex mutex;
    std::condition_variable count_cv;

    unsigned int const expected;
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

struct SurfaceStackCompositorInteractions : public testing::Test
{
    SurfaceStackCompositorInteractions()
        : timeout{std::chrono::system_clock::now() + std::chrono::seconds(1)},
          mock_buffer_stream(std::make_shared<testing::NiceMock<mtd::MockBufferStream>>()),
          stub_surface{std::make_shared<ms::BasicSurface>(
            std::string("stub"),
            geom::Rectangle{{0,0},{1,1}},
            false,
            mock_buffer_stream,
            std::shared_ptr<mir::input::InputChannel>(),
            std::shared_ptr<ms::SurfaceConfigurator>(),
            null_scene_report)}

    {
        using namespace testing;
        ON_CALL(*mock_buffer_stream, lock_compositor_buffer(_))
            .WillByDefault(Return(mt::fake_shared(stubbuf)));
    }
    mtd::StubInputRegistrar stub_input_registrar;
    std::shared_ptr<ms::SceneReport> null_scene_report{mr::null_scene_report()};
    std::shared_ptr<mc::CompositorReport> null_comp_report{mr::null_compositor_report()};
    StubRendererFactory renderer_factory;
    std::chrono::system_clock::time_point timeout;
    std::shared_ptr<mtd::MockBufferStream> mock_buffer_stream;
    std::shared_ptr<ms::BasicSurface> stub_surface; 
    msh::SurfaceCreationParameters default_params;
    mtd::StubBuffer stubbuf;
};
}

TEST_F(SurfaceStackCompositorInteractions, composes_on_start_if_told_to_in_constructor)
{
    unsigned int const expected_post_count{1};
    CountingDisplayBuffer stub_primary_db{expected_post_count};
    CountingDisplayBuffer stub_secondary_db{expected_post_count};
    StubDisplay stub_display{stub_primary_db, stub_secondary_db};
    ms::SurfaceStack stack(mt::fake_shared(stub_input_registrar), null_scene_report);
    mc::DefaultDisplayBufferCompositorFactory dbc_factory{
        mt::fake_shared(stack),
        mt::fake_shared(renderer_factory),
        null_comp_report};

    mc::MultiThreadedCompositor mt_compositor(
        mt::fake_shared(stub_display),
        mt::fake_shared(stack),
        mt::fake_shared(dbc_factory),
        null_comp_report, true);
    mt_compositor.start();

    EXPECT_TRUE(stub_primary_db.reaches_expected_count_before(timeout));
    EXPECT_TRUE(stub_secondary_db.reaches_expected_count_before(timeout));
}

TEST_F(SurfaceStackCompositorInteractions, does_not_composes_on_start_if_told_not_to_in_constructor)
{
    unsigned int const expected_post_count{0};
    CountingDisplayBuffer stub_primary_db{expected_post_count};
    CountingDisplayBuffer stub_secondary_db{expected_post_count};
    StubDisplay stub_display{stub_primary_db, stub_secondary_db};
    ms::SurfaceStack stack(mt::fake_shared(stub_input_registrar), null_scene_report);
    mc::DefaultDisplayBufferCompositorFactory dbc_factory{
        mt::fake_shared(stack),
        mt::fake_shared(renderer_factory),
        null_comp_report};

    mc::MultiThreadedCompositor mt_compositor(
        mt::fake_shared(stub_display),
        mt::fake_shared(stack),
        mt::fake_shared(dbc_factory),
        null_comp_report, false);
    mt_compositor.start();

    EXPECT_TRUE(stub_primary_db.reaches_expected_count_before(timeout));
    EXPECT_TRUE(stub_secondary_db.reaches_expected_count_before(timeout));
}

TEST_F(SurfaceStackCompositorInteractions, adding_a_surface_that_has_been_swapped_triggers_a_composition)
{
    unsigned int const expected_post_count{1};
    CountingDisplayBuffer stub_primary_db{expected_post_count};
    CountingDisplayBuffer stub_secondary_db{expected_post_count};
    StubDisplay stub_display{stub_primary_db, stub_secondary_db};
    ms::SurfaceStack stack(mt::fake_shared(stub_input_registrar), null_scene_report);
    mc::DefaultDisplayBufferCompositorFactory dbc_factory{
        mt::fake_shared(stack),
        mt::fake_shared(renderer_factory),
        null_comp_report};

    mc::MultiThreadedCompositor mt_compositor(
        mt::fake_shared(stub_display),
        mt::fake_shared(stack),
        mt::fake_shared(dbc_factory),
        null_comp_report, false);
    mt_compositor.start();

    stub_surface->swap_buffers(&stubbuf, [](mg::Buffer*){});
    stack.add_surface(stub_surface, default_params.depth, default_params.input_mode);

    EXPECT_TRUE(stub_primary_db.reaches_expected_count_before(timeout));
    EXPECT_TRUE(stub_secondary_db.reaches_expected_count_before(timeout));
}

namespace
{
int thread_distinguishing_buffers_ready_for_compositor()
{
    //c++14 integer_sequence
    std::vector<int> sequence{5,4,3,2,1};
    thread_local size_t sequence_idx{0};
    return sequence[sequence_idx++];
}
}
TEST_F(SurfaceStackCompositorInteractions, compositor_runs_until_all_surfaces_buffers_are_consumed)
{
    using namespace testing;
    ON_CALL(*mock_buffer_stream, buffers_ready_for_compositor())
        .WillByDefault(Invoke(thread_distinguishing_buffers_ready_for_compositor));

    unsigned int const expected_post_count{5};
    CountingDisplayBuffer stub_primary_db{expected_post_count};
    CountingDisplayBuffer stub_secondary_db{expected_post_count};
    StubDisplay stub_display{stub_primary_db, stub_secondary_db};
    ms::SurfaceStack stack(mt::fake_shared(stub_input_registrar), null_scene_report);
    mc::DefaultDisplayBufferCompositorFactory dbc_factory{
        mt::fake_shared(stack),
        mt::fake_shared(renderer_factory),
        null_comp_report};

    mc::MultiThreadedCompositor mt_compositor(
        mt::fake_shared(stub_display),
        mt::fake_shared(stack),
        mt::fake_shared(dbc_factory),
        null_comp_report, false);
    mt_compositor.start();

    stub_surface->swap_buffers(&stubbuf, [](mg::Buffer*){});
    stack.add_surface(stub_surface, default_params.depth, default_params.input_mode);

    EXPECT_TRUE(stub_primary_db.reaches_expected_count_before(timeout));
    EXPECT_TRUE(stub_secondary_db.reaches_expected_count_before(timeout));
}

TEST_F(SurfaceStackCompositorInteractions, moving_a_surface_triggers_composition)
{
    unsigned int const expected_post_count{1};
    CountingDisplayBuffer stub_primary_db{expected_post_count};
    CountingDisplayBuffer stub_secondary_db{expected_post_count};
    StubDisplay stub_display{stub_primary_db, stub_secondary_db};
    ms::SurfaceStack stack(mt::fake_shared(stub_input_registrar), null_scene_report);
    mc::DefaultDisplayBufferCompositorFactory dbc_factory{
        mt::fake_shared(stack),
        mt::fake_shared(renderer_factory),
        null_comp_report};

    stub_surface->swap_buffers(&stubbuf, [](mg::Buffer*){});
    stack.add_surface(stub_surface, default_params.depth, default_params.input_mode);

    mc::MultiThreadedCompositor mt_compositor(
        mt::fake_shared(stub_display),
        mt::fake_shared(stack),
        mt::fake_shared(dbc_factory),
        null_comp_report, false);

    mt_compositor.start();
    stub_surface->move_to(geom::Point{1,1});

    EXPECT_TRUE(stub_primary_db.reaches_expected_count_before(timeout));
    EXPECT_TRUE(stub_secondary_db.reaches_expected_count_before(timeout));
}

TEST_F(SurfaceStackCompositorInteractions, removing_a_surface_triggers_composition)
{
    unsigned int const expected_post_count{1};
    CountingDisplayBuffer stub_primary_db{expected_post_count};
    CountingDisplayBuffer stub_secondary_db{expected_post_count};
    StubDisplay stub_display{stub_primary_db, stub_secondary_db};
    ms::SurfaceStack stack(mt::fake_shared(stub_input_registrar), null_scene_report);
    mc::DefaultDisplayBufferCompositorFactory dbc_factory{
        mt::fake_shared(stack),
        mt::fake_shared(renderer_factory),
        null_comp_report};

    stub_surface->swap_buffers(&stubbuf, [](mg::Buffer*){});
    stack.add_surface(stub_surface, default_params.depth, default_params.input_mode);

    mc::MultiThreadedCompositor mt_compositor(
        mt::fake_shared(stub_display),
        mt::fake_shared(stack),
        mt::fake_shared(dbc_factory),
        null_comp_report, false);

    mt_compositor.start();
    stack.remove_surface(stub_surface);

    EXPECT_TRUE(stub_primary_db.reaches_expected_count_before(timeout));
    EXPECT_TRUE(stub_secondary_db.reaches_expected_count_before(timeout));
}

TEST_F(SurfaceStackCompositorInteractions, buffer_updates_trigger_composition)
{
    unsigned int const expected_post_count{1};
    CountingDisplayBuffer stub_primary_db{expected_post_count};
    CountingDisplayBuffer stub_secondary_db{expected_post_count};
    StubDisplay stub_display{stub_primary_db, stub_secondary_db};
    ms::SurfaceStack stack(mt::fake_shared(stub_input_registrar), null_scene_report);
    mc::DefaultDisplayBufferCompositorFactory dbc_factory{
        mt::fake_shared(stack),
        mt::fake_shared(renderer_factory),
        null_comp_report};

    stub_surface->swap_buffers(&stubbuf, [](mg::Buffer*){});
    stack.add_surface(stub_surface, default_params.depth, default_params.input_mode);

    mc::MultiThreadedCompositor mt_compositor(
        mt::fake_shared(stub_display),
        mt::fake_shared(stack),
        mt::fake_shared(dbc_factory),
        null_comp_report, false);

    mt_compositor.start();
    stub_surface->swap_buffers(&stubbuf, [](mg::Buffer*){});

    EXPECT_TRUE(stub_primary_db.reaches_expected_count_before(timeout));
    EXPECT_TRUE(stub_secondary_db.reaches_expected_count_before(timeout));
}

//TODO: future tests:
//TEST_F(SurfaceStackCompositorInteractions, surface_buffer_updates_triggers_composition)
//TEST_F(SurfaceStackCompositorInteractions, surface_buffer_updates_triggers_composition_until_all_consumed)
