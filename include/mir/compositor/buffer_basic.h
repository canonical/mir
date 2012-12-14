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
#ifndef MIR_COMPOSITOR_BUFFER_BASIC_H_
#define MIR_COMPOSITOR_BUFFER_BASIC_H_

#include "mir/compositor/buffer.h"
#include "mir/compositor/buffer_id.h"

namespace mir
{
namespace compositor
{

class BufferBasic : public Buffer
{
public:
    virtual ~BufferBasic() {}

    /* common functions */
    BufferID id() const
    {
        return buffer_id;
    }

protected:
    BufferBasic(BufferID id)
     : buffer_id(id)
    {}

    /* things we kick down to the concrete classes */
    virtual geometry::Size size() const = 0;
    virtual geometry::Stride stride() const = 0;
    virtual geometry::PixelFormat pixel_format() const = 0;
    virtual void bind_to_texture() = 0;
    virtual std::shared_ptr<BufferIPCPackage> get_ipc_package() const = 0;


private:
    BufferID const buffer_id;

    BufferBasic() = default;
    BufferBasic(BufferBasic const&) = delete;
    BufferBasic& operator=(BufferBasic const&) = delete;

};

}
}

#endif /* MIR_COMPOSITOR_BUFFER_BASIC_H_ */
