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
#include "mir/scene/surface_observer.h"
#include "mir/test/doubles/mock_frontend_surface_stack.h"
#include "mir/test/doubles/explicit_executor.h"
#include "mir/test/fake_shared.h"

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

    void SetUp() override
    {
        cache = std::make_shared<InMemoryDesktopFileCache>();
        file_manager = std::make_shared<mf::DesktopFileManager>(cache);
    }
};

