/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "internal_client.h"

namespace mg=mir::graphics;
namespace mgm=mir::graphics::mesa;

mgm::InternalClient::InternalClient(std::shared_ptr<MirMesaEGLNativeDisplay> const& native_display)
    : native_display(native_display)
{
}

EGLNativeDisplayType mgm::InternalClient::egl_native_display()
{
    return reinterpret_cast<EGLNativeDisplayType>(native_display.get());
}

EGLNativeWindowType mgm::InternalClient::egl_native_window(std::shared_ptr<InternalSurface> const& surface)
{
    if (!client_window)
    {
        client_window = std::make_shared<mgm::InternalNativeSurface>(surface);
    }

    return client_window.get();
}
