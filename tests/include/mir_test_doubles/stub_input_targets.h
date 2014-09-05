/*
 * Copyright © 2014 Canonical Ltd.
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

#ifndef MIR_TEST_DOUBLES_STUB_INPUT_TARGETS_H_
#define MIR_TEST_DOUBLES_STUB_INPUT_TARGETS_H_

#include "mir/input/input_targets.h"

namespace mir
{
namespace test
{
namespace doubles
{

class StubInputTargets : public input::InputTargets
{
    void for_each(std::function<void(std::shared_ptr<input::Surface> const&)> const& ) override
    {
    }
    void add_observer(std::shared_ptr<scene::Observer> const& /* observer */)
    {
    }
    void remove_observer(std::weak_ptr<scene::Observer> const& /* observer */)
    {
    }
};

}
}
} // namespace mir

#endif /* MIR_TEST_DOUBLES_STUB_INPUT_TARGETS_H_ */
