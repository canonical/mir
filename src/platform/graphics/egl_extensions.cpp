/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by:
 *   Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/graphics/egl_extensions.h"
#include <boost/throw_exception.hpp>
#include <stdexcept>
#include <cstring>

#define MIR_LOG_COMPONENT "EGL extensions"
#include "mir/log.h"

namespace mg=mir::graphics;

mg::egl::WaylandExtensions::WaylandExtensions() :
    eglBindWaylandDisplayWL{
        reinterpret_cast<PFNEGLBINDWAYLANDDISPLAYWL>(eglGetProcAddress("eglBindWaylandDisplayWL"))
    },
    eglQueryWaylandBufferWL{
        reinterpret_cast<PFNEGLQUERYWAYLANDBUFFERWL>(eglGetProcAddress("eglQueryWaylandBufferWL"))
    }
{
    if (!eglBindWaylandDisplayWL || !eglQueryWaylandBufferWL)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("EGL implementation doesn't support EGL_WL_bind_wayland_display"));
    }
}

mg::egl::NVStreamAttribExtensions::NVStreamAttribExtensions() :
    eglCreateStreamAttribNV{
        reinterpret_cast<PFNEGLCREATESTREAMATTRIBNVPROC>(eglGetProcAddress("eglCreateStreamAttribNV"))
    },
    eglStreamConsumerAcquireAttribNV{
        reinterpret_cast<PFNEGLSTREAMCONSUMERACQUIREATTRIBNVPROC>(
            eglGetProcAddress("eglStreamConsumerAcquireAttribNV"))
    }
{
    if (!eglCreateStreamAttribNV || !eglStreamConsumerAcquireAttribNV)
    {
        BOOST_THROW_EXCEPTION((std::runtime_error{"EGL implementation doesn't support EGL_NV_stream_attrib"}));
    }
}

