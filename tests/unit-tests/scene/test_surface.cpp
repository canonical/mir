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

#include "src/server/scene/basic_surface.h"
#include "src/server/report/null_report_factory.h"
#include "mir/shell/surface_creation_parameters.h"
#include "mir/input/input_channel.h"

#include "mir_test_doubles/mock_buffer_stream.h"
#include "mir_test_doubles/mock_input_surface.h"
#include "mir_test_doubles/mock_compositing_criteria.h"
#include "mir_test_doubles/stub_buffer.h"
#include "mir_test/fake_shared.h"

#include "gmock_set_arg.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <stdexcept>

namespace ms = mir::scene;
namespace msh = mir::shell;
namespace mg = mir::graphics;
namespace mi = mir::input;
namespace geom = mir::geometry;
namespace mt = mir::test;
namespace mtd = mt::doubles;
namespace mr = mir::report;

namespace
{
struct MockInputChannel : public mi::InputChannel
{
    MOCK_CONST_METHOD0(server_fd, int());
    MOCK_CONST_METHOD0(client_fd, int());
};
}

TEST(SurfaceCreationParametersTest, default_creation_parameters)
{
    using namespace geom;
    msh::SurfaceCreationParameters params;

    geom::Point const default_point{geom::X{0}, geom::Y{0}};

    EXPECT_EQ(std::string(), params.name);
    EXPECT_EQ(Width(0), params.size.width);
    EXPECT_EQ(Height(0), params.size.height);
    EXPECT_EQ(default_point, params.top_left);
    EXPECT_EQ(mg::BufferUsage::undefined, params.buffer_usage);
    EXPECT_EQ(mir_pixel_format_invalid, params.pixel_format);

    EXPECT_EQ(msh::a_surface(), params);
}

TEST(SurfaceCreationParametersTest, builder_mutators)
{
    using namespace geom;
    Size const size{1024, 768};
    mg::BufferUsage const usage{mg::BufferUsage::hardware};
    MirPixelFormat const format{mir_pixel_format_abgr_8888};
    std::string name{"surface"};

    auto params = msh::a_surface().of_name(name)
                                 .of_size(size)
                                 .of_buffer_usage(usage)
                                 .of_pixel_format(format);

    EXPECT_EQ(name, params.name);
    EXPECT_EQ(size, params.size);
    EXPECT_EQ(usage, params.buffer_usage);
    EXPECT_EQ(format, params.pixel_format);
}

TEST(SurfaceCreationParametersTest, equality)
{
    using namespace geom;
    Size const size{1024, 768};
    mg::BufferUsage const usage{mg::BufferUsage::hardware};
    MirPixelFormat const format{mir_pixel_format_abgr_8888};

    auto params0 = msh::a_surface().of_name("surface")
                                  .of_size(size)
                                  .of_buffer_usage(usage)
                                  .of_pixel_format(format);

    auto params1 = msh::a_surface().of_name("surface")
                                  .of_size(size)
                                  .of_buffer_usage(usage)
                                  .of_pixel_format(format);

    EXPECT_EQ(params0, params1);
    EXPECT_EQ(params1, params0);
}

TEST(SurfaceCreationParametersTest, inequality)
{
    using namespace geom;

    std::vector<Size> const sizes{{1024, 768},
                                  {1025, 768}};

    std::vector<mg::BufferUsage> const usages{mg::BufferUsage::hardware,
                                              mg::BufferUsage::software};

    std::vector<MirPixelFormat> const formats{mir_pixel_format_abgr_8888,
                                                 mir_pixel_format_bgr_888};

    std::vector<msh::SurfaceCreationParameters> params_vec;

    for (auto const& size : sizes)
    {
        for (auto const& usage : usages)
        {
            for (auto const& format : formats)
            {
                auto cur_params = msh::a_surface().of_name("surface0")
                                                 .of_size(size)
                                                 .of_buffer_usage(usage)
                                                 .of_pixel_format(format);
                params_vec.push_back(cur_params);
                size_t cur_index = params_vec.size() - 1;

                /*
                 * Compare the current SurfaceCreationParameters with all the previously
                 * created ones.
                 */
                for (size_t i = 0; i < cur_index; i++)
                {
                    EXPECT_NE(params_vec[i], params_vec[cur_index]) << "cur_index: " << cur_index << " i: " << i;
                    EXPECT_NE(params_vec[cur_index], params_vec[i]) << "cur_index: " << cur_index << " i: " << i;
                }

            }
        }
    }
}

namespace
{

struct SurfaceCreation : public ::testing::Test
{
    virtual void SetUp()
    {
        using namespace testing;

        notification_count = 0;
        change_notification = [this]()
        {
            notification_count++;
        };

        surface_name = "test_surfaceA";
        pf = mir_pixel_format_abgr_8888;
        size = geom::Size{43, 420};
        rect = geom::Rectangle{geom::Point{geom::X{0}, geom::Y{0}}, size};
        stride = geom::Stride{4 * size.width.as_uint32_t()};
        mock_buffer_stream = std::make_shared<testing::NiceMock<mtd::MockBufferStream>>();

        ON_CALL(*mock_buffer_stream, swap_client_buffers(_, _))
            .WillByDefault(InvokeArgument<1>(&stub_buffer));
    }

