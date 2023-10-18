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

#include "mir/renderer/gl/context.h"

#include <EGL/egl.h>

namespace mir::graphics::gbm
{
class SurfacelessEGLContext : public mir::renderer::gl::Context
{
public:
    SurfacelessEGLContext(EGLDisplay dpy);
    SurfacelessEGLContext(EGLDisplay dpy, EGLContext share_with);

    ~SurfacelessEGLContext() override;
    
    void make_current() const override;
    
    void release_current() const override;
    
    auto make_share_context() const -> std::unique_ptr<Context> override;
    explicit operator EGLContext() override;

    auto egl_display() const -> EGLDisplay;
private:    
    EGLDisplay const dpy;
    EGLContext const ctx;
};
}