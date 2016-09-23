/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Cemil Azizoglu <cemil.azizoglu@canonical.com>
 *
 */

#include "mir/graphics/gl_config.h"
#include "gl_context.h"

namespace mg=mir::graphics;
namespace mgx=mg::X;

mgx::XGLContext::XGLContext(::Display* const x_dpy,
                            std::shared_ptr<GLConfig> const& gl_config,
                            EGLContext const shared_ctx)
    : egl{*gl_config}
{
    egl.setup(x_dpy, shared_ctx);
}

void mgx::XGLContext::make_current() const
{
    egl.make_current();
}

void mgx::XGLContext::release_current() const
{
    egl.release_current();
}
