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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "src/server/compositor/compositing_screencast.h"
#include "mir/compositor/display_buffer_compositor_factory.h"
#include "mir/compositor/display_buffer_compositor.h"
#include "mir/compositor/scene.h"
#include "mir/graphics/display_buffer.h"
#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/geometry/rectangle.h"
#include "mir/geometry/rectangles.h"
#include "mir/renderer/gl/render_target.h"

#include "mir/test/doubles/null_display.h"
#include "mir/test/doubles/null_display_buffer_compositor_factory.h"
#include "mir/test/doubles/stub_display_configuration.h"
#include "mir/test/doubles/stub_gl_buffer_allocator.h"
#include "mir/test/doubles/mock_buffer.h"
#include "mir/test/doubles/mock_gl.h"
#include "mir/test/doubles/stub_scene.h"
#include "mir/test/doubles/stub_scene_element.h"
#include "mir/test/doubles/mock_scene.h"

#include "mir/test/as_render_target.h"
#include "mir/test/fake_shared.h"

#include <boost/throw_exception.hpp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <unordered_set>

namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace mf = mir::frontend;
namespace mtd = mir::test::doubles;
namespace mt = mir::test;
namespace mrgl = mir::renderer::gl;
namespace geom = mir::geometry;

namespace
{

class StubVirtualOutput : public mg::VirtualOutput
{
public:
    StubVirtualOutput(std::function<void()> notify_enable)
        : notify_enable{notify_enable}
    {
    }

    void enable() override
    {
        notify_enable();
    }

    void disable() override
    {
    }

private:
    std::function<void()> notify_enable;
};

class StubDisplay : public mtd::NullDisplay
{
public:
    StubDisplay()
        : display_regions{{{0, 0}, {1920, 1080}}, {{1920, 0}, {640,480}}}
    {
    }

    std::unique_ptr<mg::DisplayConfiguration> configuration() const override
    {
        return {std::make_unique<mtd::StubDisplayConfig>(display_regions)};
    }

    std::unique_ptr<mg::VirtualOutput> create_virtual_output(int /*width*/, int /*height*/) override
    {
        virtual_output_created = true;
        return {std::make_unique<StubVirtualOutput>([this] { virtual_output_enabled = true; })};
    }

    geom::Rectangle extents()
    {
        geom::Rectangles disp_rects;
        configuration()->for_each_output([&disp_rects](mg::DisplayConfigurationOutput const& disp_conf)
        {
            if (disp_conf.connected)
                disp_rects.add(disp_conf.extents());
        });
        return disp_rects.bounding_rectangle();
    }

    geom::Point adjacent_point()
    {
        return {extents().size.width.as_int(), 0};
    }

    bool virtual_output_created{false};
    bool virtual_output_enabled{false};
private:
    std::vector<geom::Rectangle> const display_regions;
};

struct MockDisplayBufferCompositor : mc::DisplayBufferCompositor
{
    void composite(mc::SceneElementSequence&& seq)
    {
        composite_(seq);
    }

    MOCK_METHOD1(composite_, void(mc::SceneElementSequence&));
};

class WrappingDisplayBufferCompositor : public mc::DisplayBufferCompositor
{
public:
    WrappingDisplayBufferCompositor(mc::DisplayBufferCompositor& comp, mg::DisplayBuffer& db)
        : comp(comp),
          render_target(mt::as_render_target(db))
    {
        //TODO: we shouldn't need to care if the display buffer is GL capable
        if (!render_target)
            BOOST_THROW_EXCEPTION(std::logic_error("Display Buffer does not support GL rendering"));
    }

    void composite(mc::SceneElementSequence&& elements)
    {
        render_target->bind();
        comp.composite(std::move(elements));
        render_target->swap_buffers();
    }

private:
    mc::DisplayBufferCompositor& comp;
    mrgl::RenderTarget* render_target;
};

struct MockBufferAllocator : mg::GraphicBufferAllocator
{
    MOCK_METHOD1(alloc_buffer, std::shared_ptr<mg::Buffer>(mg::BufferProperties const&));
    MOCK_METHOD2(alloc_software_buffer, std::shared_ptr<mg::Buffer>(geom::Size, MirPixelFormat));
    MOCK_METHOD3(alloc_buffer, std::shared_ptr<mg::Buffer>(geom::Size, uint32_t, uint32_t));
    MOCK_METHOD0(supported_pixel_formats,
                 std::vector<MirPixelFormat>());
};

struct MockDisplayBufferCompositorFactory : mc::DisplayBufferCompositorFactory
{
    std::unique_ptr<mc::DisplayBufferCompositor> create_compositor_for(mg::DisplayBuffer& db) override
    {
        create_compositor_mock(db);
        return std::make_unique<WrappingDisplayBufferCompositor>(mock_db_compositor, db);
    }

