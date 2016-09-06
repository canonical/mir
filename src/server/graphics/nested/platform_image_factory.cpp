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

#include "platform_image_factory.h"
#include "native_buffer.h"

namespace mgn = mir::graphics::nested;

auto mgn::AndroidImageFactory::create_egl_image_from(
    mgn::NativeBuffer& buffer, EGLDisplay display, EGLint const* attrs) const ->
    std::unique_ptr<EGLImageKHR, std::function<void(EGLImageKHR*)>>
{
    auto ext = egl_extensions;
    return EGLImageUPtr{
        new EGLImageKHR(
            egl_extensions->eglCreateImageKHR(display, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID, buffer.get_native_handle(), attrs)),
        [ext, display](EGLImageKHR* image) { ext->eglDestroyImageKHR(display, image); delete image; }};
//        []);
}
