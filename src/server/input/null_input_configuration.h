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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_INPUT_NULL_INPUT_CONFIGURATION_H_
#define MIR_INPUT_NULL_INPUT_CONFIGURATION_H_

#include "mir/input/input_configuration.h"

namespace mir
{
namespace input
{

class NullInputConfiguration : public InputConfiguration
{
public:
    NullInputConfiguration() = default;
    virtual ~NullInputConfiguration() = default;

    std::shared_ptr<scene::InputRegistrar> the_input_registrar();
    std::shared_ptr<shell::InputTargeter> the_input_targeter();
    std::shared_ptr<InputManager> the_input_manager();

    void set_input_targets(std::shared_ptr<InputTargets> const& /* targets */);

protected:
    NullInputConfiguration(const NullInputConfiguration&) = delete;
    NullInputConfiguration& operator=(const NullInputConfiguration&) = delete;
};

}
}

#endif // MIR_INPUT_NULL_INPUT_CONFIGURATION
