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

#include "mir/scene/scene_change_notification.h"
#include "mir/scene/surface_observer.h"

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
    MOCK_METHOD2(invoke, void(int, mir::geometry::Rectangle const&));
};

struct SceneChangeNotificationTest : public testing::Test
{
    void SetUp() override
    {
        surface = std::make_shared<testing::NiceMock<mtd::MockSurface>>();
        ON_CALL(*surface, visible()).WillByDefault(testing::Return(true));
    }
    testing::NiceMock<MockSceneCallback> scene_callback;
    testing::NiceMock<MockBufferCallback> buffer_callback;
    std::function<void(int, mir::geometry::Rectangle const&)> buffer_change_callback{[this](int arg, mir::geometry::Rectangle const& damage)
        {
            buffer_callback.invoke(arg, damage);
        }};
    std::function<void()> scene_change_callback{[this](){scene_callback.invoke();}};
    std::shared_ptr<testing::NiceMock<mtd::MockSurface>> surface;
}; 
}

TEST_F(SceneChangeNotificationTest, fowards_all_observations_to_callback)
{
    EXPECT_CALL(scene_callback, invoke()).Times(3);

    ms::SceneChangeNotification observer(scene_change_callback, buffer_change_callback);
    observer.surface_added(surface);
    observer.surface_removed(surface);
    observer.surfaces_reordered({});
}

TEST_F(SceneChangeNotificationTest, registers_observer_with_surfaces)
{
    EXPECT_CALL(*surface, register_interest(testing::_))
        .Times(1);

    ms::SceneChangeNotification observer(scene_change_callback, buffer_change_callback);
    observer.surface_added(surface);
}

TEST_F(SceneChangeNotificationTest, registers_observer_with_existing_surfaces)
{
    EXPECT_CALL(*surface, register_interest(testing::_))
        .Times(1);

    ms::SceneChangeNotification observer(scene_change_callback, buffer_change_callback);
    observer.surface_exists(surface);
}

TEST_F(SceneChangeNotificationTest, observes_surface_changes)
{
    using namespace ::testing;
    std::weak_ptr<ms::SurfaceObserver> surface_observer;
    EXPECT_CALL(*surface, register_interest(_)).Times(1)
        .WillOnce(SaveArg<0>(&surface_observer));
   
    int buffer_num{1};
    EXPECT_CALL(scene_callback, invoke()).Times(1);
    EXPECT_CALL(buffer_callback, invoke(buffer_num, _)).Times(1);

    ms::SceneChangeNotification observer(scene_change_callback, buffer_change_callback);
    observer.surface_added(surface);
    surface_observer.lock()->frame_posted(surface.get(), {});
}

TEST_F(SceneChangeNotificationTest, redraws_on_rename)
{
    using namespace ::testing;

    std::weak_ptr<ms::SurfaceObserver> surface_observer;

    EXPECT_CALL(*surface, register_interest(_)).Times(1)
        .WillOnce(SaveArg<0>(&surface_observer));

    ms::SceneChangeNotification observer(scene_change_callback,
                                               buffer_change_callback);
    observer.surface_added(surface);

    EXPECT_CALL(scene_callback, invoke()).Times(1);
    surface_observer.lock()->renamed(surface.get(), "Something New");
}

TEST_F(SceneChangeNotificationTest, destroying_observer_unregisters_surface_observers)
{
    using namespace ::testing;
    
    EXPECT_CALL(*surface, register_interest(_))
        .Times(1);
    EXPECT_CALL(*surface, unregister_interest(_))
        .Times(1);
    {
        ms::SceneChangeNotification observer(scene_change_callback, buffer_change_callback);
        observer.surface_added(surface);
    }
}

TEST_F(SceneChangeNotificationTest, ending_observation_unregisters_observers)
{
    using namespace ::testing;
    EXPECT_CALL(*surface, register_interest(_))
        .Times(1);
    EXPECT_CALL(*surface, unregister_interest(_))
        .Times(1);

    ms::SceneChangeNotification observer(scene_change_callback, buffer_change_callback);
    observer.surface_added(surface);
    observer.end_observation();

    // Verify that its not simply the destruction removing the observer...
    ::testing::Mock::VerifyAndClearExpectations(&observer);
}
