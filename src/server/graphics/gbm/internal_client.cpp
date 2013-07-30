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

mgg::InternalClient::InternalClient(std::shared_ptr<MirMesaEGLNativeDisplay> const& native_display)
    : native_display(native_display)
{
}

EGLNativeDisplayType mgg::InternalClient::egl_native_display()
{
    return reinterpret_cast<EGLNativeDisplayType>(native_display.get());
}

EGLNativeWindowType mgg::InternalClient::egl_native_window(std::shared_ptr<InternalSurface> const& surface)
{
    if (!client_window)
    {
        client_window = std::make_shared<mgg::InternalNativeSurface>(surface);
    }

    return client_window.get();
}
