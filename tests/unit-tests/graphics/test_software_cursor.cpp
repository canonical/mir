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

#include "src/server/graphics/software_cursor.h"
#include "mir/graphics/cursor_image.h"
#include "mir/graphics/renderable.h"

#include "mir/test/doubles/stub_buffer_allocator.h"
#include "mir/test/doubles/stub_input_scene.h"
#include "mir/test/doubles/explicit_executor.h"

#include "mir/test/fake_shared.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mg = mir::graphics;
namespace mtd = mir::test::doubles;
namespace mt = mir::test;
namespace geom = mir::geometry;

namespace
{

struct MockInputScene : mtd::StubInputScene
{
    MOCK_METHOD1(add_input_visualization,
                 void(std::shared_ptr<mg::Renderable> const&));

    MOCK_METHOD1(remove_input_visualization,
                 void(std::weak_ptr<mg::Renderable> const&));

    MOCK_METHOD0(emit_scene_changed, void());
};

struct StubCursorImage : mg::CursorImage
{
    StubCursorImage(geom::Displacement const& hotspot)
        : hotspot_{hotspot},
          pixels(
            size().width.as_uint32_t() * size().height.as_uint32_t() * bytes_per_pixel,
            0x55)
    {
    }

    void const* as_argb_8888() const
    {
        return pixels.data();
    }

    geom::Size size() const
    {
        return {64, 64};
    }

    geom::Displacement hotspot() const
    {
        return hotspot_;
    }

private:
    static size_t const bytes_per_pixel = 4;
    geom::Displacement const hotspot_;
    std::vector<unsigned char> pixels;
};

class ExplicitExectutor : public mtd::ExplicitExectutor
{
public:
    ~ExplicitExectutor()
    {
        execute();
    }
};

struct SoftwareCursor : testing::Test
{
    StubCursorImage stub_cursor_image{{3,4}};
    StubCursorImage another_stub_cursor_image{{10,9}};
    mtd::StubBufferAllocator stub_buffer_allocator;
    testing::NiceMock<MockInputScene> mock_input_scene;
    ExplicitExectutor executor;

    mg::SoftwareCursor cursor{
        mt::fake_shared(stub_buffer_allocator),
        mt::fake_shared(executor),
        mt::fake_shared(mock_input_scene)};
};

MATCHER_P(RenderableWithPosition, s, "")
{
    return s == arg->screen_position().top_left;
}

MATCHER_P(RenderableWithSize, s, "")
{
    return s == arg->screen_position().size;
}

MATCHER_P(WeakPtrEq, sp, "")
{
    return sp == arg.lock();
}

ACTION_TEMPLATE(SavePointerToArg,
                HAS_1_TEMPLATE_PARAMS(int, k),
                AND_1_VALUE_PARAMS(output))
{
    *output = &std::get<k>(args);
}

}

TEST_F(SoftwareCursor, is_added_to_scene_when_shown)
{
    using namespace testing;

    EXPECT_CALL(mock_input_scene, add_input_visualization(_));

    cursor.show(stub_cursor_image);
}

TEST_F(SoftwareCursor, is_removed_from_scene_on_destruction)
{
    using namespace testing;

    InSequence s;
    EXPECT_CALL(mock_input_scene, add_input_visualization(_));
    EXPECT_CALL(mock_input_scene, remove_input_visualization(_));

    cursor.show(stub_cursor_image);
}

TEST_F(SoftwareCursor, tolerates_being_hidden_while_being_shown)
{
    using namespace testing;

    InSequence s;
    EXPECT_CALL(mock_input_scene, add_input_visualization(_))
        .WillOnce(Invoke([&](auto)
            {
                cursor.hide(); // should do nothing
            }));
    EXPECT_CALL(mock_input_scene, remove_input_visualization(_)).Times(0);

    cursor.show(stub_cursor_image);
    executor.execute();

    Mock::VerifyAndClearExpectations(&mock_input_scene);
}

TEST_F(SoftwareCursor, tolerates_being_hidden_while_being_reshown)
{
    using namespace testing;

    InSequence s;
    EXPECT_CALL(mock_input_scene, add_input_visualization(_));
    EXPECT_CALL(mock_input_scene, remove_input_visualization(_));
    EXPECT_CALL(mock_input_scene, add_input_visualization(_))
        .WillOnce(Invoke([&](auto)
            {
                cursor.hide(); // should do nothing
            }));

    cursor.show(stub_cursor_image);
    executor.execute();
    cursor.hide();
    executor.execute();
    cursor.show(stub_cursor_image);
    executor.execute();

    Mock::VerifyAndClearExpectations(&mock_input_scene);
}

TEST_F(SoftwareCursor, is_removed_from_scene_when_hidden)
{
    using namespace testing;

    InSequence s;
    EXPECT_CALL(mock_input_scene, add_input_visualization(_));
    EXPECT_CALL(mock_input_scene, remove_input_visualization(_));

    cursor.show(stub_cursor_image);
    executor.execute();
    cursor.hide();
    executor.execute();

    Mock::VerifyAndClearExpectations(&mock_input_scene);
}

TEST_F(SoftwareCursor, renderable_has_cursor_size)
{
    using namespace testing;

    EXPECT_CALL(mock_input_scene,
                add_input_visualization(
                    RenderableWithSize(stub_cursor_image.size())));

    cursor.show(stub_cursor_image);
}

TEST_F(SoftwareCursor, places_renderable_at_origin_offset_by_hotspot)
{
    using namespace testing;

    auto const pos = geom::Point{0,0} - stub_cursor_image.hotspot();

    EXPECT_CALL(mock_input_scene,
                add_input_visualization(RenderableWithPosition(pos)));

    cursor.show(stub_cursor_image);
}

