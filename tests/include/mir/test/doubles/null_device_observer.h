/*
 * Copyright © 2018 Canonical Ltd.
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
 *
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_NULL_DEVICE_OBSERVER_H_
#define MIR_TEST_DOUBLES_NULL_DEVICE_OBSERVER_H_

#include "mir/console_services.h"

namespace mir
{
namespace test
{
namespace doubles
{
class NullDeviceObserver : public mir::Device::Observer
{
public:
    NullDeviceObserver() = default;

    void activated(mir::Fd&&) override {}
    void suspended() override {}
    void removed() override {}
};
}
}
}

#endif //MIR_TEST_DOUBLES_NULL_DEVICE_OBSERVER_H_
