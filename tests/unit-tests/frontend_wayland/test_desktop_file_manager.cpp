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

#include "src/server/frontend_wayland/desktop_file_manager.h"
#include "mir/test/doubles/mock_surface.h"
#include "mir/test/doubles/mock_scene_session.h"
#include "mir/scene/surface_observer.h"
#include "mir/test/doubles/explicit_executor.h"
#include "mir_test_framework/open_wrapper.h"

#include <filesystem>
#include <fcntl.h>
#include <gmock/gmock.h>
#include <sstream>
#include <fstream>

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace geom = mir::geometry;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;
namespace mtf = mir_test_framework;

using namespace testing;

namespace
{

class InMemoryDesktopFileCache : public mf::DesktopFileCache
{
public:
    std::shared_ptr<mf::DesktopFile> lookup_by_app_id(std::string const& name) const override
    {
        for (auto file : files)
            if (file->id == name)
                return file;
        return nullptr;
    }

    std::shared_ptr<mf::DesktopFile> lookup_by_wm_class(std::string const& name) const override
    {
        for (auto file : files)
            if (file->wm_class == name)
                return file;
        return nullptr;
    }

    std::shared_ptr<mf::DesktopFile> lookup_by_exec_string(std::string const& exec) const override
    {
        for (auto file : files)
            if (file->exec == exec)
                return file;
        return nullptr;
    }

    std::vector<std::shared_ptr<mf::DesktopFile>> files;
};

}

struct DesktopFileManager : Test
{
    std::shared_ptr<InMemoryDesktopFileCache> cache;
    std::shared_ptr<mf::DesktopFileManager> file_manager;
    mtd::MockSurface surface;
    std::shared_ptr<mtd::MockSceneSession> session;

    const char* APPLICATION_ID = "test_app_id";
    const char* DESKTOP_FILE_APP_ID = "test_app_id.desktop";
    const int PID = -12345; // Negative so it never accidentally works

    void SetUp() override
    {
        session = std::make_shared<mtd::MockSceneSession>();
        ON_CALL(*session, process_id)
            .WillByDefault(testing::Invoke([this]() {
                return PID;
            }));

        ON_CALL(surface, application_id())
            .WillByDefault(testing::Invoke([this]() { return APPLICATION_ID; }));

        ON_CALL(surface, session())
            .WillByDefault(testing::Invoke([this]() {
                return session;
            }));

        cache = std::make_shared<InMemoryDesktopFileCache>();
        file_manager = std::make_shared<mf::DesktopFileManager>(cache);
    }
};

TEST_F(DesktopFileManager, can_find_app_when_app_id_matches)
{
    auto new_file = std::make_shared<mf::DesktopFile>(DESKTOP_FILE_APP_ID, nullptr, nullptr);
    cache->files.push_back(new_file);
    auto app_id = file_manager->resolve_app_id(&surface);
    EXPECT_THAT(app_id, Eq(new_file->id));
}

TEST_F(DesktopFileManager, can_find_app_when_wm_class_matches)
{
    auto new_file = std::make_shared<mf::DesktopFile>(nullptr, APPLICATION_ID, nullptr);
    cache->files.push_back(new_file);
    auto app_id = file_manager->resolve_app_id(&surface);
    EXPECT_THAT(app_id, Eq(new_file->id));
}

TEST_F(DesktopFileManager, can_find_app_when_app_id_matches_despite_being_uppercase)
{
    auto uppercase_app_id = std::string(APPLICATION_ID);
    for(char &ch : uppercase_app_id)
        ch = std::toupper(ch);

    ON_CALL(surface, application_id())
        .WillByDefault(testing::Invoke([uppercase_app_id]() { return uppercase_app_id; }));

    auto new_file = std::make_shared<mf::DesktopFile>(DESKTOP_FILE_APP_ID, nullptr, nullptr);
    cache->files.push_back(new_file);
    auto app_id = file_manager->resolve_app_id(&surface);
    EXPECT_THAT(app_id, Eq(new_file->id));
}

TEST_F(DesktopFileManager, when_every_check_fails_then_application_id_is_returned)
{
    auto app_id = file_manager->resolve_app_id(&surface);
    EXPECT_THAT(app_id, Eq(APPLICATION_ID));
}

struct DesktopFileManagerParameterizedTestFixture : public DesktopFileManager, WithParamInterface<std::string> {
};

TEST_P(DesktopFileManagerParameterizedTestFixture, can_find_app_when_app_id_matches_with_prefix)
{
    std::string prefix = GetParam();
    const std::string prefixed = prefix + DESKTOP_FILE_APP_ID;
    auto new_file = std::make_shared<mf::DesktopFile>(prefixed.c_str(), nullptr, nullptr);
    cache->files.push_back(new_file);
    auto app_id = file_manager->resolve_app_id(&surface);
    EXPECT_THAT(app_id, Eq(new_file->id));
}

INSTANTIATE_TEST_SUITE_P(DesktopFileManager, DesktopFileManagerParameterizedTestFixture, ::testing::Values(
    "gnome-",
    "fedora-",
    "mozilla-",
    "debian-"
));

TEST_F(DesktopFileManager, when_security_profile_does_not_start_with_prefix_then_empty_string_is_returned)
{
    auto result = mf::DesktopFileManager::parse_snap_security_profile_to_desktop_id("firefox.firefox (current");
    EXPECT_TRUE(result.empty());
}

TEST_F(DesktopFileManager, when_security_profile_is_valid_then_desktop_id_is_returned)
{
    auto result = mf::DesktopFileManager::parse_snap_security_profile_to_desktop_id("snap.firefox.firefox (enforce)");
    EXPECT_EQ(result, "firefox_firefox.desktop");
}

TEST_F(DesktopFileManager, when_security_profile_is_valid_and_lacks_protection_indication_then_desktop_id_is_returned)
{
    auto result = mf::DesktopFileManager::parse_snap_security_profile_to_desktop_id("snap.firefox.firefox");
    EXPECT_EQ(result, "firefox_firefox.desktop");
}

TEST_F(DesktopFileManager, can_resolve_from_valid_flatpak_info)
{
    const char* app_id = "test.application.name";
    const char* desktop_app_id = "test.application.name.desktop";

    std::stringstream flatpak_info_path;
    flatpak_info_path << "/proc/" << PID << "/root/.flatpak-info";
    auto const flatpak_info = flatpak_info_path.str();
    auto tmp_file_name = std::tmpnam(NULL);
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

    auto new_file = std::make_shared<mf::DesktopFile>(desktop_app_id, nullptr, nullptr);
    cache->files.push_back(new_file);
    auto found_app_id = file_manager->resolve_app_id(&surface);
    EXPECT_THAT(found_app_id, Eq(desktop_app_id));
}

TEST_F(DesktopFileManager, app_id_will_not_resolve_from_flatpak_info_when_name_is_missing)
{
    const char* desktop_app_id = "test.application.name.desktop";

    std::stringstream flatpak_info_path;
    flatpak_info_path << "/proc/" << PID << "/root/.flatpak-info";
    auto const flatpak_info = flatpak_info_path.str();
    auto tmp_file_name = std::tmpnam(NULL);
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

    auto new_file = std::make_shared<mf::DesktopFile>(desktop_app_id, nullptr, nullptr);
    cache->files.push_back(new_file);
    auto found_app_id = file_manager->resolve_app_id(&surface);
    EXPECT_NE(found_app_id, desktop_app_id);
    EXPECT_EQ(found_app_id, APPLICATION_ID);
}
