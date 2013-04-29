/*
 * Copyright © 2012,2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 *   Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_GRAPHICS_ANDROID_ANDROID_BUFFER_H_
#define MIR_GRAPHICS_ANDROID_ANDROID_BUFFER_H_

#include "mir/compositor/buffer_basic.h"

#include <system/window.h>
#include <memory>

namespace mir
{
namespace graphics
{
namespace android
{

class AndroidBuffer : public compositor::BufferBasic
{
public:
    virtual ~AndroidBuffer() = default;
    virtual std::shared_ptr<ANativeWindowBuffer> native_buffer_handle() const = 0;
protected:
    AndroidBuffer() = default;
    AndroidBuffer(AndroidBuffer const&) = delete;
    AndroidBuffer& operator=(AndroidBuffer const&) = delete;

};

}
}
}

#endif /* MIR_GRAPHICS_ANDROID_ANDROID_BUFFER_H_ */
