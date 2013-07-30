/*
 * Copyright © 2013 Canonical Ltd.
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

#include "internal_client.h"

#include "mir/frontend/surface.h"
#include "mir/graphics/internal_surface.h"

namespace mg=mir::graphics;
namespace mgg=mir::graphics::gbm;
namespace mf=mir::frontend;

namespace
{
class ForwardingInternalSurface : public mg::Surface
{
public:
    ForwardingInternalSurface(std::shared_ptr<mf::Surface> const& surface) : surface(surface) {}

private:
    virtual std::shared_ptr<mg::Buffer> advance_client_buffer() { return surface->advance_client_buffer(); }
    virtual mir::geometry::Size size() const { return surface->size(); }
    virtual MirPixelFormat pixel_format() const { return static_cast<MirPixelFormat>(surface->pixel_format()); }

    std::shared_ptr<mf::Surface> const surface;
};
}

mgg::InternalClient::InternalClient(std::shared_ptr<MirMesaEGLNativeDisplay> const& native_display)
    : native_display(native_display)
{
}

EGLNativeDisplayType mgg::InternalClient::egl_native_display()
{
    return reinterpret_cast<EGLNativeDisplayType>(native_display.get());
}

EGLNativeWindowType mgg::InternalClient::egl_native_window(std::shared_ptr<mf::Surface> const& surface)
{
    if (!client_window)
    {
        auto tmp = std::make_shared<ForwardingInternalSurface>(surface);
        client_window = std::make_shared<mgg::InternalNativeSurface>(tmp);
    }

    return client_window.get();
}
