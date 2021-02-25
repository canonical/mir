/*
 * Copyright © 2020 Canonical Ltd.
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
 *
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef MIR_PLATFORM_GBM_KMS_LINUX_DMABUF_H_
#define MIR_PLATFORM_GBM_KMS_LINUX_DMABUF_H_

#include "linux-dmabuf-unstable-v1_wrapper.h"

#include <EGL/egl.h>

#include "mir/graphics/buffer.h"
#include "mir/graphics/egl_extensions.h"


namespace mir
{
class Executor;

namespace renderer
{
namespace gl
{
class Context;
}
}

namespace graphics
{

class DmaBufFormatDescriptors;

class LinuxDmaBufUnstable : public mir::wayland::LinuxDmabufV1::Global
{
public:
    LinuxDmaBufUnstable(
        wl_display* display,
        EGLDisplay dpy,
        std::shared_ptr<EGLExtensions> egl_extensions,
        EGLExtensions::EXTImageDmaBufImportModifiers const& dmabuf_ext);

    std::shared_ptr<Buffer> buffer_from_resource(
        wl_resource* buffer,
        std::shared_ptr<renderer::gl::Context> ctx,
        std::function<void()>&& on_consumed,
        std::function<void()>&& on_release,
        std::shared_ptr<Executor> wayland_executor);

private:
    class Instance;
    void bind(wl_resource* new_resource) override;

    EGLDisplay const dpy;
    std::shared_ptr<EGLExtensions> const egl_extensions;
    std::shared_ptr<DmaBufFormatDescriptors> const formats;
};

}
}

#endif //MIR_PLATFORM_GBM_KMS_LINUX_DMABUF_H_