    MockDisplayBufferCompositor mock_db_compositor;

    MOCK_METHOD1(create_compositor_mock, void(mg::DisplayBuffer&));
};

struct StubDisplayBufferCompositor : mc::DisplayBufferCompositor
{
    void composite(mc::SceneElementSequence&&) override {}
};

struct StubDisplayBufferCompositorFactory : mc::DisplayBufferCompositorFactory
{
    std::unique_ptr<mc::DisplayBufferCompositor> create_compositor_for(mg::DisplayBuffer& db) override
    {
        return std::make_unique<WrappingDisplayBufferCompositor>(stub_db_compositor, db);
    }
    StubDisplayBufferCompositor stub_db_compositor;
};

MATCHER_P(DisplayBufferCoversArea, output_extents, "")
{
    return arg.view_area() == output_extents;
}

MATCHER_P(BufferPropertiesMatchSize, size, "")
{
    return arg.size == size;
}

struct CompositingScreencastTest : testing::Test
{
    CompositingScreencastTest()
        : screencast{mt::fake_shared(stub_scene),
                     mt::fake_shared(stub_display),
                     mt::fake_shared(stub_buffer_allocator),
                     mt::fake_shared(stub_db_compositor_factory)},
          default_size{1, 1},
          default_region{{0, 0}, {1, 1}},
          default_pixel_format{mir_pixel_format_xbgr_8888}
    {
    }

    testing::NiceMock<mtd::MockGL> mock_gl;
    mtd::StubScene stub_scene;
    StubDisplay stub_display;
    mtd::StubGLBufferAllocator stub_buffer_allocator;
    StubDisplayBufferCompositorFactory stub_db_compositor_factory;
    mc::CompositingScreencast screencast;
    geom::Size const default_size;
    geom::Rectangle const default_region;
    MirPixelFormat const default_pixel_format;
    int const default_num_buffers{2};
    MirMirrorMode const default_mirror_mode{mir_mirror_mode_vertical};
};

}

TEST_F(CompositingScreencastTest, produces_different_session_ids)
{
    std::unordered_set<mf::ScreencastSessionId> session_ids;

    for (int i = 0; i != 10; ++i)
    {
        auto session_id = screencast.create_session(
            default_region, default_size, default_pixel_format,
            default_num_buffers, default_mirror_mode);
        ASSERT_TRUE(session_ids.find(session_id) == session_ids.end())
            << "session_id: " << session_id << " iter: " << i;
        session_ids.insert(session_id);
    }
}

TEST_F(CompositingScreencastTest, throws_on_creation_with_invalid_params)
{
    std::unordered_set<mf::ScreencastSessionId> session_ids;
    geom::Size invalid_size{0, 0};
    geom::Rectangle invalid_region{{0, 0}, {0, 0}};

    EXPECT_THROW(screencast.create_session(invalid_region, default_size, default_pixel_format, default_num_buffers, default_mirror_mode), std::runtime_error);
    EXPECT_THROW(screencast.create_session(default_region, invalid_size, default_pixel_format, default_num_buffers, default_mirror_mode), std::runtime_error);
    EXPECT_THROW(screencast.create_session(default_region, default_size, mir_pixel_format_invalid, default_num_buffers, default_mirror_mode), std::runtime_error);
}

TEST_F(CompositingScreencastTest, throws_on_capture_with_invalid_session_id)
{
    mf::ScreencastSessionId const invalid_session_id{10};
    EXPECT_THROW(screencast.capture(invalid_session_id), std::logic_error);
}

TEST_F(CompositingScreencastTest, throws_on_capture_with_destroyed_session_id)
{
    auto session_id = screencast.create_session(
        default_region, default_size, default_pixel_format,
        default_num_buffers, default_mirror_mode);
    screencast.destroy_session(session_id);
    EXPECT_THROW(screencast.capture(session_id), std::logic_error);
}

