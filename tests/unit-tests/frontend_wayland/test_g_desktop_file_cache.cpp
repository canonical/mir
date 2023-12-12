/*
 * Copyright © Canonical Ltd.
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
 */

#define MIR_LOG_COMPONENT "test_g_desktop_file_cache"

#include "mir/log.h"
#include "src/server/frontend_wayland/foreign_toplevel_manager_v1.h"
#include "mir/time/steady_clock.h"
#include "src/include/server/mir/glib_main_loop.h"
#include "mir/test/doubles/mock_main_loop.h"
#include <fstream>

#include <gmock/gmock.h>
#include <cstdlib>
#include <filesystem>

namespace mf = mir::frontend;

using namespace testing;
using ::testing::IsNull;
using ::testing::NotNull;

namespace
{
static std::string applications_dir;
}

struct GDesktopFileCache : Test
{
    std::shared_ptr<mir::MainLoop> main_loop;
    std::thread main_loop_thread;

    static void SetUpTestSuite()
    {
        // Establish the temporary directory
        std::filesystem::path tmp_dir_path {std::filesystem::temp_directory_path() /= std::tmpnam(nullptr)};
        std::filesystem::create_directories(tmp_dir_path);
        std::string desktop_file_directory_name = tmp_dir_path;
        setenv("XDG_DATA_DIRS", desktop_file_directory_name.c_str(), 1);
        applications_dir = desktop_file_directory_name + "/applications";
        mkdir(applications_dir.c_str(), S_IRWXU);
    }

    void SetUp() override
    {
        main_loop = std::make_shared<mir::GLibMainLoop>(std::make_shared<mir::time::SteadyClock>());

        main_loop_thread = std::thread([&]() {
            main_loop->run();
        });
    }

    void TearDown() override
    {
        main_loop->stop();
        main_loop_thread.join();
        main_loop= nullptr;

        // Remove all files from the temporary directory
        for (const auto& entry : std::filesystem::directory_iterator(applications_dir))
            std::filesystem::remove_all(entry.path());
    }

    void write_desktop_file(const char* contents, const char* filename)
    {
        auto filepath = applications_dir + "/" + filename;
        std::fstream file(filepath, std::fstream::out);
        file << contents;
    }
};

TEST_F(GDesktopFileCache, when_desktop_file_is_in_directory_then_it_can_be_found_after_construction)
{
    const char* sh_desktop_contents = "[Desktop Entry]\n"
        "Name=Sh\n"
        "Exec=sh\n"
        "Type=Application\n";
    write_desktop_file(sh_desktop_contents, "mir.test.info.desktop");
    mf::GDesktopFileCache cache(main_loop);

    auto file = cache.lookup_by_app_id("mir.test.info.desktop");
    ASSERT_THAT(file, NotNull());
}

TEST_F(GDesktopFileCache, when_desktop_file_has_wm_class_then_it_can_be_found_via_wm_class_lookup)
{
    const char* sh_desktop_contents = "[Desktop Entry]\n"
        "Name=Sh\n"
        "Exec=sh\n"
        "Type=Application\n"
        "StartupWMClass=test-wm-class\n";
    write_desktop_file(sh_desktop_contents, "mir.test.info.desktop");
    mf::GDesktopFileCache cache(main_loop);

    auto file = cache.lookup_by_wm_class("test-wm-class");
    ASSERT_THAT(file, NotNull());
}

TEST_F(GDesktopFileCache, when_desktop_file_lacks_exec_then_it_cannot_be_found)
{
    const char* sh_desktop_contents = "[Desktop Entry]\n"
        "Name=Sh\n"
        "Type=Application\n";
    write_desktop_file(sh_desktop_contents, "mir.test.info.desktop");
    mf::GDesktopFileCache cache(main_loop);

    auto file = cache.lookup_by_app_id("mir.test.info.desktop");
    ASSERT_THAT(file, IsNull());
}

TEST_F(GDesktopFileCache, when_desktop_file_is_set_to_nodisplay_then_it_cannot_be_found)
{
    const char* sh_desktop_contents = "[Desktop Entry]\n"
        "Name=Sh\n"
        "Exec=sh\n"
        "Type=Application\n"
        "NoDisplay=true\n";
    write_desktop_file(sh_desktop_contents, "mir.test.info.desktop");
    mf::GDesktopFileCache cache(main_loop);

    auto file = cache.lookup_by_app_id("mir.test.info.desktop");
    ASSERT_THAT(file, IsNull());
}

TEST_F(GDesktopFileCache, when_a_desktop_has_invald_exec_then_it_cannot_be_found)
{
    // Note: by invalid we mean that the call does not match any executable on the filesystem
    const char* sh_desktop_contents = "[Desktop Entry]\n"
        "Name=Sh\n"
        "Exec=an-extremely-improbable-executable-file-that-will-not-be-found-on-someones-computer-by-mistake\n"
        "Type=Application\n";
    write_desktop_file(sh_desktop_contents, "mir.test.info.desktop");
    mf::GDesktopFileCache cache(main_loop);

    auto file = cache.lookup_by_app_id("mir.test.info.desktop");
    ASSERT_THAT(file, IsNull());
}
