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

#ifndef MIR_GRAPHICS_ANDROID_MIR_NATIVE_BUFFER_H_
#define MIR_GRAPHICS_ANDROID_MIR_NATIVE_BUFFER_H_

#include <system/window.h>

#include <functional>
#include <atomic>
#include <memory>
#include <iostream>

namespace mir
{
namespace graphics
{
namespace android
{
struct MirNativeBuffer : public ANativeWindowBuffer
{
    MirNativeBuffer(std::function<void(MirNativeBuffer*)> free);
    void driver_reference();
    void driver_dereference();
    void mir_dereference();

private:
    ~MirNativeBuffer();

    std::function<void(MirNativeBuffer*)> free_fn;
    std::atomic<bool> mir_reference;
    std::atomic<int> driver_references;

};

struct MirNativeBufferDeleter
{ 
    void operator()(MirNativeBuffer* a)
    {
        a->mir_dereference();
    }
};
}
}
}

#endif /* MIR_GRAPHICS_ANDROID_MIR_NATIVE_BUFFER_H_ */
