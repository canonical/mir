/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_TEST_DOUBLES_NULL_DISPLAY_BUFFER_H_
#define MIR_TEST_DOUBLES_NULL_DISPLAY_BUFFER_H_

#include "mir/graphics/display_buffer.h"

namespace mir
{
namespace test
{
namespace doubles
{

class NullDisplayBuffer : public graphics::DisplayBuffer
{
public:
    geometry::Rectangle view_area() const override { return geometry::Rectangle(); }
    auto pixel_size() const -> geometry::Size override { return geometry::Size(); }
    bool overlay(std::vector<mir::graphics::DisplayElement> const&) override { return false; }
    void set_next_image(std::unique_ptr<mir::graphics::Framebuffer>) override { }
    glm::mat2 transformation() const override { return glm::mat2(1); }
protected:
    auto maybe_create_allocator(graphics::DisplayAllocator::Tag const&) -> graphics::DisplayAllocator*
    {
        return nullptr;
    }
};

}
}
}

#endif /* MIR_TEST_DOUBLES_NULL_DISPLAY_BUFFER_H_ */
