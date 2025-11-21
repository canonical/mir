/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_SCENE_MEDIATING_DISPLAY_CHANGER_H_
#define MIR_SCENE_MEDIATING_DISPLAY_CHANGER_H_

#include <mir/frontend/display_changer.h>
#include <mir/display_changer.h>
#include <mir/shell/display_configuration_controller.h>

#include <mutex>
#include <map>
#include <mir/graphics/display_configuration.h>

namespace mir
{
class ServerActionQueue;

namespace time
{
class Alarm;
class AlarmFactory;
}

namespace graphics
{
    class Display;
    class DisplayConfigurationPolicy;
    class DisplayConfigurationObserver;
}
namespace compositor { class Compositor; }
namespace scene
{
class SessionEventHandlerRegister;
class SessionContainer;
class Session;

class MediatingDisplayChanger : public frontend::DisplayChanger,
                                public mir::DisplayChanger,
                                public shell::DisplayConfigurationController
{
public:
    MediatingDisplayChanger(
        std::shared_ptr<graphics::Display> const& display,
        std::shared_ptr<compositor::Compositor> const& compositor,
        std::shared_ptr<graphics::DisplayConfigurationPolicy> const& display_configuration_policy,
        std::shared_ptr<SessionContainer> const& session_container,
        std::shared_ptr<SessionEventHandlerRegister> const& session_event_handler_register,
        std::shared_ptr<ServerActionQueue> const& server_action_queue,
        std::shared_ptr<graphics::DisplayConfigurationObserver> const& observer,
        std::shared_ptr<time::AlarmFactory> const& alarm_factory);

    ~MediatingDisplayChanger();

        /* From mir::frontend::DisplayChanger */
    std::shared_ptr<graphics::DisplayConfiguration> base_configuration() override;
    void configure(std::shared_ptr<scene::Session> const& session,
                   std::shared_ptr<graphics::DisplayConfiguration> const& conf) override;

    /* From mir::DisplayChanger */
    void configure(std::shared_ptr<graphics::DisplayConfiguration> const& conf) override;

    void pause_display_config_processing() override;
    void resume_display_config_processing() override;

    /* From shell::DisplayConfigurationController */
    void set_base_configuration(std::shared_ptr<graphics::DisplayConfiguration> const &conf) override;
    void set_power_mode(MirPowerMode new_power_mode) override;

private:
    void focus_change_handler(std::shared_ptr<Session> const& session);
    void no_focus_handler();
    void session_stopping_handler(std::shared_ptr<Session> const& session);

    void apply_config(std::shared_ptr<graphics::DisplayConfiguration> const& conf);
    void apply_base_config();
    void send_config_to_all_sessions(
        std::shared_ptr<graphics::DisplayConfiguration> const& conf);

    std::shared_ptr<graphics::Display> const display;
    std::shared_ptr<compositor::Compositor> const compositor;
    std::shared_ptr<graphics::DisplayConfigurationPolicy> const display_configuration_policy;
    std::shared_ptr<SessionContainer> const session_container;
    std::shared_ptr<SessionEventHandlerRegister> const session_event_handler_register;
    std::shared_ptr<ServerActionQueue> const server_action_queue;
    std::shared_ptr<graphics::DisplayConfigurationObserver> const observer;
    std::mutex configuration_mutex;
    std::map<std::weak_ptr<scene::Session>,
             std::shared_ptr<graphics::DisplayConfiguration>,
             std::owner_less<std::weak_ptr<scene::Session>>> config_map;
    std::weak_ptr<scene::Session> focused_session;
    std::shared_ptr<graphics::DisplayConfiguration> base_configuration_;
    bool base_configuration_applied;
    std::shared_ptr<time::AlarmFactory> const alarm_factory;
    struct SessionObserver;
    std::unique_ptr<SessionObserver> const session_observer;

    /// Mutex protecting pending_configuration
    std::mutex pending_configuration_mutex;
    /// See configure(). The config that will be applied by a currently in-flight server action.
    /// If null, there is no config to apply or hardware config server action queued.
    std::shared_ptr<graphics::DisplayConfiguration> pending_configuration;

    /// Mutex protecting power_mode
    std::mutex power_mode_mutex;
    /// Power mode outputs have been set to
    MirPowerMode power_mode{mir_power_mode_on};
};

}
}

#endif /* MIR_SCENE_MEDIATING_DISPLAY_CHANGER_H_ */
