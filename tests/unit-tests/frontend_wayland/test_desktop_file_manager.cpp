/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "src/server/frontend_wayland/desktop_file_manager.h"
#include "mir/scene/observer.h"
#include "mir/test/doubles/mock_surface.h"
#include "mir/test/doubles/mock_scene_session.h"
#include "mir/scene/surface_observer.h"
#include "mir/test/doubles/mock_frontend_surface_stack.h"
#include "mir/test/doubles/explicit_executor.h"
#include "mir/test/fake_shared.h"
#include <gmock/gmock.h>

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace geom = mir::geometry;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

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

    std::vector<std::shared_ptr<mf::DesktopFile>> const& get_desktop_files() const override
    {
        return files;
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

TEST_F(DesktopFileManager, when_all_fail_application_id_is_returned)
{
    auto app_id = file_manager->resolve_app_id(&surface);
    EXPECT_THAT(app_id, Eq(APPLICATION_ID));
}