/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
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

#ifndef MIR_TEST_DOUBLES_NULL_PIXEL_BUFFER_H_
#define MIR_TEST_DOUBLES_NULL_PIXEL_BUFFER_H_

#include "src/server/scene/pixel_buffer.h"

namespace mir
{
namespace test
{
namespace doubles
{

struct NullPixelBuffer : public scene::PixelBuffer
{
    void fill_from(graphics::Buffer&) {}
    void const* as_argb_8888() { return nullptr; }
    geometry::Size size() const { return {}; }
    geometry::Stride stride() const { return {}; }
};

}
}
}

#endif /* MIR_TEST_DOUBLES_NULL_PIXEL_BUFFER_H_ */
