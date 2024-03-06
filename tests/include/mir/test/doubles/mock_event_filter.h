/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_TEST_DOUBLES_MOCK_EVENT_FILTER_H_
#define MIR_TEST_DOUBLES_MOCK_EVENT_FILTER_H_

#include "mir/input/event_filter.h"

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{
struct MockEventFilter : public mir::input::EventFilter
{
    MOCK_METHOD(bool, handle, (const MirEvent&), (override));
};
}
}
}

#endif // MIR_TEST_DOUBLES_MOCK_EVENT_FILTER_H_
