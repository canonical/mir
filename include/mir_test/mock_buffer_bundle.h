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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_TEST_MOCK_BUFFER_BUNDLE_H_
#define MIR_TEST_MOCK_BUFFER_BUNDLE_H_

#include "mir/compositor/buffer_bundle.h"

#include <gmock/gmock.h>

namespace mir
{
namespace graphics
{
class Texture;
}
namespace compositor
{

struct MockBufferBundle : public BufferBundle
{
    MOCK_METHOD0(secure_client_buffer, std::shared_ptr<GraphicBufferClientResource>());
    MOCK_METHOD0(lock_and_bind_back_buffer, std::shared_ptr<graphics::Texture>());

    MOCK_METHOD0(get_bundle_pixel_format, geometry::PixelFormat());
    MOCK_METHOD0(bundle_height, geometry::Height());
    MOCK_METHOD0(bundle_width, geometry::Width());
};

}
}
#endif /* MIR_TEST_MOCK_BUFFER_BUNDLE_H_ */
