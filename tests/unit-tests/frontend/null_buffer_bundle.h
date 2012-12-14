/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_TEST_NULL_BUFFER_BUNDLE_H_
#define MIR_TEST_NULL_BUFFER_BUNDLE_H_

#include <mir/compositor/buffer_bundle.h>
#include <mir_test_doubles/mock_buffer.h>
namespace mir
{
namespace test
{

class NullBufferBundle : public compositor::BufferBundle
{
public:
    NullBufferBundle()
    {
        empty_client_resource = std::make_shared<compositor::GraphicBufferClientResource>();
        geometry::Size sz;
        geometry::Stride st;
        geometry::PixelFormat pf;
        mock_buffer = std::make_shared<mir::test::doubles::MockBuffer>(sz, st, pf);
        empty_client_resource->buffer = mock_buffer;
    }
    std::shared_ptr<compositor::GraphicBufferClientResource> secure_client_buffer()
    {
        return empty_client_resource;
    }

    std::shared_ptr<compositor::GraphicBufferCompositorResource> lock_back_buffer()
    {
        return std::shared_ptr<compositor::GraphicBufferCompositorResource>();
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

    std::shared_ptr<compositor::GraphicBufferClientResource> empty_client_resource;
    std::shared_ptr<compositor::Buffer> mock_buffer;
};

}
}

#endif /* MIR_TEST_NULL_BUFFER_BUNDLE_H_ */
