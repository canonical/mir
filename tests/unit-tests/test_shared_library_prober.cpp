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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "mir/shared_library_prober.h"

#include "mir_test_framework/executable_path.h"

#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <cstring>
#include <unordered_map>

#include <system_error>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mtf = mir_test_framework;

namespace
{
class MockSharedLibraryProberReport : public mir::SharedLibraryProberReport
{
public:
    MOCK_METHOD1(probing_path, void(boost::filesystem::path const&));
    MOCK_METHOD2(probing_failed, void(boost::filesystem::path const&, std::exception const&));
    MOCK_METHOD1(loading_library, void(boost::filesystem::path const&));
    MOCK_METHOD2(loading_failed, void(boost::filesystem::path const&, std::exception const&));
};

class SharedLibraryProber : public testing::Test
{
public:
    SharedLibraryProber()
        : library_path{mtf::executable_path() + "/test_data"}
    {
        // Can't use std::string, as mkdtemp mutates its argument.
        auto tmp_name = std::unique_ptr<char[], std::function<void(char*)>>{strdup("/tmp/mir_empty_directory_XXXXXX"),
                                                                            [](char* data) {free(data);}};
        if (mkdtemp(tmp_name.get()) == NULL)
        {
            throw std::system_error{errno, std::system_category(), "Failed to create temporary directory"};
        }
        temporary_directory = std::string{tmp_name.get()};
    }

    ~SharedLibraryProber()
    {
        // Can't do anything useful in case of failure...
        rmdir(temporary_directory.c_str());
    }

    std::string const library_path;
    std::string temporary_directory;
    testing::NiceMock<MockSharedLibraryProberReport> null_report;
};

}

TEST_F(SharedLibraryProber, returns_non_empty_list_for_path_containing_libraries)
{
    auto libraries = mir::libraries_for_path(library_path, null_report);
    EXPECT_GE(libraries.size(), 1);
}

TEST_F(SharedLibraryProber, raises_exception_for_nonexistent_path)
{
    EXPECT_THROW(mir::libraries_for_path("/a/path/that/certainly/doesnt/exist", null_report),
                 std::system_error);
}

TEST_F(SharedLibraryProber, exception_mentions_missing_path_name)
{
    bool thrown = false;

    try
    {
        mir::libraries_for_path("/panacea", null_report);
    }
    catch (std::system_error& err)
    {
        thrown = true;
        EXPECT_THAT(err.what(), testing::HasSubstr("/panacea"));
    }

    EXPECT_TRUE(thrown);
}

TEST_F(SharedLibraryProber, non_existent_path_raises_ENOENT_error)
{
    try
    {
        mir::libraries_for_path("/a/path/that/certainly/doesnt/exist", null_report);
    }
    catch (std::system_error &err)
    {
        EXPECT_EQ(err.code(), std::error_code(ENOENT, std::system_category()));
    }
}

TEST_F(SharedLibraryProber, path_with_no_shared_libraries_returns_empty_list)
{
    auto libraries = mir::libraries_for_path(temporary_directory.c_str(), null_report);
    EXPECT_EQ(0, libraries.size());
}

TEST_F(SharedLibraryProber, logs_start_of_probe)
{
    using namespace testing;
    testing::NiceMock<MockSharedLibraryProberReport> report;

    EXPECT_CALL(report, probing_path(testing::Eq(library_path)));

    mir::libraries_for_path(library_path, report);
}

TEST_F(SharedLibraryProber, logs_for_nonexistent_path)
{
    using namespace testing;
    NiceMock<MockSharedLibraryProberReport> report;

    EXPECT_CALL(report, probing_path(testing::Eq("/yo/dawg/I/heard/you/liked/slashes")));

    EXPECT_THROW(mir::libraries_for_path("/yo/dawg/I/heard/you/liked/slashes", report),
                 std::runtime_error);
}

TEST_F(SharedLibraryProber, logs_failure_for_nonexistent_path)
{
    using namespace testing;
    NiceMock<MockSharedLibraryProberReport> report;

    EXPECT_CALL(report, probing_failed(Eq("/yo/dawg/I/heard/you/liked/slashes"), _));

    EXPECT_THROW(mir::libraries_for_path("/yo/dawg/I/heard/you/liked/slashes", report),
                 std::runtime_error);

}

TEST_F(SharedLibraryProber, logs_no_libraries_for_path_without_libraries)
{
    using namespace testing;
    NiceMock<MockSharedLibraryProberReport> report;

    EXPECT_CALL(report, loading_library(_)).Times(0);

    mir::libraries_for_path(temporary_directory.c_str(), report);
}

namespace
{
MATCHER_P(FilenameMatches, matcher, "")
{
    using namespace testing;
    *result_listener << "where the path is " << arg;
    return Matches(matcher)(arg.filename().native());
}
}

TEST_F(SharedLibraryProber, logs_each_library_probed)
{
    using namespace testing;
    NiceMock<MockSharedLibraryProberReport> report;

    auto const dso_filename_regex = ".*\\.so(\\..*)?";

    // We have at least 8 DSOs to probe:
    // i386, amd64, armhf, arm64, powerpc, ppc64el, this-arch, libinvalid.so.3
    EXPECT_CALL(report,
        loading_library(FilenameMatches(MatchesRegex(dso_filename_regex)))).Times(AtLeast(8));
    // We shouldn't probe anything that doesn't look like a DSO.
    EXPECT_CALL(report,
        loading_library(FilenameMatches(Not(MatchesRegex(dso_filename_regex))))).Times(0);

    mir::libraries_for_path(library_path, report);
}

TEST_F(SharedLibraryProber, logs_failure_for_load_failure)
{
    using namespace testing;
    NiceMock<MockSharedLibraryProberReport> report;

    std::unordered_map<std::string, bool> probing_map;

    ON_CALL(report, loading_library(_))
        .WillByDefault(Invoke([&probing_map](auto const& filename)
        {
            probing_map[filename.filename().native()] = true;
        }));
    ON_CALL(report, loading_failed(_,_))
        .WillByDefault(Invoke([&probing_map](auto const& filename, auto const&)
        {
            probing_map[filename.filename().native()] = false;
        }));

    mir::libraries_for_path(library_path, report);

    EXPECT_FALSE(probing_map.at("libinvalid.so.3"));

    // As we add extra test DSOs you can add them to this list, but it's not mandatory.
    // At least one of these should fail on any conceivable architecture.
    EXPECT_THAT(probing_map, Contains(AnyOf(
        std::make_pair("lib386.so", false),
        std::make_pair("libamd64.so", false),
        std::make_pair("libarmhf.so", false),
        std::make_pair("libarm64.so", false),
        std::make_pair("libpowerpc.so", false),
        std::make_pair("libppc64el.so", false))));
}

TEST_F(SharedLibraryProber, does_not_log_failure_on_success)
{
    using namespace testing;
    NiceMock<MockSharedLibraryProberReport> report;

    std::unordered_map<std::string, bool> probing_map;

    ON_CALL(report, loading_library(_))
        .WillByDefault(Invoke([&probing_map](auto const& filename)
        {
            probing_map[filename.filename().native()] = true;
        }));
    ON_CALL(report, loading_failed(_,_))
        .WillByDefault(Invoke([&probing_map](auto const& filename, auto const&)
        {
            probing_map[filename.filename().native()] = false;
        }));

    mir::libraries_for_path(library_path, report);

    // libthis-arch should always be loadable...
    EXPECT_TRUE(probing_map.at("libthis-arch.so"));
}