    std::string surface_name;
    std::shared_ptr<testing::NiceMock<mtd::MockBufferStream>> mock_buffer_stream;
    MirPixelFormat pf;
    geom::Stride stride;
    geom::Size size;
    geom::Rectangle rect;
    std::shared_ptr<ms::SceneReport> const report = mr::null_scene_report();
    std::function<void()> change_notification;
    int notification_count;
    mtd::StubBuffer stub_buffer;
};

}

TEST_F(SurfaceCreation, test_surface_queries_stream_for_pf)
{
    using namespace testing;
    ms::BasicSurface surf(
        surface_name,
        rect,
        change_notification,
        false,
        mock_buffer_stream,
        std::shared_ptr<mi::InputChannel>(),
        report);

    EXPECT_CALL(*mock_buffer_stream, get_stream_pixel_format())
        .Times(1)
        .WillOnce(Return(pf));

    auto ret_pf = surf.pixel_format();

    EXPECT_EQ(ret_pf, pf);
}

TEST_F(SurfaceCreation, test_surface_gets_right_name)
{
    ms::BasicSurface surf(
        surface_name,
        rect,
        change_notification,
        false,
        mock_buffer_stream,
        std::shared_ptr<mi::InputChannel>(),
        report);

    EXPECT_EQ(surface_name, surf.name());
}

TEST_F(SurfaceCreation, test_surface_queries_state_for_size)
{
    ms::BasicSurface surf(
        surface_name,
        rect,
        change_notification,
        false,
        mock_buffer_stream,
        std::shared_ptr<mi::InputChannel>(),
        report);

    EXPECT_EQ(size, surf.size());
}

TEST_F(SurfaceCreation, test_surface_next_buffer)
{
    using namespace testing;
    ms::BasicSurface surf(
        surface_name,
        rect,
        change_notification,
        false,
        mock_buffer_stream,
        std::shared_ptr<mi::InputChannel>(),
        report);

    mtd::StubBuffer graphics_resource;

    EXPECT_CALL(*mock_buffer_stream, swap_client_buffers(_, _))
        .Times(1)
        .WillOnce(InvokeArgument<1>(&graphics_resource));

    surf.swap_buffers(
        nullptr,
        [&graphics_resource](mg::Buffer* result){ EXPECT_THAT(result, Eq(&graphics_resource)); });
}

TEST_F(SurfaceCreation, test_surface_gets_ipc_from_stream)
{
    using namespace testing;

    mtd::StubBuffer stub_buffer;

    ms::BasicSurface surf(
        surface_name,
        rect,
        change_notification,
        false,
        mock_buffer_stream,
        std::shared_ptr<mi::InputChannel>(),
        report);

    EXPECT_CALL(*mock_buffer_stream, swap_client_buffers(_, _))
        .Times(1)
        .WillOnce(InvokeArgument<1>(&stub_buffer));

    surf.swap_buffers(
        nullptr,
        [&stub_buffer](mg::Buffer* result){ EXPECT_THAT(result, Eq(&stub_buffer)); });
}

TEST_F(SurfaceCreation, test_surface_gets_top_left)
{
    ms::BasicSurface surf(
        surface_name,
        rect,
        change_notification,
        false,
        mock_buffer_stream,
        std::shared_ptr<mi::InputChannel>(),
        report);

    auto ret_top_left = surf.top_left();
    EXPECT_EQ(geom::Point(), ret_top_left);
}

TEST_F(SurfaceCreation, test_surface_move_to)
{
    geom::Point p{55, 66};

    ms::BasicSurface surf(
        surface_name,
        rect,
        change_notification,
        false,
        mock_buffer_stream,
        std::shared_ptr<mi::InputChannel>(),
        report);

    surf.move_to(p);
    EXPECT_EQ(p, surf.top_left());
}

TEST_F(SurfaceCreation, resize_updates_stream_and_state)
{
    using namespace testing;
    geom::Size const new_size{123, 456};

    EXPECT_CALL(*mock_buffer_stream, resize(new_size))
        .Times(1);

    ms::BasicSurface surf(
        surface_name,
        rect,
        change_notification,
        false,
        mock_buffer_stream,
        std::shared_ptr<mi::InputChannel>(),
        report);

    ASSERT_NE(new_size, surf.size());
    EXPECT_TRUE(surf.resize(new_size));
    EXPECT_EQ(new_size, surf.size());
}

