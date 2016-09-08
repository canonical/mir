/*
 * Copyright © 2016 Canonical Ltd.
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

#ifndef MIR_GRAPHICS_NESTED_EGL_IMAGE_FACTORY_H_
#define MIR_GRAPHICS_NESTED_EGL_IMAGE_FACTORY_H_

#include "mir_toolkit/client_types_nbs.h"
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <memory>

namespace mir
{
namespace graphics
{
namespace nested
{

class NativeBuffer;
class EglImageFactory
{
public:
    virtual ~EglImageFactory() = default;
    virtual std::unique_ptr<EGLImageKHR> create_egl_image_from(
        NativeBuffer& buffer, EGLDisplay display, EGLint const* attrs) const = 0;
protected:
    EglImageFactory() = default;
    EglImageFactory(EglImageFactory const&) = delete;
    EglImageFactory& operator=(EglImageFactory const&) = delete;
};

}
}
}
#endif /* MIR_GRAPHICS_NESTED_EGL_IMAGE_FACTORY_H_ */
