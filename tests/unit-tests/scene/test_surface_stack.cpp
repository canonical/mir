/*
 * Copyright © 2012-2014 Canonical Ltd.
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
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 */

#include "src/server/scene/surface_stack.h"
#include "mir/graphics/buffer_properties.h"
#include "mir/geometry/rectangle.h"
#include "mir/shell/surface_creation_parameters.h"
#include "src/server/report/null_report_factory.h"
#include "src/server/scene/basic_surface.h"
#include "mir/input/input_channel_factory.h"
#include "mir_test_doubles/stub_input_registrar.h"
#include "mir_test_doubles/stub_input_channel.h"
#include "mir_test_doubles/mock_input_registrar.h"
#include "mir_test/fake_shared.h"
#include "mir_test_doubles/stub_buffer_stream.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>
#include <thread>
#include <atomic>

namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace ms = mir::scene;
namespace msh = mir::shell;
namespace mf = mir::frontend;
namespace mi = mir::input;
namespace geom = mir::geometry;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;
namespace mr = mir::report;

namespace
{

struct StubInputChannelFactory : public mi::InputChannelFactory
{
    std::shared_ptr<mi::InputChannel> make_input_channel()
    {
        return std::make_shared<mtd::StubInputChannel>();
    }
};

struct StubInputChannel : public mi::InputChannel
{
    StubInputChannel(int server_fd, int client_fd)
        : s_fd(server_fd),
          c_fd(client_fd)
    {
    }

    int client_fd() const
    {
        return c_fd;
    }
    int server_fd() const
    {
        return s_fd;
    }

    int const s_fd;
    int const c_fd;
};

class MockCallback
{
public:
    MOCK_METHOD0(call, void());
};

struct SurfaceStack : public ::testing::Test
{
    void SetUp()
    {
        using namespace testing;
        default_params = msh::a_surface().of_size(geom::Size{geom::Width{1024}, geom::Height{768}});

        stub_surface1 = std::make_shared<ms::BasicSurface>(
            std::string("stub"),
            geom::Rectangle{{},{}},
            false,
            std::make_shared<mtd::StubBufferStream>(),
            std::shared_ptr<mir::input::InputChannel>(),
            std::shared_ptr<ms::SurfaceConfigurator>(),
            report);

        stub_surface2 = std::make_shared<ms::BasicSurface>(
            std::string("stub"),
            geom::Rectangle{{},{}},
            false,
            std::make_shared<mtd::StubBufferStream>(),
            std::shared_ptr<mir::input::InputChannel>(),
            std::shared_ptr<ms::SurfaceConfigurator>(),
            report);

        stub_surface3 = std::make_shared<ms::BasicSurface>(
            std::string("stub"),
            geom::Rectangle{{},{}},
            false,
            std::make_shared<mtd::StubBufferStream>(),
            std::shared_ptr<mir::input::InputChannel>(),
            std::shared_ptr<ms::SurfaceConfigurator>(),
            report);
    }

    mtd::StubInputRegistrar input_registrar;
    msh::SurfaceCreationParameters default_params;
    std::shared_ptr<ms::BasicSurface> stub_surface1;
    std::shared_ptr<ms::BasicSurface> stub_surface2;
    std::shared_ptr<ms::BasicSurface> stub_surface3;

    std::shared_ptr<ms::SceneReport> const report = mr::null_scene_report();
};

}

TEST_F(SurfaceStack, owns_surface_from_add_to_remove)
{
    using namespace testing;

    ms::SurfaceStack stack(mt::fake_shared(input_registrar), report);

    auto const use_count = stub_surface1.use_count();

    stack.add_surface(stub_surface1, default_params.depth, default_params.input_mode);

    EXPECT_THAT(stub_surface1.use_count(), Gt(use_count));

    stack.remove_surface(stub_surface1);

    EXPECT_THAT(stub_surface1.use_count(), Eq(use_count));
}

