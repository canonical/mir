/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "src/server/graphics/android/android_framebuffer_window.h"

#include "mir_test_doubles/mock_egl.h"

#include <memory>
#include <system/window.h>
#include <gtest/gtest.h>
#include <stdexcept>

namespace mga = mir::graphics::android;
namespace mtd = mir::test::doubles;

class ANativeWindowInterface
{
public:
    virtual ~ANativeWindowInterface() = default;
    virtual int query_interface(const ANativeWindow* win , int code, int* value) const = 0;
};
class MockANativeWindow : public ANativeWindowInterface,
    public ANativeWindow
{
public:
    MockANativeWindow()
        : fake_visual_id(5)
    {
        using namespace testing;

        query = hook_query;

        ON_CALL(*this, query_interface(_,_,_))
        .WillByDefault(DoAll(
                           SetArgPointee<2>(fake_visual_id),
                           Return(0)));
    }

    ~MockANativeWindow() noexcept {}

    static int hook_query(const ANativeWindow* anw, int code, int *ret)
    {
        const MockANativeWindow* mocker = static_cast<const MockANativeWindow*>(anw);
        return mocker->query_interface(anw, code, ret);
    }

    MOCK_CONST_METHOD3(query_interface,int(const ANativeWindow*,int,int*));

    int fake_visual_id;
};

struct FramebufferInfoTest : public ::testing::Test
{
    virtual void SetUp()
    {
        mock_egl.silence_uninteresting();
    }

    MockANativeWindow mock_anw;
    mtd::MockEGL mock_egl;
};

