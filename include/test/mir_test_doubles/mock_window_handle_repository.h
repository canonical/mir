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

#include "src/server/input/android/android_window_handle_repository.h"
#include <gmock/gmock.h>

#ifndef MIR_TEST_DOUBLES_MOCK_WINDOW_HANDLE_REPOSITORY_H_
#define MIR_TEST_DOUBLES_MOCK_WINDOW_HANDLE_REPOSITORY_H_

namespace mir
{
namespace test
{
namespace doubles
{

struct MockWindowHandleRepository : public input::android::WindowHandleRepository
{
    MockWindowHandleRepository()
    {
        using namespace testing;
        ON_CALL(*this, handle_for_channel(_))
            .WillByDefault(Return(droidinput::sp<droidinput::InputWindowHandle>()));
    }
    ~MockWindowHandleRepository() noexcept(true) {};
    MOCK_METHOD1(handle_for_channel,
        droidinput::sp<droidinput::InputWindowHandle>(std::shared_ptr<input::InputChannel const> const&));
};

}
}
}

#endif /* MIR_TEST_DOUBLES_MOCK_WINDOW_HANDLE_REPOSITORY_H_ */
