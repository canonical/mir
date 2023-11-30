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
#include "mir/test/doubles/mock_surface.h"
#include "mir/test/doubles/explicit_executor.h"
#include "mir/test/fake_shared.h"

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace mw = mir::wayland;
namespace geom = mir::geometry;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

using namespace testing;

struct DesktopFileManager : Test
{
    std::shared_ptr<mf::DesktopFileManager> file_manager;

    void SetUp() override
    {

    }
};