/*
 * Copyright © 2013 Canonical Ltd.
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
#ifndef MIR_TEST_DOUBLES_MOCK_INTERPRETER_RESOURCE_CACHE_H_
#define MIR_TEST_DOUBLES_MOCK_INTERPRETER_RESOURCE_CACHE_H_

#include "src/platforms/android/server/interpreter_resource_cache.h"

namespace mir
{
namespace test
{
namespace doubles
{
struct MockInterpreterResourceCache : public graphics::android::InterpreterResourceCache
{
    MOCK_METHOD2(store_buffer, void(std::shared_ptr<graphics::Buffer>const&,
        std::shared_ptr<graphics::NativeBuffer> const&));
    MOCK_METHOD1(retrieve_buffer, std::shared_ptr<graphics::Buffer>(ANativeWindowBuffer*));
    MOCK_METHOD2(update_native_fence, void(ANativeWindowBuffer*, int));
};
}
}
}
#endif /* MIR_TEST_DOUBLES_MOCK_INTERPRETER_RESOURCE_CACHE_H_ */
