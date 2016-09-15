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
#include "mir_toolkit/mir_buffer.h"

namespace mgn = mir::graphics::nested;

auto mgn::AndroidImageFactory::create_egl_image_from(
    mgn::NativeBuffer& buffer, EGLDisplay display, EGLint const* attrs) const ->
    std::unique_ptr<EGLImageKHR, std::function<void(EGLImageKHR*)>>
{
    auto mb = buffer.client_handle();

    printf("MAPIMAGE\n");
    (void)attrs;
    EGLenum t;
    EGLClientBuffer b;
    EGLint* attr;
    mir_buffer_get_egl_image(mb, "", &t, &b, &attr);
    auto ext = egl_extensions;
    return EGLImageUPtr{
        new EGLImageKHR(
            egl_extensions->eglCreateImageKHR(display, EGL_NO_CONTEXT, t, b, attr)),
        [ext, display](EGLImageKHR* image) { ext->eglDestroyImageKHR(display, image); delete image; }};
}
