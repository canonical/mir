/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <mir/frontend/session_credentials.h>
#include <mir/fd.h>
#include <mir_test_framework/open_wrapper.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <fcntl.h>
#include <format>
#include <fstream>

namespace mf = mir::frontend;
namespace mtf = mir_test_framework;

using namespace testing;


struct SessionCredentials : Test
{
    const pid_t PID = -12345; // Negative so it never accidentally works
};


TEST_F(SessionCredentials, can_resolve_from_valid_snap_apparmor_label)
{
    auto info = mf::SessionCredentials::detect_sandbox_info(PID, "snap.some_snap.some_app");

    ASSERT_THAT(std::holds_alternative<mf::SessionCredentials::SnapInfo>(info), IsTrue());
    auto snap_info = std::get<mf::SessionCredentials::SnapInfo>(info);
    EXPECT_THAT(snap_info.snap_name, Eq("some_snap"));
    EXPECT_THAT(snap_info.app_name, Eq("some_app"));
}

TEST_F(SessionCredentials, will_not_resolve_invalid_apparmor_label)
{
    // Unconfined processes on an AppArmor system
    auto info = mf::SessionCredentials::detect_sandbox_info(PID, "unconfined");
    EXPECT_THAT(std::holds_alternative<std::monostate>(info), IsTrue());

    // Systems not using AppArmor
    info = mf::SessionCredentials::detect_sandbox_info(PID, "");
    EXPECT_THAT(std::holds_alternative<std::monostate>(info), IsTrue());

    // Non-snap AppArmor labels
    info = mf::SessionCredentials::detect_sandbox_info(PID, "/usr/bin/man");
    EXPECT_THAT(std::holds_alternative<std::monostate>(info), IsTrue());

    // Malformed snap AppArmor labels
    info = mf::SessionCredentials::detect_sandbox_info(PID, "snap.");
    EXPECT_THAT(std::holds_alternative<std::monostate>(info), IsTrue());
    info = mf::SessionCredentials::detect_sandbox_info(PID, "snap.some_snap");
    EXPECT_THAT(std::holds_alternative<std::monostate>(info), IsTrue());
}

TEST_F(SessionCredentials, can_resolve_from_valid_flatpak_info)
{
    const char* app_id = "test.application.name";

    auto const flatpak_info = std::format("/proc/{}/root/.flatpak-info", PID);
    char tmp_file_name[] = "/tmp/mir_test_dtp_fmgr_can_XXXXXX";
    {
        auto fd_rw = ::mkstemp(tmp_file_name);
        ASSERT_GE(fd_rw, 0);
        ::close(fd_rw);
    }
    {
        std::ofstream tmp_file;
        tmp_file.open(tmp_file_name);
        tmp_file << "[Application]\nname=" << app_id;
    }

    auto fd = mir::Fd{::open(tmp_file_name, O_RDONLY)};
    auto open_handler = mtf::add_open_handler([flatpak_info, fd](
        const char* path,
        int,
        std::optional<mode_t>) -> std::optional<int>
        {
            if (flatpak_info != path)
            {
                return std::nullopt;
            }

            return static_cast<int>(fd);
        });


    auto info = mf::SessionCredentials::detect_sandbox_info(PID, "");
    ASSERT_THAT(std::holds_alternative<mf::SessionCredentials::FlatpakInfo>(info), IsTrue());
    EXPECT_THAT(std::get<mf::SessionCredentials::FlatpakInfo>(info).app_id, Eq(app_id));
    ::unlink(tmp_file_name);
}

TEST_F(SessionCredentials, app_id_will_not_resolve_from_flatpak_info_when_name_is_missing)
{
    auto const flatpak_info = std::format("/proc/{}/root/.flatpak-info", PID);
    char tmp_file_name[] = "/tmp/mir_test_dtp_fmgr_wont_XXXXXX";
    {
        auto fd_rw = ::mkstemp(tmp_file_name);
        ASSERT_GE(fd_rw, 0);
        ::close(fd_rw);
    }
    {
        std::ofstream tmp_file;
        tmp_file.open(tmp_file_name);
        tmp_file << "[Application]";
    }

    auto fd = mir::Fd{::open(tmp_file_name, O_RDONLY)};
    auto open_handler = mtf::add_open_handler([flatpak_info, fd](
        const char* path,
        int,
        std::optional<mode_t>) -> std::optional<int>
        {
            if (flatpak_info != path)
            {
               return std::nullopt;
            }

            return static_cast<int>(fd);
        });

    auto info = mf::SessionCredentials::detect_sandbox_info(PID, "");
    EXPECT_THAT(std::holds_alternative<std::monostate>(info), IsTrue());
    ::unlink(tmp_file_name);
}

TEST_F(SessionCredentials, snap_info_chosen_over_flatpak_info)
{
    auto const flatpak_info = std::format("/proc/{}/root/.flatpak-info", PID);
    char tmp_file_name[] = "/tmp/mir_test_dtp_fmgr_can_XXXXXX";
    {
        auto fd_rw = ::mkstemp(tmp_file_name);
        ASSERT_GE(fd_rw, 0);
        ::close(fd_rw);
    }
    {
        std::ofstream tmp_file;
        tmp_file.open(tmp_file_name);
        tmp_file << "[Application]\nname=test.application.name";
    }

    auto fd = mir::Fd{::open(tmp_file_name, O_RDONLY)};
    auto open_handler = mtf::add_open_handler([flatpak_info, fd](
        const char* path,
        int,
        std::optional<mode_t>) -> std::optional<int>
        {
            if (flatpak_info != path)
            {
                return std::nullopt;
            }

            return static_cast<int>(fd);
        });


    auto info = mf::SessionCredentials::detect_sandbox_info(PID, "snap.some_snap.some_app");
    ASSERT_THAT(std::holds_alternative<mf::SessionCredentials::SnapInfo>(info), IsTrue());
    ::unlink(tmp_file_name);
}
