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

#include "fence.h"
#include <system/window.h>
#include <memory>
#include <mutex>

namespace mir
{
namespace graphics
{
namespace android
{
class Fence;

class NativeBuffer : public ANativeWindowBuffer 
{
public:
    virtual ~NativeBuffer() = default;

    //todo: break inheritance by doing this
    virtual operator ANativeWindowBuffer*() = 0;
    virtual operator NativeFence() const = 0;

    virtual void wait_for_content() = 0;
    virtual void update_fence(NativeFence& fence) = 0; 

protected:
    NativeBuffer() = default;
    NativeBuffer(NativeBuffer const&) = delete;
    NativeBuffer& operator=(NativeBuffer const&) = delete;
};


struct AndroidNativeBuffer : public NativeBuffer
{
    AndroidNativeBuffer(std::shared_ptr<const native_handle_t> const& handle,
        std::shared_ptr<Fence> const& fence);

    operator ANativeWindowBuffer*();
    operator NativeFence() const;

    void wait_for_content();
    void update_fence(NativeFence& merge_fd);

    void driver_reference();
    void driver_dereference();
    void mir_dereference();
private:
    ~AndroidNativeBuffer();

    std::shared_ptr<Fence> fence;
//    std::shared_ptr<ANativeWindowBuffer> 
    std::shared_ptr<const native_handle_t> const handle_resource;

    std::mutex mutex;
    bool mir_reference;
    int driver_references;
};

}
}
}

#endif /* MIR_GRAPHICS_ANDROID_MIR_NATIVE_BUFFER_H_ */
