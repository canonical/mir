/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_INPUT_DYNAMIC_DEFAULT_CURSOR_IMAGE_H_
#define MIR_INPUT_DYNAMIC_DEFAULT_CURSOR_IMAGE_H_

#include <mir/graphics/cursor_image.h>
#include <mir/synchronised.h>

#include <memory>

namespace mir
{
namespace input
{
class CursorImages;

/// A CursorImage proxy for the default cursor that re-queries the current
/// CursorImages when refreshed (e.g. on a cursor theme change). Between
/// refreshes, the resolved image is cached for performance.
class DynamicDefaultCursorImage : public graphics::CursorImage
{
public:
    explicit DynamicDefaultCursorImage(std::shared_ptr<CursorImages> const& cursor_images);

    void const* as_argb_8888() const override;
    geometry::Size size() const override;
    geometry::Displacement hotspot() const override;

    /// Re-query the default cursor image from the current CursorImages.
    void refresh();

private:
    std::shared_ptr<CursorImages> const cursor_images;
    Synchronised<std::shared_ptr<graphics::CursorImage>> cached;
};
}
}

#endif // MIR_INPUT_DYNAMIC_DEFAULT_CURSOR_IMAGE_H_
