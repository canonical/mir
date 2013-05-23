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

#include "src/server/input/android/android_input_targeter.h"
#include "src/server/input/android/android_input_registrar.h"
#include "src/server/input/android/android_window_handle_repository.h"

#include "mir/input/surface_target.h"

#include "mir_test_doubles/mock_input_dispatcher.h"
#include "mir_test_doubles/stub_surface_target.h"

#include "mir_test/fake_shared.h"

#include <InputWindow.h>
#include <InputApplication.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <stdexcept>

namespace mi = mir::input;
namespace mia = mi::android;
namespace mt = mir::test;
namespace mtd = mt::doubles;
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
        return new StubInputWindowHandle(surface->name());
    }
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

static bool
application_handle_matches_surface(droidinput::sp<droidinput::InputApplicationHandle> const& handle,
                                   std::shared_ptr<mi::SurfaceTarget> const& surface)
{
   if (handle->getName() != droidinput::String8(surface->name().c_str()))
        return false;
   return true;
}

static bool
window_handle_matches_surface(droidinput::sp<droidinput::InputWindowHandle> const& handle,
                              std::shared_ptr<mi::SurfaceTarget> const& surface)
{
    if (handle->getName() != surface->name())
        return false;
    return true;
}

MATCHER_P(WindowHandleFor, surface, "")
{
    return window_handle_matches_surface(arg, surface);
}

MATCHER_P(ApplicationHandleFor, surface, "")
{
    return application_handle_matches_surface(arg, surface);
}

MATCHER_P(VectorContainingWindowHandleFor, surface, "")
{
    auto i = arg.size();
    for (i = 0; i < arg.size(); i++)
    {
        if (window_handle_matches_surface(arg[i], surface))
            return true;
    }
    return false;
}

MATCHER(EmptyVector, "")
{
    return arg.size() == 0;
}

}

TEST(AndroidInputTargeterSetup, on_focus_cleared)
{
    using namespace ::testing;

    droidinput::sp<mtd::MockInputDispatcher> dispatcher = new mtd::MockInputDispatcher;

    EXPECT_CALL(*dispatcher, setFocusedApplication(droidinput::sp<droidinput::InputApplicationHandle>(0))).Times(1);
    EXPECT_CALL(*dispatcher, setInputWindows(EmptyVector())).Times(1);

    StubWindowHandleRepository repository;
    mia::InputTargeter targeter(dispatcher, mt::fake_shared(repository));
    
    targeter.focus_cleared();
}

TEST(AndroidInputTargeterSetup, on_focus_changed)
{
    using namespace ::testing;

    droidinput::sp<mtd::MockInputDispatcher> dispatcher = new mtd::MockInputDispatcher;
    auto surface = std::make_shared<StubSurfaceTarget>("test_focus_changed");
    {
        InSequence seq;
        EXPECT_CALL(*dispatcher, setFocusedApplication(ApplicationHandleFor(surface))).Times(1);
        EXPECT_CALL(*dispatcher, setInputWindows(VectorContainingWindowHandleFor(surface))).Times(1);
    }

    StubWindowHandleRepository repository;
    mia::InputTargeter targeter(dispatcher, mt::fake_shared(repository));
    
    targeter.focus_changed(surface);
}

TEST(AndroidInputTargeterSetup, on_focus_changed_throw_behavior)
{
    using namespace ::testing;

    droidinput::sp<mtd::MockInputDispatcher> dispatcher = new mtd::MockInputDispatcher;
    StubWindowHandleRepository repository;
    mia::InputTargeter targeter(dispatcher, mt::fake_shared(repository));

    auto surface = std::make_shared<StubSurfaceTarget>("test_focus_changed_throw");

    EXPECT_THROW({
            // We can't focus surfaces which never opened
            targeter.focus_changed(surface);
    }, std::logic_error);
}
