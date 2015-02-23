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

#include "mir_test_framework/executable_path.h"

using namespace testing;
namespace mtf = mir_test_framework;

TEST(SymbolsRequiredByMesa, are_exported_by_client_platform_mesa)
{
    auto const handle = dlopen(mtf::client_platform("mesa").c_str(), RTLD_LAZY);
    ASSERT_THAT(handle, NotNull());

    auto const sym = dlsym(handle, "mir_client_mesa_egl_native_display_is_valid");
    EXPECT_THAT(sym, NotNull());

    dlclose(handle);
}
