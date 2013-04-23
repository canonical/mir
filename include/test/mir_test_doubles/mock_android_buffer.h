/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_MOCK_ANDROID_BUFFER_H_
#define MIR_TEST_DOUBLES_MOCK_ANDROID_BUFFER_H_

#include "src/server/graphics/android/android_buffer.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mir
{
namespace test
{
namespace doubles
{

struct MockAndroidBuffer : public mir::graphics::android::AndroidBuffer
{
 public:
    MockAndroidBuffer()
    {
        using namespace testing;
        ON_CALL(*this, native_buffer_handle())
            .WillByDefault(Return(std::make_shared<ANativeWindowBuffer>()));
    }

    MOCK_CONST_METHOD0(size, geometry::Size());
    MOCK_CONST_METHOD0(stride, geometry::Stride());
    MOCK_CONST_METHOD0(pixel_format, geometry::PixelFormat());
    MOCK_CONST_METHOD0(get_ipc_package, std::shared_ptr<compositor::BufferIPCPackage>());

    MOCK_METHOD0(bind_to_texture, void());
    MOCK_CONST_METHOD0(id, compositor::BufferID());
    MOCK_CONST_METHOD0(native_buffer_handle, std::shared_ptr<ANativeWindowBuffer>());
};

}
}
}

#endif // MIR_TEST_DOUBLES_MOCK_ANDROID_BUFFER_H_
