/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_TEST_INPUT_DEVICES_MATCHER_H_
#define MIR_TEST_INPUT_DEVICES_MATCHER_H_

#include "mir/input/device.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <algorithm>

namespace mir
{
namespace protobuf
{
static void PrintTo(mir::protobuf::InputDeviceInfo const &, ::std::ostream*) __attribute__ ((unused));
void PrintTo(mir::protobuf::InputDeviceInfo const&, ::std::ostream*) {}
}

namespace test
{
MATCHER_P(InputDevicesMatch, devices, "")
{
    using std::begin;
    using std::end;
    if (distance(begin(devices), end(devices)) != distance(begin(arg), end(arg)))
        return false;
    return std::equal(begin(arg), end(arg),
                      begin(devices),
                      [](auto const& lhs, std::shared_ptr<input::Device> const& rhs)
                      {
                          return lhs.id() == rhs->id() &&
                              lhs.name() == rhs->name() &&
                              lhs.unique_id() == rhs->unique_id() &&
                              lhs.capabilities() == rhs->capabilities().value();
                      });

}
}
}

#endif
