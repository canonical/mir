/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
 * published by the Free Software Foundation.
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

#ifndef MIR_GRAPHICS_ANDROID_INTERPRETER_CACHE_H_
#define MIR_GRAPHICS_ANDROID_INTERPRETER_CACHE_H_

#include "interpreter_resource_cache.h"
#include <unordered_map>

namespace mir
{
namespace graphics
{
namespace android
{
class InterpreterCache : public InterpreterResourceCache
{
public:
    InterpreterCache() {}

    void store_buffer(std::shared_ptr<graphics::Buffer>const& buffer,
        std::shared_ptr<graphics::NativeBuffer> const& key);
    std::shared_ptr<graphics::Buffer> retrieve_buffer(ANativeWindowBuffer* key);
    void update_native_fence(ANativeWindowBuffer* key, int fence);

private:
    std::unordered_map<ANativeWindowBuffer*, std::shared_ptr<graphics::Buffer>> buffers_in_driver;
    std::unordered_map<ANativeWindowBuffer*, std::shared_ptr<graphics::NativeBuffer>> native_buffers;
};
}
}
}
#endif /* MIR_GRAPHICS_ANDROID_INTERPRETER_CACHE_H_ */
