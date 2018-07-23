/*
 * Copyright © 2013 Canonical Ltd.
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
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
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

class NullDisplayBuffer : public graphics::DisplayBuffer, public graphics::NativeDisplayBuffer
{
public:
    geometry::Rectangle view_area() const override { return geometry::Rectangle(); }
    bool overlay(graphics::RenderableList const&) override { return false; }
    glm::mat2 transformation() const override { return glm::mat2(1); }
    NativeDisplayBuffer* native_display_buffer() override { return this; }
};

}
}
}

#endif /* MIR_TEST_DOUBLES_NULL_DISPLAY_BUFFER_H_ */
