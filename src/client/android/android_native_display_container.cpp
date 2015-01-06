/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "android_native_display_container.h"

#include "mir_toolkit/mir_client_library.h"

#include <mutex>

namespace mcl = mir::client;
namespace mcla = mcl::android;

namespace
{
// default_display_container needs to live until the library is unloaded
std::mutex default_display_container_mutex;
mcla::AndroidNativeDisplayContainer* default_display_container{nullptr};

extern "C" int __attribute__((destructor)) destroy()
{
    std::lock_guard<std::mutex> lock(default_display_container_mutex);

    delete default_display_container;

    return 0;
}
}

mcl::EGLNativeDisplayContainer& mcl::EGLNativeDisplayContainer::instance()
{
    std::lock_guard<std::mutex> lock(default_display_container_mutex);

    if (!default_display_container)
        default_display_container = new mcla::AndroidNativeDisplayContainer;

    return *default_display_container;
}

mcla::AndroidNativeDisplayContainer::AndroidNativeDisplayContainer()
{
}

mcla::AndroidNativeDisplayContainer::~AndroidNativeDisplayContainer()
{
}

bool
mcla::AndroidNativeDisplayContainer::validate(MirEGLNativeDisplayType display) const
{
    std::lock_guard<std::mutex> lg(guard);
    return (valid_displays.find(display) != valid_displays.end());
}

MirEGLNativeDisplayType
mcla::AndroidNativeDisplayContainer::create(ClientContext* context)
{
    std::lock_guard<std::mutex> lg(guard);
    auto egl_display = static_cast<MirEGLNativeDisplayType>(context);
    valid_displays.insert(egl_display);

    return egl_display;
}

void
mcla::AndroidNativeDisplayContainer::release(MirEGLNativeDisplayType display)
{
    std::lock_guard<std::mutex> lg(guard);

    valid_displays.erase(display);
}
