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


#ifndef MIR_INPUT_XCURSOR_CURSOR_LOADER_H_
#define MIR_INPUT_XCURSOR_CURSOR_LOADER_H_

#include "mir/input/cursor_images.h"

#include <memory>
#include <string>
#include <map>
#include <mutex>
#include <functional>

// Unfortunately this library does not compile as C++ so we can not namespace it.
extern "C"
{
struct _XcursorImages;
}

namespace mir
{
namespace graphics
{
class CursorImage;
}

namespace input
{
class XCursorLoader : public CursorImages
{
public:
    XCursorLoader();
    virtual ~XCursorLoader() = default;

    std::shared_ptr<graphics::CursorImage> image(std::string const& cursor_name,
        geometry::Size const& size);

protected:
    XCursorLoader(XCursorLoader const&) = delete;
    XCursorLoader& operator=(XCursorLoader const&) = delete;

private:
    std::mutex guard;

    std::map<std::string, std::shared_ptr<graphics::CursorImage>> loaded_images;

    void load_cursor_theme(std::string const& theme_name);
    void load_appropriately_sized_image(_XcursorImages *images);
};
}
}


#endif /* MIR_INPUT_XCURSOR_CURSOR_LOADER_H_ */
