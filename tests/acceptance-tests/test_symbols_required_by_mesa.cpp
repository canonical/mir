/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <dlfcn.h>

using namespace testing;

TEST(SymbolsRequiredByMesa, are_exported_by_libmirclientplatform)
{
    auto const handle = dlopen(MIR_CLIENT_DRIVER_BINARY, RTLD_LAZY);
    ASSERT_THAT(handle, NotNull());

    auto const sym = dlsym(handle, "mir_client_mesa_egl_native_display_is_valid");
    EXPECT_THAT(sym, NotNull());

    dlclose(handle);
}

TEST(SymbolsRequiredByMesa, are_exported_by_libmirplatformgraphics)
{
    auto const handle = dlopen(MIR_PLATFORM_DRIVER_BINARY, RTLD_LAZY);
    ASSERT_THAT(handle, NotNull());

    auto const sym = dlsym(handle, "mir_server_mesa_egl_native_display_is_valid");
    EXPECT_THAT(sym, NotNull());

    dlclose(handle);
}