TEST_F(SurfaceStack, stacking_order)
{
    using namespace testing;
    ms::SurfaceStack stack(mt::fake_shared(input_registrar), report);

    stack.add_surface(stub_surface1, default_params.depth, default_params.input_mode);
    stack.add_surface(stub_surface2, default_params.depth, default_params.input_mode);
    stack.add_surface(stub_surface3, default_params.depth, default_params.input_mode);

    auto list = stack.generate_renderable_list();
    ASSERT_THAT(list.size(), Eq(3u));
    auto it = list.begin();
    EXPECT_THAT((*it)->id(), Eq(stub_surface1->id()));
    std::advance(it, 1);
    EXPECT_THAT((*it)->id(), Eq(stub_surface2->id()));
    std::advance(it, 1);
    EXPECT_THAT((*it)->id(), Eq(stub_surface3->id()));
}

TEST_F(SurfaceStack, surfaces_are_emitted_by_layer)
{
    using namespace testing;
    ms::SurfaceStack stack(mt::fake_shared(input_registrar), report);

    stack.add_surface(stub_surface1, ms::DepthId{0}, default_params.input_mode);
    stack.add_surface(stub_surface2, ms::DepthId{1}, default_params.input_mode);
    stack.add_surface(stub_surface3, ms::DepthId{0}, default_params.input_mode);

    auto list = stack.generate_renderable_list();
    ASSERT_THAT(list.size(), Eq(3u));
    auto it = list.begin();
    EXPECT_THAT((*it)->id(), Eq(stub_surface1->id()));
    std::advance(it, 1);
    EXPECT_THAT((*it)->id(), Eq(stub_surface3->id()));
    std::advance(it, 1);
    EXPECT_THAT((*it)->id(), Eq(stub_surface2->id()));
}

TEST_F(SurfaceStack, input_registrar_is_notified_of_surfaces)
{
    using namespace ::testing;

    mtd::MockInputRegistrar registrar;
    ms::SurfaceStack stack(mt::fake_shared(registrar), report);

    Sequence seq;
    EXPECT_CALL(registrar, input_channel_opened(_,_,_))
        .InSequence(seq);
    EXPECT_CALL(registrar, input_channel_closed(_))
        .InSequence(seq);

    stack.add_surface(stub_surface1, default_params.depth, default_params.input_mode);
    stack.remove_surface(stub_surface1);
}

TEST_F(SurfaceStack, input_registrar_is_notified_of_input_monitor_scene)
{
    using namespace ::testing;

    mtd::MockInputRegistrar registrar;
    ms::SurfaceStack stack(mt::fake_shared(registrar), report);

    Sequence seq;
    EXPECT_CALL(registrar, input_channel_opened(_,_,mi::InputReceptionMode::receives_all_input))
        .InSequence(seq);
    EXPECT_CALL(registrar, input_channel_closed(_))
        .InSequence(seq);

    stack.add_surface(stub_surface1, default_params.depth, mi::InputReceptionMode::receives_all_input);
    stack.remove_surface(stub_surface1);
}

TEST_F(SurfaceStack, raise_to_top_alters_render_ordering)
{
    using namespace ::testing;

    ms::SurfaceStack stack(mt::fake_shared(input_registrar), report);

    stack.add_surface(stub_surface1, default_params.depth, default_params.input_mode);
    stack.add_surface(stub_surface2, default_params.depth, default_params.input_mode);
    stack.add_surface(stub_surface3, default_params.depth, default_params.input_mode);

    auto list = stack.generate_renderable_list();
    ASSERT_THAT(list.size(), Eq(3u));
    auto it = list.begin();
    EXPECT_THAT((*it)->id(), Eq(stub_surface1->id()));
    std::advance(it, 1);
    EXPECT_THAT((*it)->id(), Eq(stub_surface2->id()));
    std::advance(it, 1);
    EXPECT_THAT((*it)->id(), Eq(stub_surface3->id()));

    stack.raise(stub_surface1);

    list = stack.generate_renderable_list();
    ASSERT_THAT(list.size(), 3u);
    it = list.begin();
    EXPECT_THAT((*it)->id(), Eq(stub_surface2->id()));
    std::advance(it, 1);
    EXPECT_THAT((*it)->id(), Eq(stub_surface3->id()));
    std::advance(it, 1);
    EXPECT_THAT((*it)->id(), Eq(stub_surface1->id()));
}

