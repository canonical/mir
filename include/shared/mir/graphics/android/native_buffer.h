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
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_GRAPHICS_ANDROID_NATIVE_BUFFER_H_
#define MIR_GRAPHICS_ANDROID_NATIVE_BUFFER_H_

#include "fence.h"
#include <system/window.h>

namespace mir
{
namespace graphics
{

class NativeBuffer 
{
public:
    virtual ~NativeBuffer() = default;

    virtual ANativeWindowBuffer* anwb() const = 0;
    virtual buffer_handle_t handle() const = 0;
    virtual android::NativeFence copy_fence() const = 0;

    virtual void wait_for_content() = 0;
    virtual void update_fence(android::NativeFence& fence) = 0; 

protected:
    NativeBuffer() = default;
    NativeBuffer(NativeBuffer const&) = delete;
    NativeBuffer& operator=(NativeBuffer const&) = delete;
};

}
}

#endif /* MIR_GRAPHICS_ANDROID_NATIVE_BUFFER_H_ */
