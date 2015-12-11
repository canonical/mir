/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan.griffiths@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_STUB_ANDROID_NATIVE_BUFFER_H_
#define MIR_TEST_DOUBLES_STUB_ANDROID_NATIVE_BUFFER_H_

#include "mir/graphics/android/native_buffer.h"
#include "mir/geometry/size.h"

namespace mir
{
namespace test
{
namespace doubles
{
struct StubAndroidNativeBuffer : public graphics::NativeBuffer
{
    StubAndroidNativeBuffer()
    {
    }

    StubAndroidNativeBuffer(geometry::Size sz)
    {
        stub_anwb.width = sz.width.as_int();
        stub_anwb.height = sz.height.as_int();
    }

    auto anwb() const -> ANativeWindowBuffer* override { return const_cast<ANativeWindowBuffer*>(&stub_anwb); }
    auto handle() const -> buffer_handle_t override { return &native_handle; }
    auto copy_fence() const -> graphics::android::NativeFence override { return -1; }

    void ensure_available_for(graphics::android::BufferAccess) {}
    void update_usage(graphics::android::NativeFence&, graphics::android::BufferAccess) {}

    ANativeWindowBuffer stub_anwb;
    native_handle_t native_handle;
};
}
}
}

#endif /* MIR_TEST_DOUBLES_STUB_ANDROID_NATIVE_BUFFER_H_ */
