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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */
#include <InputApplication.h>
#include <InputWindow.h>

#ifndef MIR_TEST_DOUBLES_STUB_INPUT_HANDLES_H_
#define MIR_TEST_DOUBLES_STUB_INPUT_HANDLES_H_

namespace mir
{
namespace test
{
namespace doubles
{

struct StubInputApplicationHandle : public droidinput::InputApplicationHandle
{
    StubInputApplicationHandle()
    {
        if (!mInfo)
        {
            mInfo = new droidinput::InputApplicationInfo;
        }
    }
    bool updateInfo()
    {
        return true;
    }
};

struct StubWindowHandle : public droidinput::InputWindowHandle
{
    StubWindowHandle()
      : droidinput::InputWindowHandle(make_app_handle())
    {
        if (!mInfo)
        {
            mInfo = new droidinput::InputWindowInfo();
        }
    }

    droidinput::sp<droidinput::InputApplicationHandle> make_app_handle()
    {
        return new StubInputApplicationHandle;
    }

    bool updateInfo()
    {
        return true;
    }

    int64_t publishMotionEvent(int32_t /* deviceId */,
        int32_t /* source */,
        int32_t /* action */,
        int32_t /* flags */,
        int32_t /* edgeFlags */,
        int32_t /* metaState */,
        int32_t /* buttonState */,
        float /* xOffset */,
        float /* yOffset */,
        float /* xPrecision */,
        float /* yPrecision */,
        std::chrono::nanoseconds /* downTime */,
        std::chrono::nanoseconds /* eventTime */,
        size_t /* pointerCount */,
        droidinput::PointerProperties const* /*  pointerProperties */,
        droidinput::PointerCoords const* /* pointerCoords */) override
     {
         return 0;
     }

    int64_t publishKeyEvent(
        int32_t /* deviceId */,
        int32_t /* source */,
        int32_t /* action */,
        int32_t /* flags */,
        int32_t /* keyCode */,
        int32_t /* scanCode */,
        int32_t /* metaState */,
        int32_t /* repeatCount */,
        std::chrono::nanoseconds /* downTime */,
        std::chrono::nanoseconds /* eventTime */)
    {
        return 0;
    }
};

}
}
}

#endif /* MIR_TEST_DOUBLES_STUB_INPUT_HANDLES_H_ */
