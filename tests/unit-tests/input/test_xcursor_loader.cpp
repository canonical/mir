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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "src/server/input/xcursor_loader.h"

#include "mir/graphics/cursor_image.h"
#include "mir_test_framework/executable_path.h"

#include <mir_toolkit/common.h>
#include <mir_toolkit/cursors.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <stdexcept>

#include <stdlib.h>
#include <string.h>

namespace mi = mir::input;
namespace mg = mir::graphics;

namespace
{
bool raw_argb_is_only_pixel(uint32_t const* raw_argb, size_t size, uint32_t pixel)
{
    for (unsigned int i = 0; i < size; i++)
    {
        if (raw_argb[i] != pixel)
        {
            printf("Pixel: %u\n", raw_argb[i]);
            return false;
        }
    }
    return true;
}
bool cursor_image_is_solid_color(std::shared_ptr<mg::CursorImage> const& image, uint32_t pixel)
{
    auto raw_argb = static_cast<uint32_t const*>(image->as_argb_8888());
    size_t size = image->size().width.as_uint32_t() * image->size().height.as_uint32_t();

    return raw_argb_is_only_pixel(raw_argb, size, pixel);
}

MATCHER(HasLoaded, "cursor image has loaded and is not nullptr."\
    " Test expects cursor images to be installed to the directory the test is ran from")
{
    return arg != nullptr;
}

MATCHER(IsSolidRed, "")
{
    return cursor_image_is_solid_color(arg, 0xffff0000);
}

MATCHER(IsSolidGreen, "")
{
    return cursor_image_is_solid_color(arg, 0xff00ff00);
}

MATCHER(IsSolidBlue, "")
{
    return cursor_image_is_solid_color(arg, 0xff0000ff);
}

MATCHER(IsSolidBlack, "")
{
    return cursor_image_is_solid_color(arg, 0xff000000);
}

char *old_xcursor_path = nullptr;
void set_xcursor_path()
{
    char const* old = getenv("XCURSOR_PATH");
    if (old)
        old_xcursor_path = strdup(old);
    auto test_cursor_path =mir_test_framework::executable_path() + std::string("/testing-cursor-theme");
    setenv("XCURSOR_PATH", test_cursor_path.c_str(), 1);
}

void restore_xcursor_path()
{
    if (old_xcursor_path)
    {
        setenv("XCURSOR_PATH", old_xcursor_path, 1);
        free(old_xcursor_path);
    }
}

}

TEST(XCursorLoader, loads_cursors_from_testing_theme)
{
    set_xcursor_path();

    mi::XCursorLoader loader;
    
    auto size = mi::default_cursor_size;
    auto red_image = loader.image("red", size);
    auto blue_image = loader.image("blue", size);
    auto green_image = loader.image("green", size);

    ASSERT_THAT(red_image, HasLoaded());
    ASSERT_THAT(green_image, HasLoaded());
    ASSERT_THAT(blue_image, HasLoaded());
    EXPECT_THAT(red_image, IsSolidRed());
    EXPECT_THAT(green_image, IsSolidGreen());
    EXPECT_THAT(blue_image, IsSolidBlue());
    
    restore_xcursor_path();
}

TEST(XCursorLoader, only_supports_the_default_size)
{
    mi::XCursorLoader loader;
    
    EXPECT_THROW({
            loader.image("red", {100, 100});
    }, std::logic_error);
}

TEST(XCursorLoader, default_image_is_arrow_from_xcursor_theme)
{
    set_xcursor_path();

    mi::XCursorLoader loader;
    
    auto size = mi::default_cursor_size;
    auto arrow_image = loader.image(mir_default_cursor_name, size);

    // The testing theme uses a solid black image for the "arrow" symbolic
    // name.
    ASSERT_THAT(arrow_image, HasLoaded());
    EXPECT_THAT(arrow_image, IsSolidBlack());
    
    restore_xcursor_path();
}

TEST(XCursorLoader, symbolic_names_which_are_not_present_resolve_to_default)
{
    set_xcursor_path();

    mi::XCursorLoader loader;
    
    auto size = mi::default_cursor_size;
    auto default_image = loader.image(mir_default_cursor_name, size);
    auto image_with_made_up_name = loader.image("Artickrumbulis", size);

    EXPECT_EQ(default_image, image_with_made_up_name);
    
    restore_xcursor_path();
}
