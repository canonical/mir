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

#include "mir_test_doubles/stub_native_buffer.h"
#include "mir/graphics/native_buffer.h"

namespace mtd = mir::test::doubles;
namespace mg = mir::graphics;
namespace mga = mir::graphics::android;

std::shared_ptr<mg::NativeBuffer> mtd::create_stub_buffer(
    std::shared_ptr<graphics::android::SyncFence> const& fence)
{
    auto dummy_handle = std::make_shared<native_handle_t>();
    return std::shared_ptr<mg::NativeBuffer>(
        new mg::NativeBuffer(dummy_handle, fence),
        [](mg::NativeBuffer* buffer)
        {
            buffer->mir_dereference();
        });

}

std::shared_ptr<mg::NativeBuffer> mtd::create_stub_buffer()
{
    auto ops = std::make_shared<mga::RealSyncFileOps>();
    auto dummy_sync = std::make_shared<mga::SyncFence>(ops, -1);
    return create_stub_buffer(dummy_sync);
}
