/*
 * Copyright © 2016 Canonical Ltd.
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
 *
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_INPUT_CONFIGURATION_CHANGER_H_
#define MIR_INPUT_CONFIGURATION_CHANGER_H_

#include "mir/frontend/input_configuration_changer.h"
#include "mir/input/mir_input_config.h"
#include <map>
#include <mutex>
#include <vector>
#include <memory>

namespace mir
{
namespace scene
{
class SessionEventHandlerRegister;
class Session;
class SessionContainer;
}
namespace input
{
class Device;
class InputManager;
class InputDeviceHub;
class InputDeviceObserver;

class ConfigChanger : public frontend::InputConfigurationChanger
{
public:
    ConfigChanger(std::shared_ptr<InputManager> const& input_manager,
                  std::shared_ptr<InputDeviceHub> const& devices,
                  std::shared_ptr<scene::SessionContainer> const& session_container,
                  std::shared_ptr<scene::SessionEventHandlerRegister> const& session_event_handler_register,
                  std::shared_ptr<InputDeviceHub> const& devices_wrapper);
    ~ConfigChanger();
    MirInputConfig base_configuration() override;
    void configure(std::shared_ptr<scene::Session> const&, MirInputConfig &&) override;
    void set_base_configuration(MirInputConfig &&) override;

    void devices_updated(std::vector<std::shared_ptr<Device>> const& added, std::vector<MirInputDeviceId> const& removed);
private:
    void apply_config(MirInputConfig const& config);
    void apply_config_at_session(MirInputConfig const& config, std::shared_ptr<scene::Session> const& session);
    void apply_base_config();
    void send_base_config_to_all_sessions();
    void focus_change_handler(std::shared_ptr<scene::Session> const& session);
    void no_focus_handler();
    void session_stopping_handler(std::shared_ptr<scene::Session> const& session);
    std::shared_ptr<InputManager> const input_manager;
    std::shared_ptr<InputDeviceHub> const devices;
    std::shared_ptr<scene::SessionContainer> const session_container;
    std::shared_ptr<scene::SessionEventHandlerRegister> const session_event_handler_register;
    // needs to be owned (but not used) in Mir, where better?
    std::shared_ptr<InputDeviceHub> const devices_wrapper_DO_NOT_USE;
    std::shared_ptr<InputDeviceObserver> const device_observer;
    struct SessionObserver;
    std::unique_ptr<SessionObserver> const session_observer;
    bool base_configuration_applied;

    std::weak_ptr<scene::Session> focused_session;
    MirInputConfig base;
    std::mutex config_mutex;
    std::map<std::weak_ptr<scene::Session>, MirInputConfig, std::owner_less<std::weak_ptr<scene::Session>>> config_map;
};

}
}

#endif
