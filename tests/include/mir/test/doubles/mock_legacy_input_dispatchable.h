/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Cemil Azizoglu <cemil.azizoglu@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_MOCK_LEGACY_INPUT_DISPATCHABLE_H_
#define MIR_TEST_DOUBLES_MOCK_LEGACY_INPUT_DISPATCHABLE_H_

#include "mir/input/legacy_input_dispatchable.h"
#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{
struct MockLegacyInputDispatchable : input::LegacyInputDispatchable
{
    MOCK_METHOD0(start, void());
    MOCK_CONST_METHOD0(watch_fd, mir::Fd());
    MOCK_METHOD1(dispatch, bool(dispatch::FdEvents));
    MOCK_CONST_METHOD0(relevant_events, dispatch::FdEvents());
};
}
}
}
#endif // MIR_TEST_DOUBLES_MOCK_LEGACY_INPUT_DISPATCHABLE_H_
