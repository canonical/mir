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
#include "interpreter_cache.h"
#include "internal_client_window.h"
#include "mir/graphics/android/mir_native_window.h"

namespace mga=mir::graphics::android;

mga::InternalClient::InternalClient()
{
}

EGLNativeDisplayType mga::InternalClient::egl_native_display()
{
    return EGL_DEFAULT_DISPLAY;
}

EGLNativeWindowType mga::InternalClient::egl_native_window(std::shared_ptr<InternalSurface> const& surface)
{
    if (!client_windows[surface])
    {
        auto cache = std::make_shared<mga::InterpreterCache>();
        auto interpreter = std::make_shared<mga::InternalClientWindow>(surface, cache);
        client_windows[surface] = std::make_shared<mga::MirNativeWindow>(interpreter);
    }

    return client_windows[surface].get();
}