TEST_F(SurfaceCreation, duplicate_resize_ignored)
{
    using namespace testing;
    geom::Size const new_size{123, 456};

    EXPECT_CALL(*mock_buffer_stream, resize(new_size))
        .Times(1);

    ms::BasicSurface surf(
        surface_name,
        rect,
        change_notification,
        false,
        mock_buffer_stream,
        std::shared_ptr<mi::InputChannel>(),
        report);

    ASSERT_NE(new_size, surf.size());

    EXPECT_TRUE(surf.resize(new_size));
    EXPECT_EQ(new_size, surf.size());

    EXPECT_FALSE(surf.resize(new_size));
    EXPECT_EQ(new_size, surf.size());
}

TEST_F(SurfaceCreation, unsuccessful_resize_does_not_update_state)
{
    using namespace testing;
    geom::Size const new_size{123, 456};

    EXPECT_CALL(*mock_buffer_stream, resize(new_size))
        .Times(1)
        .WillOnce(Throw(std::runtime_error("bad resize")));

    ms::BasicSurface surf(
        surface_name,
        rect,
        change_notification,
        false,
        mock_buffer_stream,
        std::shared_ptr<mi::InputChannel>(),
        report);

    EXPECT_THROW({
        surf.resize(new_size);
    }, std::runtime_error);

    EXPECT_EQ(size, surf.size());
}

TEST_F(SurfaceCreation, impossible_resize_throws)
{
    using namespace testing;

    geom::Size const bad_sizes[] =
    {
        {0, 123},
        {456, 0},
        {-1, -1},
        {78, -10},
        {0, 0}
    };

    ms::BasicSurface surf(
        surface_name,
        rect,
        change_notification,
        false,
        mock_buffer_stream,
        std::shared_ptr<mi::InputChannel>(),
        report);

    EXPECT_CALL(*mock_buffer_stream, resize(size))
        .Times(0);

    for (auto &size : bad_sizes)
    {
        EXPECT_THROW({
            surf.resize(size);
        }, std::logic_error);
    }
}

TEST_F(SurfaceCreation, test_get_input_channel)
{
    auto mock_channel = std::make_shared<MockInputChannel>();
    ms::BasicSurface surf(
        surface_name,
        rect,
        change_notification,
        false,
        mock_buffer_stream,
        mock_channel,
        report);

    EXPECT_EQ(mock_channel, surf.input_channel());
}

TEST_F(SurfaceCreation, test_surface_set_alpha)
{
    using namespace testing;

    float alpha = 0.5f;
    ms::BasicSurface surf(
        surface_name,
        rect,
        change_notification,
        false,
        mock_buffer_stream,
        std::shared_ptr<mi::InputChannel>(),
        report);

    surf.set_alpha(alpha);
    EXPECT_FLOAT_EQ(alpha, surf.alpha());
}

TEST_F(SurfaceCreation, test_surface_force_requests_to_complete)
{
    using namespace testing;

    EXPECT_CALL(*mock_buffer_stream, force_requests_to_complete()).Times(Exactly(1));

    ms::BasicSurface surf(
        surface_name,
        rect,
        change_notification,
        false,
        mock_buffer_stream,
        std::shared_ptr<mi::InputChannel>(),
        report);

    surf.force_requests_to_complete();
}

TEST_F(SurfaceCreation, test_surface_allow_framedropping)
{
    using namespace testing;

    EXPECT_CALL(*mock_buffer_stream, allow_framedropping(true))
        .Times(1);

    ms::BasicSurface surf(
        surface_name,
        rect,
        change_notification,
        false,
        mock_buffer_stream,
        std::shared_ptr<mi::InputChannel>(),
        report);

    surf.allow_framedropping(true);
}

TEST_F(SurfaceCreation, test_surface_next_buffer_tells_state_on_first_frame)
{
    ms::BasicSurface surf(
        surface_name,
        rect,
        change_notification,
        false,
        mock_buffer_stream,
        std::shared_ptr<mi::InputChannel>(),
        report);

    mg::Buffer* buffer{nullptr};

    auto const complete = [&buffer](mg::Buffer* new_buffer){ buffer = new_buffer; };
    surf.swap_buffers(buffer, complete);
    surf.swap_buffers(buffer, complete);
    surf.swap_buffers(buffer, complete);
    surf.swap_buffers(buffer, complete);

    EXPECT_EQ(3, notification_count); 
}

TEST_F(SurfaceCreation, input_fds)
{
    using namespace testing;

    ms::BasicSurface surf(
        surface_name,
        rect,
        change_notification,
        false,
        mock_buffer_stream,
        std::shared_ptr<mi::InputChannel>(),
        report);

    EXPECT_THROW({
            surf.client_input_fd();
    }, std::logic_error);

    MockInputChannel channel;
    int const client_fd = 13;
    EXPECT_CALL(channel, client_fd()).Times(1).WillOnce(Return(client_fd));

    ms::BasicSurface input_surf(
        surface_name,
        rect,
        change_notification,
        false,
        mock_buffer_stream,mt::fake_shared(channel),
        report);

    EXPECT_EQ(client_fd, input_surf.client_input_fd());
}
