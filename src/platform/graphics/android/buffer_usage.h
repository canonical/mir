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

#ifndef MIR_GRAPHICS_ANDROID_BUFFER_USAGE_H_
#define MIR_GRAPHICS_ANDROID_BUFFER_USAGE_H_

namespace mir
{
namespace graphics
{
namespace android
{

enum class BufferUsage : uint32_t
{
    use_hardware, //buffer supports usage as a gles render target, and a gles texture
    use_software, //buffer supports usage as a cpu render target, and a gles texture
    use_framebuffer_gles //buffer supports usage as a gles render target, hwc layer, and is postable to framebuffer
};

}
}
}

#endif /* MIR_GRAPHICS_ANDROID_BUFFER_USAGE_H_ */