TEST_F(CompositingScreencastTest, captures_by_compositing_with_provided_region)
{
    using namespace testing;

    mtd::StubSceneElement element1;
    mtd::StubSceneElement element2;
    mc::SceneElementSequence scene_elements{mt::fake_shared(element1), mt::fake_shared(element2)};
    NiceMock<mtd::MockScene> mock_scene;
    MockDisplayBufferCompositorFactory mock_db_compositor_factory;

    InSequence s;
    EXPECT_CALL(mock_db_compositor_factory,
                create_compositor_mock(DisplayBufferCoversArea(default_region)));
    EXPECT_CALL(mock_scene, scene_elements_for(_))
        .WillOnce(Return(scene_elements));
    EXPECT_CALL(mock_db_compositor_factory.mock_db_compositor, composite_(Eq(scene_elements)));

    mc::CompositingScreencast screencast_local{
        mt::fake_shared(mock_scene),
        mt::fake_shared(stub_display),
        mt::fake_shared(stub_buffer_allocator),
        mt::fake_shared(mock_db_compositor_factory)};

    auto session_id = screencast_local.create_session(
        default_region, default_size, default_pixel_format,
        default_num_buffers, default_mirror_mode);

    screencast_local.capture(session_id);
}

TEST_F(CompositingScreencastTest, captures_to_buffer_by_compositing)
{
    using namespace testing;

    mtd::StubGLBuffer stub_buffer;
    mtd::StubSceneElement element1;
    mtd::StubSceneElement element2;
    mc::SceneElementSequence scene_elements{mt::fake_shared(element1), mt::fake_shared(element2)};
    NiceMock<mtd::MockScene> mock_scene;
    MockDisplayBufferCompositorFactory mock_db_compositor_factory;

    InSequence s;
    EXPECT_CALL(mock_db_compositor_factory,
                create_compositor_mock(DisplayBufferCoversArea(default_region)));
    EXPECT_CALL(mock_scene, scene_elements_for(_))
        .WillOnce(Return(scene_elements));
    EXPECT_CALL(mock_db_compositor_factory.mock_db_compositor, composite_(Eq(scene_elements)));

    mc::CompositingScreencast screencast{
        mt::fake_shared(mock_scene),
        mt::fake_shared(stub_display),
        mt::fake_shared(stub_buffer_allocator),
        mt::fake_shared(mock_db_compositor_factory)};

    auto session_id = screencast.create_session(
        default_region, default_size, default_pixel_format,
        0, default_mirror_mode);

    screencast.capture(session_id, mt::fake_shared(stub_buffer));
}

TEST_F(CompositingScreencastTest, captures_to_buffer_by_compositing_with_reserve_buffers)
{
    using namespace testing;

    mtd::StubGLBuffer stub_buffer;
    NiceMock<mtd::MockScene> mock_scene;
    MockDisplayBufferCompositorFactory mock_db_compositor_factory;
    mc::CompositingScreencast screencast{
        mt::fake_shared(mock_scene),
        mt::fake_shared(stub_display),
        mt::fake_shared(stub_buffer_allocator),
        mt::fake_shared(mock_db_compositor_factory)};

    auto session_id = screencast.create_session(
        default_region, default_size, default_pixel_format,
        default_num_buffers, default_mirror_mode);

    screencast.capture(session_id, mt::fake_shared(stub_buffer));
}

TEST_F(CompositingScreencastTest, allocates_and_uses_buffer_with_provided_size)
{
    using namespace testing;

    MockBufferAllocator mock_buffer_allocator;
    mtd::StubGLBuffer stub_buffer;

    InSequence s;
    EXPECT_CALL(mock_buffer_allocator,
                alloc_buffer(BufferPropertiesMatchSize(default_size)))
        .WillOnce(Return(mt::fake_shared(stub_buffer)));

    mc::CompositingScreencast screencast_local{
        mt::fake_shared(stub_scene),
        mt::fake_shared(stub_display),
        mt::fake_shared(mock_buffer_allocator),
        mt::fake_shared(stub_db_compositor_factory)};

    auto session_id = screencast_local.create_session(
        default_region, default_size, default_pixel_format,
        1, default_mirror_mode);

    auto buffer = screencast_local.capture(session_id);
    ASSERT_EQ(&stub_buffer, buffer.get());
}

TEST_F(CompositingScreencastTest, allocates_different_buffers_per_session)
{
    using namespace testing;

    MockBufferAllocator mock_buffer_allocator;
    mtd::StubGLBuffer stub_buffer1;
    mtd::StubGLBuffer stub_buffer2;

    EXPECT_CALL(mock_buffer_allocator, alloc_buffer(_))
        .WillOnce(Return(mt::fake_shared(stub_buffer1)))
        .WillOnce(Return(mt::fake_shared(stub_buffer2)));

    mc::CompositingScreencast screencast_local{
        mt::fake_shared(stub_scene),
        mt::fake_shared(stub_display),
        mt::fake_shared(mock_buffer_allocator),
        mt::fake_shared(stub_db_compositor_factory)};

    auto session_id1 = screencast_local.create_session(
        default_region, default_size, default_pixel_format,
        1, default_mirror_mode);
    auto buffer1 = screencast_local.capture(session_id1);
    ASSERT_EQ(&stub_buffer1, buffer1.get());
    buffer1 = screencast_local.capture(session_id1);
    ASSERT_EQ(&stub_buffer1, buffer1.get());

    auto session_id2 = screencast_local.create_session(
        default_region, default_size, default_pixel_format,
        1, default_mirror_mode);
    auto buffer2 = screencast_local.capture(session_id2);
    ASSERT_EQ(&stub_buffer2, buffer2.get());
    buffer2 = screencast_local.capture(session_id2);
    ASSERT_EQ(&stub_buffer2, buffer2.get());
}

