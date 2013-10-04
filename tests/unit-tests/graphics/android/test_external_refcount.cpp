/*
 * Copyright © 2013 Canonical Ltd.
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

#include "mir/graphics/android/mir_native_buffer.h"
#include <memory>
#include <gtest/gtest.h>

namespace mga=mir::graphics::android;

TEST(AndroidRefcount, driver_hooks)
{
    auto native_handle_resource = std::make_shared<native_handle_t>();
    auto use_count_before = native_handle_resource.use_count();
    mga::NativeBuffer* driver_reference = nullptr;
    {
        auto tmp = new mga::NativeBuffer(native_handle_resource);
        std::shared_ptr<mga::NativeBuffer> buffer(tmp, [](mga::NativeBuffer* buffer)
            {
                buffer->mir_dereference();
            });

        driver_reference = buffer.get();
        driver_reference->common.incRef(&driver_reference->common);
        //Mir loses its reference, driver still has a ref
    }

    EXPECT_EQ(use_count_before+1, native_handle_resource.use_count());
    driver_reference->common.decRef(&driver_reference->common);
    EXPECT_EQ(use_count_before, native_handle_resource.use_count());
}

TEST(AndroidRefcount, driver_hooks_mir_ref)
{
    auto native_handle_resource = std::make_shared<native_handle_t>();
    auto use_count_before = native_handle_resource.use_count();
    {
        std::shared_ptr<mga::NativeBuffer> mir_reference;
        mga::NativeBuffer* driver_reference = nullptr;
        {
            auto tmp = new mga::NativeBuffer(native_handle_resource);
            mir_reference = std::shared_ptr<mga::NativeBuffer>(tmp, [](mga::NativeBuffer* buffer)
                {
                    buffer->mir_dereference();
                });
            driver_reference = mir_reference.get();
            driver_reference->common.incRef(&driver_reference->common);
        }

        //driver loses its reference
        driver_reference->common.decRef(&driver_reference->common);
        EXPECT_EQ(use_count_before+1, native_handle_resource.use_count());
    }
    EXPECT_EQ(use_count_before, native_handle_resource.use_count());
}
