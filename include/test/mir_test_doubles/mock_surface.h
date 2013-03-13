/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_MOCK_SURFACE_H_
#define MIR_TEST_DOUBLES_MOCK_SURFACE_H_

#include "mir/shell/surface.h"

#include <memory>

namespace mir
{
namespace test
{
namespace doubles
{

struct MockSurface : public shell::Surface
{
    MOCK_METHOD0(hide, void());
    MOCK_METHOD0(show, void());
    MOCK_METHOD0(destroy, void());
    MOCK_METHOD0(shutdown, void());
    MOCK_METHOD0(advance_client_buffer, void());

    MOCK_CONST_METHOD0(size, geometry::Size ());
    MOCK_CONST_METHOD0(pixel_format, geometry::PixelFormat ());
    MOCK_CONST_METHOD0(client_buffer, std::shared_ptr<compositor::Buffer> ());
    
    MOCK_CONST_METHOD0(supports_input, bool());
    MOCK_CONST_METHOD0(client_input_fd, int());
};

}
}
} // namespace mir

#endif // MIR_TEST_DOUBLES_MOCK_SURFACE_H_
