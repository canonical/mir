/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mir/graphics/egl_context_executor.h"
#include "mir/fd.h"
#include "mir/geometry/size.h"

#include <memory>
#include <optional>

#include <EGL/egl.h>

namespace mir::graphics
{
class EGLBufferCopier
{
public:
    EGLBufferCopier(std::shared_ptr<common::EGLContextExecutor> egl_delegate);

    ~EGLBufferCopier();
    /**
     * Copy whole image from src to dest
     *
     * Both src and dest must be EGLImages sharing an EGLDisplay with
     * the EGLContext in egl_delegate
     *
     * \returns  An optional native fence object, as per EGL_ANDROID_native_fence_sync
     *           If the optional is empty, the copy has been completed before blit returns.
     *           Otherwise, the fence must be used for synchronisation.
     */
    auto blit(EGLImage src, EGLImage dest, geometry::Size size) -> std::optional<Fd>;
private:
    class Impl;
    std::unique_ptr<Impl> const impl;
};
}
