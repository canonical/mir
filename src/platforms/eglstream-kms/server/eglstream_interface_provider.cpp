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

#include "eglstream_interface_provider.h"
#include "mir/graphics/platform.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <utility>

namespace mg = mir::graphics;

namespace
{
class DisplayProvider : public mg::EGLStreamDisplayProvider
{
public:
    DisplayProvider(EGLDisplay dpy, std::optional<EGLStreamKHR> stream);

    auto get_egl_display() const -> EGLDisplay override;

    auto claim_stream() -> EGLStreamKHR override;
private:
    EGLDisplay dpy;
    std::optional<EGLStreamKHR> stream;
};
  
DisplayProvider::DisplayProvider(EGLDisplay dpy, std::optional<EGLStreamKHR> stream)
    : dpy{dpy},
      stream{stream}
{
}

auto DisplayProvider::get_egl_display() const -> EGLDisplay
{
    return dpy;
}

auto DisplayProvider::claim_stream() -> EGLStreamKHR
{
    if (stream)
    {
        return *std::exchange(stream, std::nullopt);
    }
    BOOST_THROW_EXCEPTION((std::logic_error{"No EGLStream to claim (either incorrect object, or stream already claimed)"}));
}
}

mg::eglstream::InterfaceProvider::InterfaceProvider(EGLDisplay dpy)
    : dpy{dpy}
{
}

mg::eglstream::InterfaceProvider::InterfaceProvider(InterfaceProvider const& from, EGLStreamKHR with_stream)
    : dpy{from.dpy},
      stream{with_stream}
{
}

auto mg::eglstream::InterfaceProvider::maybe_create_interface(DisplayInterfaceBase::Tag const& tag)
    -> std::shared_ptr<DisplayInterfaceBase>
{
    if (dynamic_cast<EGLStreamDisplayProvider::Tag const*>(&tag))
    {
        return std::make_shared<DisplayProvider>(dpy, stream);
    }
    return nullptr;
}
