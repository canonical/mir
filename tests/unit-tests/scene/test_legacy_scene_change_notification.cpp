/*
 * Copyright © 2014 Canonical Ltd.
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
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "mir/scene/legacy_scene_change_notification.h"
#include "mir/scene/surface_observer.h"

#include "mir/test/fake_shared.h"
#include "mir/test/doubles/mock_surface.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace ms = mir::scene;
namespace mt = mir::test;
namespace mtd = mt::doubles;

namespace
{
struct MockSceneCallback
{
    MOCK_METHOD0(invoke, void());
};
struct MockBufferCallback
{
    MOCK_METHOD1(invoke, void(int));
};

struct LegacySceneChangeNotificationTest : public testing::Test
{
    void SetUp() override
    {
        surface = std::make_shared<testing::NiceMock<mtd::MockSurface>>();
        ON_CALL(*surface, visible()).WillByDefault(testing::Return(true));
    }
    testing::NiceMock<MockSceneCallback> scene_callback;
    testing::NiceMock<MockBufferCallback> buffer_callback;
    std::function<void(int)> buffer_change_callback{[this](int arg){buffer_callback.invoke(arg);}};
    std::function<void()> scene_change_callback{[this](){scene_callback.invoke();}};
    std::shared_ptr<testing::NiceMock<mtd::MockSurface>> surface;
}; 
}

TEST_F(LegacySceneChangeNotificationTest, fowards_all_observations_to_callback)
{
    EXPECT_CALL(scene_callback, invoke()).Times(2);

    ms::LegacySceneChangeNotification observer(scene_change_callback, buffer_change_callback);
    observer.surface_added(surface);
    observer.surface_removed(surface);
    observer.surfaces_reordered({});
}

TEST_F(LegacySceneChangeNotificationTest, registers_observer_with_surfaces)
{
    EXPECT_CALL(*surface, add_observer(testing::_))
        .Times(1);

    ms::LegacySceneChangeNotification observer(scene_change_callback, buffer_change_callback);
    observer.surface_added(surface);
}

TEST_F(LegacySceneChangeNotificationTest, registers_observer_with_existing_surfaces)
{
    EXPECT_CALL(*surface, add_observer(testing::_))
        .Times(1);

    ms::LegacySceneChangeNotification observer(scene_change_callback, buffer_change_callback);
    observer.surface_exists(surface);
}

TEST_F(LegacySceneChangeNotificationTest, observes_surface_changes)
{
    using namespace ::testing;
    std::shared_ptr<ms::SurfaceObserver> surface_observer;
    EXPECT_CALL(*surface, add_observer(_)).Times(1)
        .WillOnce(SaveArg<0>(&surface_observer));
   
    int buffer_num{3}; 
    EXPECT_CALL(scene_callback, invoke()).Times(0);
    EXPECT_CALL(buffer_callback, invoke(buffer_num)).Times(1);

    ms::LegacySceneChangeNotification observer(scene_change_callback, buffer_change_callback);
    observer.surface_added(surface);
    surface_observer->frame_posted(surface.get(), buffer_num, mir::geometry::Size{0, 0});
}

TEST_F(LegacySceneChangeNotificationTest, redraws_on_rename)
{
    using namespace ::testing;

    std::shared_ptr<ms::SurfaceObserver> surface_observer;

    EXPECT_CALL(*surface, add_observer(_)).Times(1)
        .WillOnce(SaveArg<0>(&surface_observer));
    EXPECT_CALL(scene_callback, invoke()).Times(1);

    ms::LegacySceneChangeNotification observer(scene_change_callback,
                                               buffer_change_callback);
    observer.surface_added(surface);
    surface_observer->renamed(surface.get(), "Something New");
}

TEST_F(LegacySceneChangeNotificationTest, destroying_observer_unregisters_surface_observers)
{
    using namespace ::testing;
    
    EXPECT_CALL(*surface, add_observer(_))
        .Times(1);
    EXPECT_CALL(*surface, remove_observer(_))
        .Times(1);
    {
        ms::LegacySceneChangeNotification observer(scene_change_callback, buffer_change_callback);
        observer.surface_added(surface);
    }
}

TEST_F(LegacySceneChangeNotificationTest, ending_observation_unregisters_observers)
{
    using namespace ::testing;
    EXPECT_CALL(*surface, add_observer(_))
        .Times(1);
    EXPECT_CALL(*surface, remove_observer(_))
        .Times(1);

    ms::LegacySceneChangeNotification observer(scene_change_callback, buffer_change_callback);
    observer.surface_added(surface);
    observer.end_observation();

    // Verify that its not simply the destruction removing the observer...
    ::testing::Mock::VerifyAndClearExpectations(&observer);
}
