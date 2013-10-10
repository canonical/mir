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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_STUB_NATIVE_BUFFER_H_
#define MIR_TEST_DOUBLES_STUB_NATIVE_BUFFER_H_

#include "mir/graphics/native_buffer.h"
#include "mir/graphics/android/sync_fence.h"
#include <memory>

namespace mir
{
namespace test
{
namespace doubles
{

std::shared_ptr<graphics::NativeBuffer> create_stub_buffer();
std::shared_ptr<graphics::NativeBuffer> create_stub_buffer(
    std::shared_ptr<graphics::android::Fence> const&);

}
}
}

#endif /* MIR_TEST_DOUBLES_STUB_NATIVE_BUFFER_H_ */
