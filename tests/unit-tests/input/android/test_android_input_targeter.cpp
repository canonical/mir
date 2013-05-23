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
struct MockWindowHandleRepository : public mia::WindowHandleRepository
{
    ~MockWindowHandleRepository() noexcept(true) {};
    MOCK_METHOD1(handle_for_surface, droidinput::sp<droidinput::InputWindowHandle>(std::shared_ptr<mi::SurfaceTarget const> const& surface));
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

MATCHER_P(WindowHandleFor, surface, "")
{
    if (arg->getName() != surface->name())
        return false;
    return true;
}

}

TEST(AndroidInputTargeterSetup, on_focus_cleared)
{
    using namespace ::testing;

    droidinput::sp<mtd::MockInputDispatcher> dispatcher = new mtd::MockInputDispatcher;

    EXPECT_CALL(*dispatcher, setKeyboardFocus(droidinput::sp<droidinput::InputWindowHandle>(0))).Times(1);

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
        EXPECT_CALL(*dispatcher, setKeyboardFocus(WindowHandleFor(surface))).Times(1);
    }

    StubWindowHandleRepository repository;
    mia::InputTargeter targeter(dispatcher, mt::fake_shared(repository));
    
    targeter.focus_changed(surface);
}

TEST(AndroidInputTargeterSetup, on_focus_changed_throw_behavior)
{
    using namespace ::testing;

    droidinput::sp<mtd::MockInputDispatcher> dispatcher = new mtd::MockInputDispatcher;
    MockWindowHandleRepository repository;
    mia::InputTargeter targeter(dispatcher, mt::fake_shared(repository));

    auto surface = std::make_shared<StubSurfaceTarget>("test_focus_changed_throw");

    EXPECT_CALL(repository, handle_for_surface(_)).Times(1)
        .WillOnce(Return(droidinput::sp<droidinput::InputWindowHandle>()));
    EXPECT_THROW({
            // We can't focus surfaces which never opened
            targeter.focus_changed(surface);
    }, std::logic_error);
}
