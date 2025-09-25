/*
 * Copyright (C) Canonical, Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3, as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranties of MERCHANTABILITY,
 * SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIROIL_DISPLAY_CONFIGURATION_CONTROLLER_WRAPPER_H_
#define MIROIL_DISPLAY_CONFIGURATION_CONTROLLER_WRAPPER_H_

#include <memory>

namespace mir { namespace shell { class DisplayConfigurationController; } }
namespace mir { namespace graphics { class DisplayConfiguration; } }

namespace miroil
{

class DisplayConfigurationControllerWrapper
{
public:
    DisplayConfigurationControllerWrapper(std::shared_ptr<mir::shell::DisplayConfigurationController> const & wrapped);
    ~DisplayConfigurationControllerWrapper() = default;

    /**
     * Set the base display configuration.
     *
     * This is the display configuration that is used by default, but will be
     * overridden by a client's requested configuration if that client is focused.
     *
     * \param [in]  conf    The new display configuration to set
     */
    void set_base_configuration(std::shared_ptr<mir::graphics::DisplayConfiguration> const& conf);

private:
    std::shared_ptr<mir::shell::DisplayConfigurationController> const & wrapped;
};

}

#endif //MIROIL_DISPLAY_CONFIGURATION_CONTROLLER_WRAPPER_H_
