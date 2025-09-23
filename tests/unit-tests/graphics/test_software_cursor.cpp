/*
 * Copyright © Canonical Ltd.
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
 */

#include "src/server/graphics/software_cursor.h"
#include "mir/graphics/cursor_image.h"
#include "mir/graphics/renderable.h"

#include "mir/test/doubles/stub_buffer_allocator.h"
#include "mir/test/doubles/stub_buffer.h"
#include "mir/test/doubles/stub_input_scene.h"
#include "mir/test/doubles/explicit_executor.h"
#include "mir/test/doubles/mock_input_scene.h"

#include "mir/test/fake_shared.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>

namespace mg = mir::graphics;
namespace mtd = mir::test::doubles;
namespace mt = mir::test;
namespace geom = mir::geometry;
namespace mrs = mir::renderer::software;

namespace
{

struct StubCursorImage : mg::CursorImage
{
    StubCursorImage(geom::Displacement const& hotspot)
        : hotspot_{hotspot},
          pixels(
            size().width.as_uint32_t() * size().height.as_uint32_t() * bytes_per_pixel,
            std::byte{0x55})
    {
    }

    void const* as_argb_8888() const override
    {
        return pixels.data();
    }

    geom::Size size() const override
    {
        return {64, 64};
    }

    geom::Displacement hotspot() const override
    {
        return hotspot_;
    }

    void fill_with(
        unsigned char r,
        unsigned char g,
        unsigned char b,
        unsigned char a)
    {
        uint32_t const fill_colour =
            (static_cast<uint32_t>(a) << 24) |
            (static_cast<uint32_t>(r) << 16) |
            (static_cast<uint32_t>(g) << 8) |
            (static_cast<uint32_t>(b) << 0);

        std::fill(
            reinterpret_cast<uint32_t*>(&*pixels.begin()),
            reinterpret_cast<uint32_t*>(&*pixels.end()),
            fill_colour);
    }

private:
    static size_t const bytes_per_pixel = 4;
    geom::Displacement const hotspot_;
    std::vector<std::byte> pixels;
};

class MockBufferAllocator : public mtd::StubBufferAllocator
{
public:
    MockBufferAllocator()
    {
        using namespace testing;
        ON_CALL(*this, supported_pixel_formats())
            .WillByDefault(Return(std::vector<MirPixelFormat>{ mir_pixel_format_argb_8888 }));
        ON_CALL(*this, alloc_software_buffer(_, _))
            .WillByDefault(
                Invoke(
                    [this](auto sz, auto pf)
                    {
                        return this->mtd::StubBufferAllocator::alloc_software_buffer(sz, pf);
                    }));
    }

    MOCK_METHOD2(alloc_software_buffer, std::shared_ptr<mg::Buffer>(geom::Size, MirPixelFormat));
    MOCK_METHOD0(supported_pixel_formats, std::vector<MirPixelFormat>());
};

class ExplicitExecutor : public mtd::ExplicitExecutor
{
public:
    ~ExplicitExecutor()
    {
        execute();
    }
};

struct SoftwareCursor : testing::Test
{
    std::shared_ptr<StubCursorImage> stub_cursor_image = std::make_shared<StubCursorImage>(geom::Displacement{3, 4});
    std::shared_ptr<StubCursorImage> another_stub_cursor_image = std::make_shared<StubCursorImage>(geom::Displacement{10, 9});
    testing::NiceMock<MockBufferAllocator> mock_buffer_allocator;
    testing::NiceMock<mtd::MockInputScene> mock_input_scene;