TEST_F(CompositingScreencastTest, registers_and_unregisters_from_scene)
{
    using namespace testing;
    NiceMock<mtd::MockScene> mock_scene;
    NiceMock<MockBufferAllocator> mock_buffer_allocator;

    ON_CALL(mock_buffer_allocator, alloc_buffer(_))
        .WillByDefault(Return(std::make_shared<mtd::StubGLBuffer>()));

    EXPECT_CALL(mock_scene, register_compositor(_))
        .Times(1);
    EXPECT_CALL(mock_scene, unregister_compositor(_))
        .Times(1);

    mc::CompositingScreencast screencast_local{
        mt::fake_shared(mock_scene),
        mt::fake_shared(stub_display),
        mt::fake_shared(mock_buffer_allocator),
        mt::fake_shared(stub_db_compositor_factory)};

    auto session_id = screencast_local.create_session(
        default_region, default_size, default_pixel_format,
        default_num_buffers, default_mirror_mode);
    screencast_local.destroy_session(session_id);
}

TEST_F(CompositingScreencastTest, attempts_to_create_output_display_when_given_region_outside_any_display)
{
    using namespace testing;

    geom::Rectangle region_outside_display{stub_display.adjacent_point(), default_size};

    mc::CompositingScreencast screencast_local{
        mt::fake_shared(stub_scene),
        mt::fake_shared(stub_display),
        mt::fake_shared(stub_buffer_allocator),
        mt::fake_shared(stub_db_compositor_factory)};

    auto session_id = screencast_local.create_session(
            region_outside_display, default_size, default_pixel_format,
            default_num_buffers, default_mirror_mode);
    screencast_local.destroy_session(session_id);

    EXPECT_TRUE(stub_display.virtual_output_created);
    EXPECT_TRUE(stub_display.virtual_output_enabled);
}

TEST_F(CompositingScreencastTest, does_not_create_virtual_output_when_given_region_that_covers_any_display)
{
    using namespace testing;

    geom::Rectangle region_inside_display{stub_display.extents().top_left, default_size};

    mc::CompositingScreencast screencast_local{
        mt::fake_shared(stub_scene),
        mt::fake_shared(stub_display),
        mt::fake_shared(stub_buffer_allocator),
        mt::fake_shared(stub_db_compositor_factory)};

    auto session_id = screencast_local.create_session(
            region_inside_display, default_size, default_pixel_format,
            default_num_buffers, default_mirror_mode);
    screencast_local.destroy_session(session_id);

    EXPECT_FALSE(stub_display.virtual_output_created);
    EXPECT_FALSE(stub_display.virtual_output_enabled);
}


TEST_F(CompositingScreencastTest, uses_requested_number_of_buffers)
{
    using namespace testing;

    MockBufferAllocator mock_buffer_allocator;
    int const expected_num_buffers = 4;
    std::vector<mtd::StubGLBuffer> buffers(expected_num_buffers);


    EXPECT_CALL(mock_buffer_allocator, alloc_buffer(_))
        .WillOnce(Return(mt::fake_shared(buffers[0])))
        .WillOnce(Return(mt::fake_shared(buffers[1])))
        .WillOnce(Return(mt::fake_shared(buffers[2])))
        .WillOnce(Return(mt::fake_shared(buffers[3])));

    mc::CompositingScreencast screencast_local{
        mt::fake_shared(stub_scene),
        mt::fake_shared(stub_display),
        mt::fake_shared(mock_buffer_allocator),
        mt::fake_shared(stub_db_compositor_factory)};

    auto session_id = screencast_local.create_session(
        default_region, default_size, default_pixel_format,
        expected_num_buffers, default_mirror_mode);

    for (int i = 0; i < expected_num_buffers; i++)
    {
        auto buffer = screencast_local.capture(session_id);
        ASSERT_EQ(&buffers[i], buffer.get());
    }
}


