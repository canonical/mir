/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_NULL_BUFFER_BUNDLE_H_
#define MIR_TEST_DOUBLES_NULL_BUFFER_BUNDLE_H_

#include <mir/surfaces/buffer_bundle.h>
#include <mir_test_doubles/stub_buffer.h>

namespace mir
{
namespace test
{
namespace doubles
{

class NullBufferBundle : public surfaces::BufferBundle
{
public:
    NullBufferBundle()
    {
        stub_buffer = std::make_shared<StubBuffer>();
    }
    std::shared_ptr<compositor::Buffer> secure_client_buffer()
    {
        return stub_buffer;
    }

    std::shared_ptr<surfaces::GraphicRegion> lock_back_buffer()
    {
        return std::shared_ptr<surfaces::GraphicRegion>();
    }

    geometry::PixelFormat get_bundle_pixel_format()
    {
        return geometry::PixelFormat();
    }

    geometry::Size bundle_size()
    {
        return geometry::Size();
    }

    void shutdown()
    {
    }

    std::shared_ptr<compositor::Buffer> stub_buffer;
};

}
}
} // namespace mir

#endif /* MIR_TEST_DOUBLES_NULL_BUFFER_BUNDLE_H_ */
