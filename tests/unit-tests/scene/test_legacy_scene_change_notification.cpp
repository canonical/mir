/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "mir/scene/legacy_scene_change_notification.h"
#include "mir/scene/surface_observer.h"

#include "mir_test/fake_shared.h"
#include "mir_test_doubles/mock_surface.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace ms = mir::scene;
namespace mt = mir::test;
namespace mtd = mt::doubles;

namespace
{
struct MockCallback
{
    MOCK_METHOD0(invoke, void());
};
}

TEST(LegacySceneChangeNotificationTest, fowards_all_observations_to_callback)
{
    MockCallback callback;
    ms::LegacySceneChangeNotification observer([&](){ callback.invoke(); });
    
    testing::NiceMock<mtd::MockSurface> surface;
    
    EXPECT_CALL(callback, invoke()).Times(3);
    observer.surface_added(mt::fake_shared(surface));
    observer.surface_removed(mt::fake_shared(surface));
    observer.surfaces_reordered();
}

TEST(LegacySceneChangeNotificationTest, registers_observer_with_surfaces)
{
    using namespace ::testing;

    mtd::MockSurface surface;
    ms::LegacySceneChangeNotification observer([](){});
    
    EXPECT_CALL(surface, add_observer(_)).Times(1);
    observer.surface_added(mt::fake_shared(surface));
}

TEST(LegacySceneChangeNotification, observes_surface_changes)
{
    using namespace ::testing;
    
    mtd::MockSurface surface;
    MockCallback callback;
    ms::LegacySceneChangeNotification observer([&](){ callback.invoke(); });
    
    std::shared_ptr<ms::SurfaceObserver> surface_observer;
    EXPECT_CALL(surface, add_observer(_)).Times(1)
        .WillOnce(SaveArg<0>(&surface_observer));
    
    // Once for surface added and once for surface change
    EXPECT_CALL(callback, invoke()).Times(2);
    observer.surface_added(mt::fake_shared(surface));
    
    surface_observer->frame_posted();
}

TEST(LegacySceneChangeNotification, destroying_observer_unregisters_surface_observers)
{
    using namespace ::testing;
    
    auto surface = std::make_shared<mtd::MockSurface>();
    
    EXPECT_CALL(*surface, add_observer(_)).Times(1);
    EXPECT_CALL(*surface, remove_observer(_)).Times(1);

    {
        ms::LegacySceneChangeNotification observer([](){});
        observer.surface_added(surface);
    }
}
