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

TEST_F(SharedLibraryProber, ReturnsNonEmptyListForPathContainingLibraries)
{
    auto libraries = mir::libraries_for_path(library_path, null_report);
    EXPECT_GE(libraries.size(), 1);
}

TEST_F(SharedLibraryProber, RaisesExceptionForNonexistentPath)
{
    EXPECT_THROW(mir::libraries_for_path("/a/path/that/certainly/doesnt/exist", null_report),
                 std::system_error);
}

TEST_F(SharedLibraryProber, NonExistentPathRaisesENOENTError)
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

TEST_F(SharedLibraryProber, PathWithNoSharedLibrariesReturnsEmptyList)
{
    auto libraries = mir::libraries_for_path(temporary_directory.c_str(), null_report);
    EXPECT_EQ(0, libraries.size());
}

TEST_F(SharedLibraryProber, LogsStartOfProbe)
{
    using namespace testing;
    testing::NiceMock<MockSharedLibraryProberReport> report;

    EXPECT_CALL(report, probing_path(testing::Eq(library_path)));

    mir::libraries_for_path(library_path, report);
}

TEST_F(SharedLibraryProber, LogsForNonexistentPath)
{
    using namespace testing;
    NiceMock<MockSharedLibraryProberReport> report;

    EXPECT_CALL(report, probing_path(testing::Eq("/yo/dawg/I/heard/you/liked/slashes")));

    EXPECT_THROW(mir::libraries_for_path("/yo/dawg/I/heard/you/liked/slashes", report),
                 std::runtime_error);
}

TEST_F(SharedLibraryProber, LogsFailureForNonexistentPath)
{
    using namespace testing;
    NiceMock<MockSharedLibraryProberReport> report;

    EXPECT_CALL(report, probing_failed(Eq("/yo/dawg/I/heard/you/liked/slashes"), _));

    EXPECT_THROW(mir::libraries_for_path("/yo/dawg/I/heard/you/liked/slashes", report),
                 std::runtime_error);

}

TEST_F(SharedLibraryProber, LogsNoLibrariesForPathWithoutLibraries)
{
    using namespace testing;
    NiceMock<MockSharedLibraryProberReport> report;

    EXPECT_CALL(report, loading_library(_)).Times(0);

    mir::libraries_for_path(temporary_directory.c_str(), report);
}

namespace
{
MATCHER_P(FilenameMatches, path, "")
{
    *result_listener << "where the path is " << arg;
    return boost::filesystem::path(path).filename() == arg.filename();
}
}

TEST_F(SharedLibraryProber, LogsEachLibraryProbed)
{
    using namespace testing;
    NiceMock<MockSharedLibraryProberReport> report;

    EXPECT_CALL(report, loading_library(FilenameMatches("libamd64.so")));
    EXPECT_CALL(report, loading_library(FilenameMatches("libarmhf.so")));

    mir::libraries_for_path(library_path, report);
}

TEST_F(SharedLibraryProber, LogsFailureForLoadFailure)
{
    using namespace testing;
    NiceMock<MockSharedLibraryProberReport> report;

    bool armhf_failed{false}, amd64_failed{false};

    ON_CALL(report, loading_failed(FilenameMatches("libamd64.so"), _))
            .WillByDefault(InvokeWithoutArgs([&amd64_failed]() { amd64_failed = true; }));
    ON_CALL(report, loading_failed(FilenameMatches("libarmhf.so"), _))
            .WillByDefault(InvokeWithoutArgs([&armhf_failed]() { armhf_failed = true; }));

    mir::libraries_for_path(library_path, report);

    EXPECT_TRUE(amd64_failed || armhf_failed);
}
