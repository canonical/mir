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

#include <unistd.h>
#include <libgen.h>

#include <system_error>
#include <boost/throw_exception.hpp>
#include <boost/exception/errinfo_errno.hpp>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace
{
std::string binary_path()
{
    char buf[1024];
    auto tmp = readlink("/proc/self/exe", buf, sizeof buf);
    if (tmp < 0)
        BOOST_THROW_EXCEPTION(boost::enable_error_info(
                                  std::runtime_error("Failed to find our executable path"))
                              << boost::errinfo_errno(errno));
    if (tmp > static_cast<ssize_t>(sizeof(buf) - 1))
        BOOST_THROW_EXCEPTION(std::runtime_error("Path to executable is too long!"));
    buf[tmp] = '\0';
    return dirname(buf);
}

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
        : library_path{binary_path() + "/test_data"}
    {
    }

    std::string const library_path;
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
    // /usr is guaranteed to exist, and shouldn't contain any libraries
    auto libraries = mir::libraries_for_path("/usr", null_report);
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

    mir::libraries_for_path("/usr", report);
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
