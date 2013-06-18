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

struct MirNativeBuffer;
static void incref_hook(struct android_native_base_t* base);
static void decref_hook(struct android_native_base_t* base);

struct MirNativeBuffer : public ANativeWindowBuffer
{
    MirNativeBuffer(std::function<void(MirNativeBuffer*)> free)
        : free_fn(free),
          mir_reference(true),
          external_references(0)
    {
        common.incRef = incref_hook;
        common.decRef = decref_hook;
    }
    void external_reference()
    {
        external_references++;
    }

    void external_dereference()
    {
        external_references--;
        if ((external_references == 0) && (!mir_reference))
        {
            free_fn(this);
            delete this;
        }
    }

    void internal_dereference()
    {
        mir_reference = false;
        if (external_references == 0)
        {
            free_fn(this);
            delete this;
        }
    }

private:
    ~MirNativeBuffer() = default;
    std::function<void(MirNativeBuffer*)> free_fn;
    std::atomic<bool> mir_reference;
    std::atomic<int> external_references;

};

static void incref_hook(struct android_native_base_t* base)
{
    auto buffer = reinterpret_cast<MirNativeBuffer*>(base);
    buffer->external_reference();
}
static void decref_hook(struct android_native_base_t* base)
{
    auto buffer = reinterpret_cast<MirNativeBuffer*>(base);
    buffer->external_dereference();
}

struct ExternalRefDeleter
{ 
    void operator()(MirNativeBuffer* a)
    {
        a->internal_dereference();
    }
};

}
}
}

#endif /* MIR_GRAPHICS_ANDROID_MIR_NATIVE_BUFFER_H_ */
