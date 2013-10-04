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

#include "mir_toolkit/mir_native_buffer.h"
#include <system/window.h>
#include <memory>
#include <mutex>

namespace mir
{
namespace graphics
{
namespace android
{

struct NativeBuffer : public MirNativeBuffer
{
    //internal allocation
    NativeBuffer(std::shared_ptr<const native_handle_t> const&);
    //external allocation
    NativeBuffer(ANativeWindowBuffer const&, int fence_fd);

    void driver_reference();
    void driver_dereference();
    void mir_dereference();

    //potentially rework
    int fence;
protected:
    virtual ~NativeBuffer();

private:

    std::shared_ptr<const native_handle_t> const handle_resource;
    std::mutex mutex;
    bool mir_reference;
    int driver_references;
};

}
}
}

#endif /* MIR_GRAPHICS_ANDROID_MIR_NATIVE_BUFFER_H_ */
