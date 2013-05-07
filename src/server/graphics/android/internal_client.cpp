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

namespace mf=mir::frontend;
namespace mga=mir::graphics::android;

mga::InternalClient::InternalClient(std::shared_ptr<frontend::Surface> const&)
{

}
EGLNativeDisplayType mga::InternalClient::egl_native_display()
{
    return (EGLNativeDisplayType) 0;
}

EGLNativeWindowType mga::InternalClient::egl_native_window()
{
    return (EGLNativeWindowType) 0;
}