TEST_F(SurfaceStack, depth_id_trumps_raise)
{
    using namespace ::testing;

    ms::SurfaceStack stack(mt::fake_shared(input_registrar), report);

    stack.add_surface(stub_surface1, ms::DepthId{0}, default_params.input_mode);
    stack.add_surface(stub_surface2, ms::DepthId{0}, default_params.input_mode);
    stack.add_surface(stub_surface3, ms::DepthId{1}, default_params.input_mode);

    auto list = stack.generate_renderable_list();
    ASSERT_THAT(list.size(), 3u);
    auto it = list.begin();
    EXPECT_THAT((*it)->id(), Eq(stub_surface1->id()));
    std::advance(it, 1);
    EXPECT_THAT((*it)->id(), Eq(stub_surface2->id()));
    std::advance(it, 1);
    EXPECT_THAT((*it)->id(), Eq(stub_surface3->id()));

    stack.raise(stub_surface1);

    list = stack.generate_renderable_list();
    ASSERT_THAT(list.size(), 3u);
    it = list.begin();
    EXPECT_THAT((*it)->id(), Eq(stub_surface2->id()));
    std::advance(it, 1);
    EXPECT_THAT((*it)->id(), Eq(stub_surface1->id()));
    std::advance(it, 1);
    EXPECT_THAT((*it)->id(), Eq(stub_surface3->id()));
}

TEST_F(SurfaceStack, raise_throw_behavior)
{
    using namespace ::testing;

    ms::SurfaceStack stack(mt::fake_shared(input_registrar), report);

    std::shared_ptr<ms::BasicSurface> null_surface{nullptr};
    EXPECT_THROW({
            stack.raise(null_surface);
    }, std::runtime_error);
}

TEST_F(SurfaceStack, generate_renderlist)
{
    using namespace testing;

    size_t num_surfaces{3};
    ms::SurfaceStack stack(
        mt::fake_shared(input_registrar), report);

    std::list<std::shared_ptr<ms::Surface>> surfacelist;
    for(auto i = 0u; i < num_surfaces; i++)
    {
        auto const surface = std::make_shared<ms::BasicSurface>(
            std::string("stub"),
            geom::Rectangle{geom::Point{3 * i, 4 * i},geom::Size{1 * i, 2 * i}},
            true,
            std::make_shared<mtd::StubBufferStream>(),
            std::shared_ptr<mir::input::InputChannel>(),
            std::shared_ptr<ms::SurfaceConfigurator>(),
            report);

        surfacelist.emplace_back(surface);
        stack.add_surface(surface, default_params.depth, default_params.input_mode);
    }

    auto list = stack.generate_renderable_list();

    ASSERT_THAT(list.size(), Eq(num_surfaces));

    auto surface_it = surfacelist.begin();
    for(auto& renderable : list)
    {
        EXPECT_THAT(renderable->screen_position(), Eq((*surface_it++)->screen_position()));
    }

    for(auto& surface : surfacelist)
        stack.remove_surface(surface);
}

TEST_F(SurfaceStack, renderlist_is_snapshot_of_positioning_info)
{
    size_t num_surfaces{3};
    ms::SurfaceStack stack(
        mt::fake_shared(input_registrar), report);

    std::list<std::shared_ptr<ms::Surface>> surfacelist;
    for(auto i = 0u; i < num_surfaces; i++)
    {
        auto const surface = std::make_shared<ms::BasicSurface>(
            std::string("stub"),
            geom::Rectangle{geom::Point{3 * i, 4 * i},geom::Size{1 * i, 2 * i}},
            true,
            std::make_shared<mtd::StubBufferStream>(),
            std::shared_ptr<mir::input::InputChannel>(),
            std::shared_ptr<ms::SurfaceConfigurator>(),
            report);

        surfacelist.emplace_back(surface);
        stack.add_surface(surface, default_params.depth, default_params.input_mode);
    }

    auto list = stack.generate_renderable_list();

    auto const changed_position = geom::Point{43,44};
    for(auto const& surface : surfacelist)
        surface->move_to(changed_position);

    //check that the renderables are not at changed_pos
    for(auto& renderable : list)
        EXPECT_THAT(changed_position, testing::Ne(renderable->screen_position().top_left));
}
