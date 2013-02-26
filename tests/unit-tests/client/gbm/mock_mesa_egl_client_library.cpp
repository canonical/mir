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

#include "mock_mesa_egl_client_library.h"

#include "mir_toolkit/api.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mtd = mir::test::doubles;

namespace
{
mtd::MockMesaEGLClientLibrary* global_mock = NULL;    
}

mtd::MockMesaEGLClientLibrary::MockMesaEGLClientLibrary()
{
    using namespace ::testing;

    assert(global_mock == NULL && "Only one mock object per process is allowed");
    
    global_mock = this;
    
    ON_CALL(*this, surface_next_buffer(_, _, _)).WillByDefault(Return((MirWaitHandle *)NULL));
}

mtd::MockMesaEGLClientLibrary::~MockMesaEGLClientLibrary()
{
    global_mock = NULL;
}

MirConnection * mtd::MockMesaEGLClientLibrary::a_connection()
{
    // We just need a unique address of the correct type for matching expectations 
    // rather than a valid MirConnection or Surface
    return reinterpret_cast<MirConnection *>(&connection);
}

MirSurface * mtd::MockMesaEGLClientLibrary::a_surface()
{
    return reinterpret_cast<MirSurface *>(&surface);
}

void mir_connection_get_platform(MirConnection *connection, MirPlatformPackage *package)
{
    global_mock->connection_get_platform(connection, package);
}

void mir_surface_get_current_buffer(MirSurface *surface, MirBufferPackage *package)
{
    global_mock->surface_get_current_buffer(surface, package);
}

void mir_surface_get_parameters(MirSurface *surface, MirSurfaceParameters *parameters)
{
    global_mock->surface_get_parameters(surface, parameters);
}

MirWaitHandle *mir_surface_next_buffer(MirSurface *surface, mir_surface_lifecycle_callback callback, void *context)
{
    return global_mock->surface_next_buffer(surface, callback, context);
}

void mir_wait_for(MirWaitHandle *wait_handle)
{
    global_mock->wait_for(wait_handle);
}
