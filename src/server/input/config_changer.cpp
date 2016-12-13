/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "config_changer.h"

#include "mir/scene/session_event_handler_register.h"
#include "mir/scene/session_container.h"
#include "mir/scene/session.h"
#include "mir/input/device.h"
#include "mir/input/input_device_hub.h"
#include "mir/input/input_device_observer.h"
#include "mir/input/input_configuration.h"
#include "mir/input/pointer_configuration.h"
#include "mir/input/touchpad_configuration.h"
#include "mir/input/keyboard_configuration.h"
#include "mir/client_visible_error.h"

#include "mir_toolkit/client_types.h"

namespace mi = mir::input;
namespace mf = mir::frontend;
namespace ms = mir::scene;

namespace
{

class InputConfigurationFailedError : public mir::ClientVisibleError
{
public:
    InputConfigurationFailedError(std::exception const& exception)
        : ClientVisibleError(std::string{"Input configuration failed: "} + exception.what())
    {
    }

    MirErrorDomain domain() const noexcept override
    {
        return mir_error_domain_input_configuration;
    }

    uint32_t code() const noexcept override
    {
        return mir_input_configuration_error_rejected_by_driver;
    }
};

void apply_device_config(mi::DeviceConfiguration const* config, mi::Device& device)
{
    if (!config)
        return;

    auto existing_ptr_conf = device.pointer_configuration();
    if (config->has_pointer_configuration() && existing_ptr_conf.is_set())
        device.apply_pointer_configuration(config->pointer_configuration());

    auto existing_tpd_conf = device.touchpad_configuration();
    if (config->has_touchpad_configuration() && existing_tpd_conf.is_set())
        device.apply_touchpad_configuration(config->touchpad_configuration());

    auto existing_kbd_conf = device.keyboard_configuration();
    if (config->has_keyboard_configuration() && existing_kbd_conf.is_set())
        device.apply_keyboard_configuration(config->keyboard_configuration());
}

mi::DeviceConfiguration get_device_config(mi::Device const& device)
{
    mi::DeviceConfiguration cfg(device.id(), device.capabilities(), device.name(), device.unique_id());
    auto ptr_conf = device.pointer_configuration();
    if (ptr_conf.is_set())
        cfg.set_pointer_configuration(ptr_conf.value());

    auto tpd_conf = device.touchpad_configuration();
    if (tpd_conf.is_set())
        cfg.set_touchpad_configuration(tpd_conf.value());

    auto kbd_conf = device.keyboard_configuration();
    if (kbd_conf.is_set())
        cfg.set_keyboard_configuration(kbd_conf.value());

    return cfg;
}

struct DeviceChangeTracker : mi::InputDeviceObserver
{
    DeviceChangeTracker(mi::ConfigChanger& changer)
        : config_changer(changer)
    {
    }

    void device_added(std::shared_ptr<mi::Device> const& device) override
    {
        added.push_back(device);
    }
    void device_changed(std::shared_ptr<mi::Device> const&) override
    {
    }

    void device_removed(std::shared_ptr<mi::Device> const& device) override
    {
        removed.push_back(device->id());
    }

    void changes_complete() override
    {
        if (!added.empty() || !removed.empty())
            config_changer.devices_updated(added, removed);

        added.clear();
        removed.clear();
    }

    mi::ConfigChanger& config_changer;
    std::vector<std::shared_ptr<mi::Device>> added;
    std::vector<MirInputDeviceId> removed;
};

}

mi::ConfigChanger::ConfigChanger(
    std::shared_ptr<InputManager> const& manager,
    std::shared_ptr<InputDeviceHub> const& devices,
    std::shared_ptr<scene::SessionContainer> const& session_container,
    std::shared_ptr<scene::SessionEventHandlerRegister> const& session_event_handler_register)
    : input_manager{manager},
      devices{devices},
      session_container{session_container},
      session_event_handler_register{session_event_handler_register},
      device_observer(std::make_shared<DeviceChangeTracker>(*this)),
      base_configuration_applied(true)
{
    devices->add_observer(device_observer);

    session_event_handler_register->register_focus_change_handler(
        [this](std::shared_ptr<ms::Session> const& session)
        {
            focus_change_handler(session);
        });

    session_event_handler_register->register_no_focus_handler(
        [this]
        {
            no_focus_handler();
        });

    session_event_handler_register->register_session_stopping_handler(
        [this](std::shared_ptr<ms::Session> const& session)
        {
            session_stopping_handler(session);
        });
}

mi::ConfigChanger::~ConfigChanger()
{
    devices->remove_observer(device_observer);
}

mi::InputConfiguration mi::ConfigChanger::base_configuration()
{
    std::lock_guard<std::mutex> lg{config_mutex};
    return base;
}

void mi::ConfigChanger::configure(std::shared_ptr<frontend::Session> const& session, InputConfiguration && config)
{
    std::lock_guard<std::mutex> lg{config_mutex};
    auto const& session_config = (config_map[session] = std::move(config));

    if (session != focused_session.lock())
        return;

    devices->for_each_mutable_input_device(
        [&session_config](Device& device)
        {
            auto device_config = session_config.get_device_configuration_by_id(device.id());
            apply_device_config(device_config, device);
        });
}

void mi::ConfigChanger::set_base_configuration(InputConfiguration && config)
{
    std::lock_guard<std::mutex> lg{config_mutex};
    base = std::move(config);
}

void mi::ConfigChanger::devices_updated(std::vector<std::shared_ptr<Device>> const& added, std::vector<MirInputDeviceId> const& removed)
{
    std::lock_guard<std::mutex> lg{config_mutex};
    for (auto const id : removed)
    {
        base.remove_device_by_id(id);
        for(auto & session_config : config_map)
            session_config.second.remove_device_by_id(id);
    }

    for (auto const dev : added)
    {
        base.add_device_configuration(get_device_config(*dev));
    }

    session_container->for_each(
        [this](std::shared_ptr<ms::Session> const& session)
        {
            session->send_input_config(base);
        });
}

void mi::ConfigChanger::focus_change_handler(std::shared_ptr<ms::Session> const& session)
{
    std::lock_guard<std::mutex> lg{config_mutex};

    focused_session = session;

    auto const it = config_map.find(session);
    if (it != end(config_map))
    {
        try
        {
            apply_config(it->second);
        }
        catch (std::invalid_argument const& invalid)
        {
            session->send_error(InputConfigurationFailedError{invalid});
        }
    }
    else if (!base_configuration_applied)
    {
        apply_base_config();
    }

}

void mi::ConfigChanger::no_focus_handler()
{
    std::lock_guard<std::mutex> lg{config_mutex};
    focused_session.reset();
    if (!base_configuration_applied)
    {
        apply_base_config();
    }
}

void mi::ConfigChanger::session_stopping_handler(std::shared_ptr<ms::Session> const& session)
{
    std::lock_guard<std::mutex> lg{config_mutex};

    config_map.erase(session);
}

void mi::ConfigChanger::apply_config(mi::InputConfiguration const& config)
{
    devices->for_each_mutable_input_device(
        [&config](Device& device)
        {
            auto device_config = config.get_device_configuration_by_id(device.id());
            if (device_config)
               apply_device_config(device_config, device);
        });
    base_configuration_applied = false;
}

void mi::ConfigChanger::apply_base_config()
{
    apply_config(base);
    base_configuration_applied = true;
}
