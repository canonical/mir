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

#ifndef MIR_TEST_DOUBLES_NULL_CLIENT_BUFFER_H_
#define MIR_TEST_DOUBLES_NULL_CLIENT_BUFFER_H_

#include "mir/client_buffer.h"

namespace mir
{
namespace test
{
namespace doubles
{

class NullClientBuffer : public client::ClientBuffer
{
public:
    std::shared_ptr<client::MemoryRegion> secure_for_cpu_write()
    {
        return std::make_shared<client::MemoryRegion>();
    }
    geometry::Size size() const { return geometry::Size(); }
    geometry::Stride stride() const { return geometry::Stride(); }
    MirPixelFormat pixel_format() const { return mir_pixel_format_invalid; }
    uint32_t age() const { return 0; }
    void increment_age() {}
    void mark_as_submitted() {}
    void update_from(MirBufferPackage const&) {}
    void fill_update_msg(MirBufferPackage&) {}
    std::shared_ptr<graphics::NativeBuffer> native_buffer_handle() const
    {
        return nullptr;
    }
};

}
}
}

#endif /* MIR_TEST_DOUBLES_NULL_CLIENT_BUFFER_H_ */
