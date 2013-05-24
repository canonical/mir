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

#include "mir/input/null_input_registrar.h"
#include "mir/input/null_input_targeter.h"
#include "mir/input/null_input_manager.h"

namespace mir
{
namespace input
{

class NullInputConfiguration : public InputConfiguration
{
public:
    NullInputConfiguration() {};
    virtual ~NullInputConfiguration() {}

    std::shared_ptr<surfaces::InputRegistrar> the_input_registrar()
    {
        return std::make_shared<NullInputRegistrar>();
    }
    std::shared_ptr<shell::InputTargeter> the_input_targeter()
    {
        return std::make_shared<NullInputTargeter>();
    }
    std::shared_ptr<InputManager> the_input_manager()
    {
        return std::make_shared<NullInputManager>();
    }
    
    void set_input_targets(std::shared_ptr<InputTargets> const& /* targets */)
    {
    }

protected:
    NullInputConfiguration(const NullInputConfiguration&) = delete;
    NullInputConfiguration& operator=(const NullInputConfiguration&) = delete;
};

}
}

#endif // MIR_INPUT_NULL_INPUT_CONFIGURATION
