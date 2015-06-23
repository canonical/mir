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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "src/server/compositor/compositing_screencast.h"
#include "mir/compositor/display_buffer_compositor_factory.h"
#include "mir/compositor/display_buffer_compositor.h"
#include "mir/compositor/scene.h"
#include "mir/graphics/display_buffer.h"
#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/geometry/rectangle.h"

#include "mir/test/doubles/null_display.h"
#include "mir/test/doubles/null_display_buffer_compositor_factory.h"
#include "mir/test/doubles/stub_display_configuration.h"
#include "mir/test/doubles/stub_buffer_allocator.h"
#include "mir/test/doubles/mock_buffer.h"
#include "mir/test/doubles/mock_gl.h"
#include "mir/test/doubles/stub_scene.h"
#include "mir/test/doubles/stub_scene_element.h"
#include "mir/test/doubles/mock_scene.h"

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
namespace geom = mir::geometry;

namespace
{

class StubDisplay : public mtd::NullDisplay
{
public:
    StubDisplay()
        : connected_used_outputs{{connected,!used}, {connected,used}},
          stub_config{connected_used_outputs}
    {
    }

    std::unique_ptr<mg::DisplayConfiguration> configuration() const override
    {
        return std::unique_ptr<mg::DisplayConfiguration>{
            new mtd::StubDisplayConfig{connected_used_outputs}};
    }

    mg::DisplayConfigurationOutput const& output_with(mg::DisplayConfigurationOutputId id)
    {
        for (auto const& output : stub_config.outputs)
        {
            if (output.id == id)
                return output;
        }

        BOOST_THROW_EXCEPTION(std::logic_error("Can't find stub output"));
    }

private:
    std::vector<std::pair<bool,bool>> const connected_used_outputs;
    mtd::StubDisplayConfig const stub_config;
    static bool const connected;
    static bool const used;
};

bool const StubDisplay::connected{true};
bool const StubDisplay::used{true};

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
    WrappingDisplayBufferCompositor(mc::DisplayBufferCompositor& comp)
        : comp(comp)
    {
    }

    void composite(mc::SceneElementSequence&& elements)
    {
        comp.composite(std::move(elements));
    }

private:
    mc::DisplayBufferCompositor& comp;
};


struct MockBufferAllocator : mg::GraphicBufferAllocator
{
    MOCK_METHOD1(alloc_buffer,
                 std::shared_ptr<mg::Buffer>(mg::BufferProperties const&));
    MOCK_METHOD0(supported_pixel_formats,
                 std::vector<MirPixelFormat>());
};

struct MockDisplayBufferCompositorFactory : mc::DisplayBufferCompositorFactory
{
    std::unique_ptr<mc::DisplayBufferCompositor> create_compositor_for(mg::DisplayBuffer& db)
    {
        create_compositor_mock(db);
        return std::unique_ptr<WrappingDisplayBufferCompositor>(
            new WrappingDisplayBufferCompositor{mock_db_compositor});
    }

    MockDisplayBufferCompositor mock_db_compositor;

    MOCK_METHOD1(create_compositor_mock, void(mg::DisplayBuffer&));
};

MATCHER_P(DisplayBufferCoversArea, output_extents, "")
{
    return arg.view_area() == output_extents;
}

MATCHER_P(BufferPropertiesMatchOutput, output, "")
{
    return arg.size == output.extents().size;
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
    mtd::StubBufferAllocator stub_buffer_allocator;
    mtd::NullDisplayBufferCompositorFactory stub_db_compositor_factory;
    mc::CompositingScreencast screencast;
    geom::Size const default_size;
    geom::Rectangle const default_region;
    MirPixelFormat default_pixel_format;
};

}

TEST_F(CompositingScreencastTest, produces_different_session_ids)
{
    std::unordered_set<mf::ScreencastSessionId> session_ids;

    for (int i = 0; i != 10; ++i)
    {
        auto session_id = screencast.create_session(default_region, default_size, default_pixel_format);
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

    EXPECT_THROW(screencast.create_session(invalid_region, default_size, default_pixel_format), std::runtime_error);
    EXPECT_THROW(screencast.create_session(default_region, invalid_size, default_pixel_format), std::runtime_error);
    EXPECT_THROW(screencast.create_session(default_region, default_size, mir_pixel_format_invalid), std::runtime_error);
}

TEST_F(CompositingScreencastTest, throws_on_capture_with_invalid_session_id)
{
    mf::ScreencastSessionId const invalid_session_id{10};
    EXPECT_THROW(screencast.capture(invalid_session_id), std::logic_error);
}

TEST_F(CompositingScreencastTest, throws_on_capture_with_destroyed_session_id)
{
    auto session_id = screencast.create_session(default_region, default_size, default_pixel_format);
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

    auto session_id = screencast_local.create_session(default_region, default_size, default_pixel_format);

    screencast_local.capture(session_id);
}

TEST_F(CompositingScreencastTest, allocates_and_uses_buffer_with_provided_size)
{
    using namespace testing;

    MockBufferAllocator mock_buffer_allocator;
    mtd::StubBuffer stub_buffer;

    InSequence s;
    EXPECT_CALL(mock_buffer_allocator,
                alloc_buffer(BufferPropertiesMatchSize(default_size)))
        .WillOnce(Return(mt::fake_shared(stub_buffer)));

    mc::CompositingScreencast screencast_local{
        mt::fake_shared(stub_scene),
        mt::fake_shared(stub_display),
        mt::fake_shared(mock_buffer_allocator),
        mt::fake_shared(stub_db_compositor_factory)};

    auto session_id = screencast_local.create_session(default_region, default_size, default_pixel_format);

    auto buffer = screencast_local.capture(session_id);
    ASSERT_EQ(&stub_buffer, buffer.get());
}

TEST_F(CompositingScreencastTest, uses_one_buffer_per_session)
{
    using namespace testing;

    MockBufferAllocator mock_buffer_allocator;
    mtd::StubBuffer stub_buffer1;
    mtd::StubBuffer stub_buffer2;

    EXPECT_CALL(mock_buffer_allocator, alloc_buffer(_))
        .WillOnce(Return(mt::fake_shared(stub_buffer1)))
        .WillOnce(Return(mt::fake_shared(stub_buffer2)));

    mc::CompositingScreencast screencast_local{
        mt::fake_shared(stub_scene),
        mt::fake_shared(stub_display),
        mt::fake_shared(mock_buffer_allocator),
        mt::fake_shared(stub_db_compositor_factory)};

    auto session_id1 = screencast_local.create_session(default_region, default_size, default_pixel_format);
    auto buffer1 = screencast_local.capture(session_id1);
    ASSERT_EQ(&stub_buffer1, buffer1.get());
    buffer1 = screencast_local.capture(session_id1);
    ASSERT_EQ(&stub_buffer1, buffer1.get());

    auto session_id2 = screencast_local.create_session(default_region, default_size, default_pixel_format);
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
        .WillByDefault(Return(std::make_shared<mtd::StubBuffer>()));

    EXPECT_CALL(mock_scene, register_compositor(_))
        .Times(1);
    EXPECT_CALL(mock_scene, unregister_compositor(_))
        .Times(1);

    mc::CompositingScreencast screencast_local{
        mt::fake_shared(mock_scene),
        mt::fake_shared(stub_display),
        mt::fake_shared(mock_buffer_allocator),
        mt::fake_shared(stub_db_compositor_factory)};

    auto session_id = screencast_local.create_session(default_region, default_size, default_pixel_format);
    screencast_local.destroy_session(session_id);
}
