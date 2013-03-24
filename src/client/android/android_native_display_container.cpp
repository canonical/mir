/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "android_native_display_container.h"

#include "mir_toolkit/mir_client_library.h"

namespace mcl = mir::client;
namespace mcla = mcl::android;

mcl::EGLNativeDisplayContainer& mcl::EGLNativeDisplayContainer::instance()
{
    static mcla::AndroidNativeDisplayContainer default_display_container;
    return default_display_container;
}

mcla::AndroidNativeDisplayContainer::AndroidNativeDisplayContainer()
{
}

mcla::AndroidNativeDisplayContainer::~AndroidNativeDisplayContainer()
{
}

bool
mcla::AndroidNativeDisplayContainer::validate(mir_toolkit::MirEGLNativeDisplayType display) const
{
    return mir_connection_is_valid(static_cast<mir_toolkit::MirConnection*>(display));
}

mir_toolkit::MirEGLNativeDisplayType
mcla::AndroidNativeDisplayContainer::create(mir_toolkit::MirConnection* connection)
{
    return static_cast<mir_toolkit::MirEGLNativeDisplayType>(connection);
}

void
mcla::AndroidNativeDisplayContainer::release(mir_toolkit::MirEGLNativeDisplayType /* display */)
{
}    
