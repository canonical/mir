/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
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

#include "xcursor_loader.h"

#include "mir/graphics/cursor_image.h"

#include <boost/throw_exception.hpp>
#include <stdexcept>

#include <string.h>

#include <mir_toolkit/cursors.h>

// Unforunately this can not be compiled as C++...so we can not namespace
// these symbols. In order to differentiate from internal symbols
//  we refer to them via their _ prefixed version, i.e. _XcursorImage
extern "C"
{
#include <xcursor.h>
}

namespace mg = mir::graphics;
namespace mi = mir::input;
namespace geom = mir::geometry;

namespace
{
class XCursorImage : public mg::CursorImage
{
public:
    XCursorImage(_XcursorImage *image, std::shared_ptr<_XcursorImages> const& save_resource)
        : image(image),
          save_resource(save_resource)
    {
        if (image->width != mi::default_cursor_size.width.as_uint32_t() ||
            image->height != mi::default_cursor_size.height.as_uint32_t())
        {
            BOOST_THROW_EXCEPTION(
                std::runtime_error("Somehow we got a cursor not of the default size (currently only 24x24 supported)"));
        }
    }

    ~XCursorImage()
    {
    }

    void const* as_argb_8888() const override
    {
        return image->pixels;
    }
    geom::Size size() const override
    {
        return mi::default_cursor_size;
    }
    geom::Displacement hotspot() const override
    {
        return {image->xhot, image->yhot};
    }

private:
    _XcursorImage *image;
    std::shared_ptr<_XcursorImages> const save_resource;
};

std::string const
xcursor_name_for_mir_cursor(std::string const& mir_cursor_name)
{
    if (mir_cursor_name == mir_default_cursor_name)
    {
        return "arrow";
    }
    else if (mir_cursor_name == mir_arrow_cursor_name)
    {
        return "arrow";
    }
    else if (mir_cursor_name == mir_busy_cursor_name)
    {
        return "watch";
    }
    else if (mir_cursor_name == mir_caret_cursor_name)
    {
        return "xterm"; // Yep
    }
    else if (mir_cursor_name == mir_pointing_hand_cursor_name)
    {
        return "hand2";
    }
    else if (mir_cursor_name == mir_open_hand_cursor_name)
    {
        return "hand";
    }
    else if (mir_cursor_name == mir_closed_hand_cursor_name)
    {
        return "grabbing";
    }
    else if (mir_cursor_name == mir_horizontal_resize_cursor_name)
    {
        return "h_double_arrow";
    }
    else if (mir_cursor_name == mir_vertical_resize_cursor_name)
    {
        return "v_double_arrow";
    }
    else if (mir_cursor_name == mir_diagonal_resize_bottom_to_top_cursor_name)
    {
        return "top_right_corner";
    }
    else if (mir_cursor_name == mir_diagonal_resize_top_to_bottom_cursor_name)
    {
        return "bottom_right_corner";
    }
    else if (mir_cursor_name == mir_omnidirectional_resize_cursor_name)
    {
        return "fleur";
    }
    else if (mir_cursor_name == mir_vsplit_resize_cursor_name)
    {
        return "v_double_arrow";
    }
    else if (mir_cursor_name == mir_hsplit_resize_cursor_name)
    {
        return "h_double_arrow";
    }
    else if (mir_cursor_name == mir_crosshair_cursor_name)
    {
        return "crosshair";
    }
    else
    {
        return mir_cursor_name;
    }
}
}

mi::XCursorLoader::XCursorLoader()
{
    load_cursor_theme("default");
}

// Each XcursorImages represents images for the different sizes of a given symbolic cursor.
void mi::XCursorLoader::load_appropriately_sized_image(_XcursorImages *images)
{
    // We would rather take this lock in load_cursor_theme but the Xcursor lib style 
    // makes it difficult to use our standard 'pass the lg around to _locked members' pattern
    std::lock_guard<std::mutex> lg(guard);

    // We have to save all the images as XCursor expects us to free them.
    // This contains the actual image data though, so we need to ensure they stay alive 
    // with the lifetime of the mg::CursorImage instance which refers to them.
    auto saved_xcursor_library_resource = std::shared_ptr<_XcursorImages>(images, [](_XcursorImages *images)
        {
            XcursorImagesDestroy(images);
        });

    _XcursorImage *image_of_correct_size = nullptr;
    for (int i = 0; i < images->nimage; i++)
    {
        _XcursorImage *candidate = images->images[i];
        if (candidate->width == mi::default_cursor_size.width.as_uint32_t() &&
            candidate->height == mi::default_cursor_size.height.as_uint32_t())
        {
            image_of_correct_size = candidate;
            break;
        }
    }
    if (!image_of_correct_size)
        return;
    loaded_images[std::string(images->name)] = std::make_shared<XCursorImage>(image_of_correct_size, saved_xcursor_library_resource);
}

void mi::XCursorLoader::load_cursor_theme(std::string const& theme_name)
{
    // Cursors are named by their square dimension...called the nominal size in XCursor terminology, so we just look up by width.
    // Later we verify the actual size.
    xcursor_load_theme(theme_name.c_str(), mi::default_cursor_size.width.as_uint32_t(), 
        [](XcursorImages* images, void *this_ptr)  -> void
        {
            // Can't use lambda capture as this lambda is thunked to a C function ptr
            auto p = static_cast<mi::XCursorLoader*>(this_ptr);
            p->load_appropriately_sized_image(images);
        }, this);
}

std::shared_ptr<mg::CursorImage> mi::XCursorLoader::image(
    std::string const& cursor_name,
    geom::Size const& size)
{
    auto xcursor_name = xcursor_name_for_mir_cursor(cursor_name);

    if (size != mi::default_cursor_size)
        BOOST_THROW_EXCEPTION(
            std::logic_error("Only the default cursor size is currently supported (mi::default_cursor_size)"));
    
    std::lock_guard<std::mutex> lg(guard);

    auto it = loaded_images.find(xcursor_name);
    if (it != loaded_images.end())
        return it->second;

    // Fall back
    it = loaded_images.find("arrow");
    if (it != loaded_images.end())
        return it->second;
    
    return nullptr;
}
