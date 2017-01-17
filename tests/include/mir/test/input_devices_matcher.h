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

namespace mir
{
namespace test
{
MATCHER_P(InputConfigurationMatches, devices, "")
{
    if (devices.size() != arg.size())
        return false;
    auto it = begin(devices);
    auto result = true;
    arg.for_each(
        [&it, &result](auto const& conf)
        {
           auto const device = *it++;
           if (conf.id() != device->id() ||
               conf.name() != device->name() ||
               conf.unique_id() != device->unique_id() ||
               conf.capabilities() != device->capabilities())
               result = false;
        });
    return result;
}
}
}

#endif
