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

#ifndef MIR_TEST_DOUBLES_MOCK_CURSOR_OBSERVER_H
#define MIR_TEST_DOUBLES_MOCK_CURSOR_OBSERVER_H

#include "mir/input/cursor_observer.h"

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

struct MockCursorObserver : public input::CursorObserver
{
    MOCK_METHOD(void, cursor_moved_to, (float, float), (override));
    ~MockCursorObserver() noexcept {}
    void pointer_usable() {}
    void pointer_unusable() {}
};

}
}
}

#endif
