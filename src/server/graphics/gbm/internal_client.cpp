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

namespace mg=mir::graphics;
namespace mgg=mir::graphics::gbm;
namespace mf=mir::frontend;

mgg::InternalClient::InternalClient(std::shared_ptr<MirMesaEGLNativeDisplay> const& native_display,
                                    std::shared_ptr<mf::Surface> const& surface)
    : native_display(native_display),
      surface(surface)
{
}

EGLNativeDisplayType mgg::InternalClient::egl_native_display()
{
    return reinterpret_cast<EGLNativeDisplayType>(native_display.get());
}

EGLNativeWindowType mgg::InternalClient::egl_native_window()
{
    return reinterpret_cast<EGLNativeWindowType>(surface.get());
}
