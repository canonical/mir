/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "src/server/input/android/android_input_target_enumerator.h"
#include "src/server/input/android/android_window_handle_repository.h"

#include "mir/input/surface_target.h"
#include "mir/input/input_targets.h"

#include "mir_test/fake_shared.h"

#include <InputApplication.h>
#include <InputWindow.h>
#include <InputDispatcher.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <string>
#include <initializer_list>
#include <map>

namespace mi = mir::input;
namespace mia = mir::input::android;
namespace mt = mir::test;
namespace geom = mir::geometry;

namespace
{

struct StubInputApplicationHandle : public droidinput::InputApplicationHandle
{
    bool updateInfo()
    {
        return true;
    }
};

struct StubInputWindowHandle : public droidinput::InputWindowHandle
{
    StubInputWindowHandle(std::string const& name)
        : droidinput::InputWindowHandle(new StubInputApplicationHandle)
    {
        mInfo = new droidinput::InputWindowInfo();
        mInfo->name = droidinput::String8(name);
    }
          
    bool updateInfo()
    {
        return true;
    }
};

struct StubWindowHandleRepository : public mia::WindowHandleRepository
{
    droidinput::sp<droidinput::InputWindowHandle> handle_for_surface(std::shared_ptr<mi::SurfaceTarget const> const& surface)
    {
        if (handles.find(surface) == handles.end())
            handles[surface] = new StubInputWindowHandle(surface->name());
        
        return handles[surface];
    }
    std::map<std::shared_ptr<mi::SurfaceTarget const>, droidinput::sp<droidinput::InputWindowHandle>> handles;
};

struct StubSurfaceTarget : public mi::SurfaceTarget
{
    StubSurfaceTarget(std::string const& name)
        : target_name(name)
    {
    }
    std::string const& name() const override
    {
        return target_name;
    }

    int server_input_fd() const override
    {
        return 0;
    }
    geom::Size size() const override
    {
        return geom::Size();
    }
    geom::Point top_left() const override
    {
        return geom::Point();
    }

    std::string const target_name;
};

struct StubInputTargets : public mi::InputTargets
{
    StubInputTargets(std::initializer_list<std::shared_ptr<mi::SurfaceTarget>> const& target_list)
        : targets(target_list.begin(), target_list.end())
    {
    }
    
    void for_each(std::function<void(std::shared_ptr<mi::SurfaceTarget> const&)> const& callback) override
    {
        for (auto target : targets)
            callback(target);
    }
    
    std::vector<std::shared_ptr<mi::SurfaceTarget>> targets;
};

MATCHER_P(HandleWithName, name, "")
{
    return arg->getName() == name;
}

}

TEST(AndroidInputTargetEnumerator, enumerates_registered_handles_for_surfaces)
{
    using namespace ::testing;

    std::string const surface1_name = "Pacific";
    std::string const surface2_name = "Atlantic";
    
    struct MockTargetObserver
    {
        MOCK_METHOD1(see, void(droidinput::sp<droidinput::InputWindowHandle> const&));
    } observer;
    {
        InSequence seq;
        EXPECT_CALL(observer, see(HandleWithName(surface1_name))).Times(1);
        EXPECT_CALL(observer, see(HandleWithName(surface2_name))).Times(1);
    }

    StubSurfaceTarget t1(surface1_name), t2(surface2_name);
    StubInputTargets targets({mt::fake_shared(t1), mt::fake_shared(t2)});
    StubWindowHandleRepository handles;
    
    // The InputTargetEnumerator only holds a weak reference to the targets so we need to hold a shared pointer.
    auto shared_targets = mt::fake_shared(targets);
    auto shared_handles = mt::fake_shared(handles);

    mia::InputTargetEnumerator enumerator(shared_targets, shared_handles);
    enumerator.for_each([&observer](droidinput::sp<droidinput::InputWindowHandle> const& handle)
        {
            observer.see(handle);
        });                        
}
