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

#ifndef MIR_GRAPHICS_ANDROID_INTERPRETER_RESOURCE_CACHE_H_
#define MIR_GRAPHICS_ANDROID_INTERPRETER_RESOURCE_CACHE_H_
#include <system/window.h>
#include <memory>

namespace mir
{
namespace graphics
{
class Buffer;
class NativeBuffer;

namespace android
{
class InterpreterResourceCache
{
public:
    InterpreterResourceCache() {}

    virtual void store_buffer(std::shared_ptr<graphics::Buffer>const& buffer,
                              std::shared_ptr<graphics::NativeBuffer> const& key) = 0;
    virtual std::shared_ptr<graphics::Buffer> retrieve_buffer(ANativeWindowBuffer* key) = 0;
    virtual void update_native_fence(ANativeWindowBuffer* key, int fence) = 0;

protected:
    virtual ~InterpreterResourceCache() {}
    InterpreterResourceCache(const InterpreterResourceCache&) = delete;
    InterpreterResourceCache& operator=(const InterpreterResourceCache&) = delete;
};
}
}
}
#endif /* MIR_GRAPHICS_ANDROID_INTERPRETER_RESOURCE_CACHE_H_ */
