/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "mir/library_shared_ptr.h"
#include "mir/shared_library.h"
#include "mir/graphics/platform.h"
#include "mir_test_framework/executable_path.h"
#include "src/server/report/null_report_factory.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mtf = mir_test_framework;
namespace mg = mir::graphics;

TEST(LibrarySharedPtr, keeps_library_loaded)
{
    using namespace ::testing;

    mir::LibrarySharedPtr<mg::Platform> platform;
    {
        auto lib = std::make_shared<mir::SharedLibrary>(mtf::server_platform("graphics-dummy.so"));
        platform = mir::LibrarySharedPtr<mg::Platform>(
            lib,
            lib->load_function<mir::graphics::CreateHostPlatform>("create_host_platform")
            (
                std::shared_ptr<mir::options::Option>(),
                std::shared_ptr<mir::EmergencyCleanupRegistry>(),
                mir::report::null_display_report()
            )
            );
    }

    EXPECT_THAT(!!platform, Eq(true));
    platform.reset();
    EXPECT_THAT(!platform, Eq(true));
}