    mg::SoftwareCursor cursor{
        mt::fake_shared(mock_buffer_allocator),
        mt::fake_shared(mock_input_scene)};
};

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

TEST_F(SoftwareCursor, scene_changed_is_emitted_when_shown)
{
    using namespace testing;

    EXPECT_CALL(mock_input_scene, emit_scene_changed());

    cursor.show(stub_cursor_image);
}

TEST_F(SoftwareCursor, tolerates_being_hidden_while_being_shown)
{
    using namespace testing;

    InSequence s;
    EXPECT_CALL(mock_input_scene, emit_scene_changed())
        .WillOnce(Invoke([&]
            {
                cursor.hide();
            }));
    EXPECT_CALL(mock_input_scene, emit_scene_changed());

    cursor.show(stub_cursor_image);

    Mock::VerifyAndClearExpectations(&mock_input_scene);
}

TEST_F(SoftwareCursor, tolerates_being_hidden_while_being_reshown)
{
    using namespace testing;

    InSequence s;
    EXPECT_CALL(mock_input_scene, emit_scene_changed()); // Show
    EXPECT_CALL(mock_input_scene, emit_scene_changed()); // Hide
    EXPECT_CALL(mock_input_scene, emit_scene_changed()) // Reshow
        .WillOnce(Invoke([&]
            {
                cursor.hide();
            }));
    EXPECT_CALL(mock_input_scene, emit_scene_changed()); // Rehide

    cursor.show(stub_cursor_image);
    cursor.hide();
    cursor.show(stub_cursor_image);

    Mock::VerifyAndClearExpectations(&mock_input_scene);
}

TEST_F(SoftwareCursor, renderable_has_cursor_size)
{
    using namespace testing;
    cursor.show(stub_cursor_image);
    EXPECT_THAT(cursor.renderable()->screen_position().size, Eq(stub_cursor_image->size()));
}

TEST_F(SoftwareCursor, places_renderable_at_origin_offset_by_hotspot)
{
    using namespace testing;
    auto const pos = geom::Point{0,0} - stub_cursor_image->hotspot();
    cursor.show(stub_cursor_image);
    EXPECT_THAT(cursor.renderable()->screen_position().top_left, Eq(pos));
}

TEST_F(SoftwareCursor, moves_scene_renderable_offset_by_hotspot_when_moved)
{
    using namespace testing;

    cursor.show(stub_cursor_image);

    geom::Point const new_position{12,34};
    cursor.move_to(new_position);

    EXPECT_THAT(cursor.renderable()->screen_position().top_left,
                Eq(new_position - stub_cursor_image->hotspot()));
}

TEST_F(SoftwareCursor, notifies_scene_when_moving)
{
    using namespace testing;

    EXPECT_CALL(mock_input_scene, emit_scene_changed())
        .Times(2);

    cursor.show(stub_cursor_image);
    cursor.move_to({22,23});
}

TEST_F(SoftwareCursor, creates_renderable_with_filled_buffer)
{
    using namespace testing;

    size_t const image_size =
        4 * stub_cursor_image->size().width.as_uint32_t() *
        stub_cursor_image->size().height.as_uint32_t();
    auto const image_data =
        static_cast<std::byte const*>(stub_cursor_image->as_argb_8888());

    cursor.show(stub_cursor_image);

    auto const buffer = static_cast<mtd::StubBuffer*>(cursor.renderable()->buffer().get());
    EXPECT_THAT(buffer->written_pixels, ElementsAreArray(image_data, image_size));
}

TEST_F(SoftwareCursor, does_not_hide_or_move_when_already_hidden)
{
    using namespace testing;

    EXPECT_CALL(mock_input_scene, emit_scene_changed()).Times(0);

    // Already hidden, nothing should happen
    cursor.hide();
    // Hidden, nothing should happen
    cursor.move_to({3,4});
}

TEST_F(SoftwareCursor, creates_new_renderable_for_new_cursor_image)
{
    using namespace testing;

    cursor.show(stub_cursor_image);
    auto const first_cursor_renderable = cursor.renderable();

    cursor.show(another_stub_cursor_image);
    EXPECT_THAT(cursor.renderable(), Ne(first_cursor_renderable));
}

TEST_F(SoftwareCursor, places_new_cursor_renderable_at_correct_position)
{
    using namespace testing;

    auto const cursor_position = geom::Point{3, 4};

    cursor.show(stub_cursor_image);
    cursor.move_to(cursor_position);

    auto const renderable_position =
        cursor_position - another_stub_cursor_image->hotspot();

    cursor.show(another_stub_cursor_image);
    EXPECT_THAT(cursor.renderable()->screen_position().top_left,
                Eq(renderable_position));
}

//lp: #1413211
TEST_F(SoftwareCursor, new_buffer_on_each_show)
{
    EXPECT_CALL(mock_buffer_allocator, alloc_software_buffer(testing::_, testing::_))
        .Times(3);
    mg::SoftwareCursor cursor{
        mt::fake_shared(mock_buffer_allocator),
        mt::fake_shared(mock_input_scene)};
    cursor.show(another_stub_cursor_image);
    cursor.show(another_stub_cursor_image);
    cursor.show(stub_cursor_image);
}

//lp: 1483779
TEST_F(SoftwareCursor, doesnt_try_to_remove_after_hiding)
{
    using namespace testing;

    EXPECT_CALL(mock_input_scene, emit_scene_changed())
        .Times(3);
    cursor.show(stub_cursor_image);
    cursor.hide(); //should remove here
    cursor.show(stub_cursor_image); //should add, but not remove a second time
    Mock::VerifyAndClearExpectations(&mock_input_scene);
}


TEST_F(SoftwareCursor, handles_argb_8888_cursor_surface)
{
    using namespace testing;

    ON_CALL(mock_buffer_allocator, supported_pixel_formats())
        .WillByDefault(Return(std::vector<MirPixelFormat>{ mir_pixel_format_argb_8888 }));

    auto const test_image = std::make_shared<StubCursorImage>(geom::Displacement{8, 8});
    unsigned char const r = 0x11;
    unsigned char const g = 0x55;
    unsigned char const b = 0xbb;
    unsigned char const a = 0xaa;
    test_image->fill_with(r, g, b, a);

    std::shared_ptr<mg::Buffer> cursor_buffer;
    EXPECT_CALL(mock_buffer_allocator, alloc_software_buffer(_,mir_pixel_format_argb_8888))
        .Times(1)
        .WillOnce(
            Invoke(
                [this, &cursor_buffer](auto sz, auto pf)
                {
                   auto buffer = mock_buffer_allocator.mtd::StubBufferAllocator::alloc_software_buffer(sz, pf);
                   cursor_buffer = buffer;
                   return buffer;
                }));


    mg::SoftwareCursor cursor{
        mt::fake_shared(mock_buffer_allocator),
        mt::fake_shared(mock_input_scene)
    };
    cursor.show(test_image);
    ASSERT_THAT(cursor_buffer, NotNull());

    uint32_t const expected_pixel =
        (static_cast<uint32_t>(a) << 24) |
        (static_cast<uint32_t>(r) << 16) |
        (static_cast<uint32_t>(g) << 8)  |
        (static_cast<uint32_t>(b) << 0);

    auto const mapping = cursor_buffer->map_readable();
    ASSERT_THAT(mapping->format(), Eq(mir_pixel_format_argb_8888));

    auto const stride = mapping->stride().as_uint32_t();
    for (auto y = 0u; y < mapping->size().height.as_uint32_t(); ++y)
    {
        auto const line = std::vector<uint32_t>{
            reinterpret_cast<uint32_t const*>(mapping->data() + (stride * y)),
            reinterpret_cast<uint32_t const*>(mapping->data() + (stride * y) + mapping->size().width.as_uint32_t())};
        EXPECT_THAT(line, Each(Eq(expected_pixel)));
    }
}

TEST_F(SoftwareCursor, handles_argb_8888_buffer_with_stride)
{
    using namespace testing;

    ON_CALL(mock_buffer_allocator, supported_pixel_formats())
        .WillByDefault(Return(std::vector<MirPixelFormat>{ mir_pixel_format_argb_8888 }));

    auto const test_image = std::make_shared<StubCursorImage>(geom::Displacement{8, 8});
    unsigned char const r = 0x42;
    unsigned char const g = 0x39;
    unsigned char const b = 0xce;
    unsigned char const a = 0xdf;
    test_image->fill_with(r, g, b, a);

    std::shared_ptr<mg::Buffer> cursor_buffer;
    EXPECT_CALL(mock_buffer_allocator, alloc_software_buffer(_,mir_pixel_format_argb_8888))
        .Times(1)
        .WillOnce(
            Invoke(
                [&cursor_buffer](auto sz, auto pf)
                {
                    // Set a stride that's not a multiple of the pixel width,
                    // for maximum testing.
                    geom::Stride const stride{
                        sz.width.as_uint32_t() * MIR_BYTES_PER_PIXEL(pf) + 41
                    };
                    auto buffer = std::make_shared<mtd::StubBuffer>(
                        mg::BufferProperties{
                            sz,
                            pf,
                            mg::BufferUsage::hardware
                        },
                        stride);
                    cursor_buffer = buffer;
                    return buffer;
                }));

    mg::SoftwareCursor cursor{
        mt::fake_shared(mock_buffer_allocator),
        mt::fake_shared(mock_input_scene)
    };
    cursor.show(test_image);
    ASSERT_THAT(cursor_buffer, NotNull());

    uint32_t const expected_pixel =
        (static_cast<uint32_t>(a) << 24) |
        (static_cast<uint32_t>(r) << 16) |
        (static_cast<uint32_t>(g) << 8)  |
        (static_cast<uint32_t>(b) << 0);

    auto const mapping = cursor_buffer->map_readable();
    ASSERT_THAT(mapping->format(), Eq(mir_pixel_format_argb_8888));

    auto const stride = mapping->stride().as_uint32_t();
    for (auto y = 0u; y < mapping->size().height.as_uint32_t(); ++y)
    {
        auto const line = std::vector<uint32_t>{
            reinterpret_cast<uint32_t const*>(mapping->data() + (stride * y)),
            reinterpret_cast<uint32_t const*>(mapping->data() + (stride * y) + mapping->size().width.as_uint32_t())};
        EXPECT_THAT(line, Each(Eq(expected_pixel)));
    }
}

TEST_F(SoftwareCursor, renderable_has_normal_orientation)
{
    cursor.show(stub_cursor_image);
    EXPECT_THAT(cursor.renderable()->orientation(), testing::Eq(mir_orientation_normal));
}

TEST_F(SoftwareCursor, renderable_has_nonone_mirror_mode)
{
    cursor.show(stub_cursor_image);
    EXPECT_THAT(cursor.renderable()->mirror_mode(), testing::Eq(mir_mirror_mode_none));
}

TEST_F(SoftwareCursor, does_not_need_compositing_when_not_shown)
{
    using namespace testing;
    EXPECT_THAT(cursor.needs_compositing(), Eq(false));
}

TEST_F(SoftwareCursor, does_need_compositing_when_shown)
{
    using namespace testing;
    cursor.show(stub_cursor_image);
    EXPECT_THAT(cursor.needs_compositing(), Eq(true));
}