TEST_F(SoftwareCursor, moves_scene_renderable_offset_by_hotspot_when_moved)
{
    using namespace testing;

    std::shared_ptr<mg::Renderable> cursor_renderable;

    EXPECT_CALL(mock_input_scene, add_input_visualization(_))
        .WillOnce(SaveArg<0>(&cursor_renderable));

    cursor.show(stub_cursor_image);
    executor.execute();

    geom::Point const new_position{12,34};
    cursor.move_to(new_position);
    executor.execute();

    EXPECT_THAT(cursor_renderable->screen_position().top_left,
                Eq(new_position - stub_cursor_image.hotspot()));
}

TEST_F(SoftwareCursor, notifies_scene_when_moving)
{
    using namespace testing;

    EXPECT_CALL(mock_input_scene, emit_scene_changed());

    cursor.show(stub_cursor_image);
    executor.execute();
    cursor.move_to({22,23});
}

TEST_F(SoftwareCursor, creates_renderable_with_filled_buffer)
{
    using namespace testing;

    size_t const image_size =
        4 * stub_cursor_image.size().width.as_uint32_t() *
        stub_cursor_image.size().height.as_uint32_t();
    auto const image_data =
        static_cast<unsigned char const*>(stub_cursor_image.as_argb_8888());

    std::shared_ptr<mg::Renderable> cursor_renderable;

    EXPECT_CALL(mock_input_scene, add_input_visualization(_)).
        WillOnce(SaveArg<0>(&cursor_renderable));

    cursor.show(stub_cursor_image);
    executor.execute();

    auto buffer = static_cast<mtd::StubBuffer*>(cursor_renderable->buffer().get());

    EXPECT_THAT(buffer->written_pixels, ElementsAreArray(image_data, image_size));
}

TEST_F(SoftwareCursor, does_not_hide_or_move_when_already_hidden)
{
    using namespace testing;

    EXPECT_CALL(mock_input_scene, remove_input_visualization(_)).Times(0);
    EXPECT_CALL(mock_input_scene, emit_scene_changed()).Times(0);

    // Already hidden, nothing should happen
    cursor.hide();
    executor.execute();
    // Hidden, nothing should happen
    cursor.move_to({3,4});
}

TEST_F(SoftwareCursor, creates_new_renderable_for_new_cursor_image)
{
    using namespace testing;

    std::shared_ptr<mg::Renderable> first_cursor_renderable;

    EXPECT_CALL(mock_input_scene, add_input_visualization(_)).
        WillOnce(SaveArg<0>(&first_cursor_renderable));

    cursor.show(stub_cursor_image);
    executor.execute();

    Mock::VerifyAndClearExpectations(&mock_input_scene);

    EXPECT_CALL(mock_input_scene,
                remove_input_visualization(WeakPtrEq(first_cursor_renderable)));
    EXPECT_CALL(mock_input_scene, add_input_visualization(Ne(first_cursor_renderable)));

    cursor.show(another_stub_cursor_image);
    executor.execute();

    Mock::VerifyAndClearExpectations(&mock_input_scene);
}

TEST_F(SoftwareCursor, places_new_cursor_renderable_at_correct_position)
{
    using namespace testing;

    auto const cursor_position = geom::Point{3, 4};

    cursor.show(stub_cursor_image);
    executor.execute();
    cursor.move_to(cursor_position);
    executor.execute();

    Mock::VerifyAndClearExpectations(&mock_input_scene);

    auto const renderable_position =
        cursor_position - another_stub_cursor_image.hotspot();
    EXPECT_CALL(mock_input_scene,
                add_input_visualization(RenderableWithPosition(renderable_position)));

    cursor.show(another_stub_cursor_image);
}

//lp: #1413211
TEST_F(SoftwareCursor, new_buffer_on_each_show)
{
    struct MockBufferAllocator : public mg::GraphicBufferAllocator
    {
        MOCK_METHOD1(alloc_buffer, std::shared_ptr<mg::Buffer>(mg::BufferProperties const&));
        MOCK_METHOD2(alloc_software_buffer, std::shared_ptr<mg::Buffer>(geom::Size, MirPixelFormat));
        MOCK_METHOD3(alloc_buffer, std::shared_ptr<mg::Buffer>(geom::Size, uint32_t, uint32_t));
        std::vector<MirPixelFormat> supported_pixel_formats() { return {mir_pixel_format_abgr_8888}; } 
    } mock_allocator;

    EXPECT_CALL(mock_allocator, alloc_software_buffer(testing::_, testing::_))
        .Times(3)
        .WillRepeatedly(testing::Return(std::make_shared<mtd::StubBuffer>()));;
    mg::SoftwareCursor cursor{
        mt::fake_shared(mock_allocator),
        mt::fake_shared(executor),
        mt::fake_shared(mock_input_scene)};
    cursor.show(another_stub_cursor_image);
    cursor.show(another_stub_cursor_image);
    cursor.show(stub_cursor_image);
}

//lp: 1483779
TEST_F(SoftwareCursor, doesnt_try_to_remove_after_hiding)
{
    using namespace testing;

    Sequence seq;
    EXPECT_CALL(mock_input_scene, add_input_visualization(_))
        .InSequence(seq);
    EXPECT_CALL(mock_input_scene, remove_input_visualization(_))
        .InSequence(seq);
    EXPECT_CALL(mock_input_scene, add_input_visualization(_))
        .InSequence(seq);
    cursor.show(stub_cursor_image);
    executor.execute();
    cursor.hide(); //should remove here
    executor.execute();
    cursor.show(stub_cursor_image); //should add, but not remove a second time
    executor.execute();
    Mock::VerifyAndClearExpectations(&mock_input_scene);
}
