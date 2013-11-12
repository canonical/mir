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
 * Authored by:
 * Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_GRAPHICS_ANDROID_FB_SWAPPER_H_
#define MIR_GRAPHICS_ANDROID_FB_SWAPPER_H_

#include <memory>

namespace mir
{
namespace graphics
{
class Buffer;

namespace android
{

class FBSwapper{
public:
    virtual ~FBSwapper() = default;

    virtual std::shared_ptr<Buffer> compositor_acquire() = 0;
    virtual void compositor_release(std::shared_ptr<Buffer> const& released_buffer) = 0;
protected:
    FBSwapper() = default;
    FBSwapper(FBSwapper const&) = delete;
    FBSwapper& operator=(FBSwapper const&) = delete;
};

}
}
}

#endif /* MIR_GRAPHICS_ANDROID_FB_SWAPPER_H_ */
