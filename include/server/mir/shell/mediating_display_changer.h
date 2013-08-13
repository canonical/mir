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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_SHELL_MEDIATING_DISPLAY_CHANGER_H_
#define MIR_SHELL_MEDIATING_DISPLAY_CHANGER_H_

#include "mir/frontend/display_changer.h"
#include "mir/display_changer.h"

#include <mutex>

namespace mir
{

namespace graphics
{
    class Display;
    class DisplayConfigurationPolicy;
}
namespace compositor { class Compositor; }
namespace input { class InputManager; }

namespace shell
{

class MediatingDisplayChanger : public frontend::DisplayChanger,
                                public mir::DisplayChanger
{
public:
    MediatingDisplayChanger(
        std::shared_ptr<graphics::Display> const& display,
        std::shared_ptr<compositor::Compositor> const& compositor,
        std::shared_ptr<input::InputManager> const& input_manager,
        std::shared_ptr<graphics::DisplayConfigurationPolicy> const& display_configuration_policy);

    /* From mir::frontend::DisplayChanger */
    std::shared_ptr<graphics::DisplayConfiguration> active_configuration();
    void configure(std::weak_ptr<frontend::Session> const& session,
                   std::shared_ptr<graphics::DisplayConfiguration> const& conf);

    /* From mir::DisplayChanger */
    void configure_for_hardware_change(
        std::shared_ptr<graphics::DisplayConfiguration> const& conf,
        SystemStateHandling pause_resume_system);

private:
    void apply_config(std::shared_ptr<graphics::DisplayConfiguration> const& conf,
                      SystemStateHandling pause_resume_system);

    std::shared_ptr<graphics::Display> const display;
    std::shared_ptr<compositor::Compositor> const compositor;
    std::shared_ptr<input::InputManager> const input_manager;
    std::shared_ptr<graphics::DisplayConfigurationPolicy> const display_configuration_policy;
    std::mutex configuration_mutex;
};

}
}

#endif /* MIR_SHELL_MEDIATING_DISPLAY_CHANGER_H_ */
