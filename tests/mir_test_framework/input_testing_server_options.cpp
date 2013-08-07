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

#include "mir_test_framework/input_testing_server_configuration.h"

#include "mir/input/input_channel.h"
#include "mir/surfaces/input_registrar.h"
#include "mir/input/surface.h"
#include "mir/input/composite_event_filter.h"

#include "mir_test/fake_event_hub.h"
#include "mir_test/fake_event_hub_input_configuration.h"

#include <boost/throw_exception.hpp>

#include <functional>
#include <stdexcept>

namespace mtf = mir_test_framework;

namespace ms = mir::surfaces;
namespace mg = mir::graphics;
namespace mi = mir::input;
namespace mia = mi::android;
namespace geom = mir::geometry;
namespace mtd = mir::test::doubles;

namespace
{
class SurfaceReadinessListener
{
public:
    virtual ~SurfaceReadinessListener() = default;
    
    virtual void channel_ready_for_input(std::string const& channel_name) = 0;
    virtual void channel_finished_for_input(std::string const& channel_name) = 0;

protected:
    SurfaceReadinessListener() = default;
    SurfaceReadinessListener(SurfaceReadinessListener const&) = delete;
    SurfaceReadinessListener& operator=(SurfaceReadinessListener const&) = delete;
};

class ProxyInputRegistrar : public ms::InputRegistrar
{
public:
    ProxyInputRegistrar(std::shared_ptr<ms::InputRegistrar> const underlying_registrar,
                        std::shared_ptr<SurfaceReadinessListener> const listener)
        : underlying_registrar(underlying_registrar),
          listener(listener)
    {   
    }

    ~ProxyInputRegistrar() noexcept(true) = default;
    
    void input_channel_opened(std::shared_ptr<mi::InputChannel> const& opened_channel,
                              std::shared_ptr<mi::Surface> const& surface,
                              mi::InputReceptionMode input_mode)
    {
        outstanding_channels[opened_channel] = surface->name();
        underlying_registrar->input_channel_opened(opened_channel, surface, input_mode);
        listener->channel_ready_for_input(surface->name());
    }
    void input_channel_closed(std::shared_ptr<mi::InputChannel> const& closed_channel)
    {
        auto channel = outstanding_channels[closed_channel];
        underlying_registrar->input_channel_closed(closed_channel);
        listener->channel_finished_for_input(channel);
    }

private:
    std::map<std::shared_ptr<mi::InputChannel>, std::string> outstanding_channels;
    std::shared_ptr<ms::InputRegistrar> const underlying_registrar;
    std::shared_ptr<SurfaceReadinessListener> const listener;
};

}

mtf::InputTestingServerConfiguration::InputTestingServerConfiguration()
{
}

void mtf::InputTestingServerConfiguration::exec()
{
    input_injection_thread = std::thread(std::mem_fn(&mtf::InputTestingServerConfiguration::inject_input), this);
}

void mtf::InputTestingServerConfiguration::on_exit()
{
    input_injection_thread.join();
}

std::shared_ptr<mi::InputConfiguration> mtf::InputTestingServerConfiguration::the_input_configuration()
{
    if (!input_configuration)
    {
        std::shared_ptr<mi::CursorListener> null_cursor_listener{nullptr};

        input_configuration = std::make_shared<mtd::FakeEventHubInputConfiguration>(
            the_composite_event_filter(),
            the_input_region(),
            null_cursor_listener,
            the_input_report());
        fake_event_hub = input_configuration->the_fake_event_hub();

        fake_event_hub->synthesize_builtin_keyboard_added();
        fake_event_hub->synthesize_builtin_cursor_added();
        fake_event_hub->synthesize_device_scan_complete();
    }
    
    return input_configuration;
}

std::shared_ptr<ms::InputRegistrar> mtf::InputTestingServerConfiguration::the_input_registrar()
{
    struct LifecycleTracker : public SurfaceReadinessListener
    {
        LifecycleTracker(std::mutex& lifecycle_lock,
                          std::condition_variable &lifecycle_condition,
                          std::map<std::string, mtf::ClientLifecycleState> &client_lifecycles)
            : lifecycle_lock(lifecycle_lock),
              lifecycle_condition(lifecycle_condition),
              client_lifecycles(client_lifecycles)
        {
        }
        void channel_ready_for_input(std::string const& channel_name)
        {
            std::unique_lock<std::mutex> lg(lifecycle_lock);
            client_lifecycles[channel_name] = mtf::ClientLifecycleState::appeared;
            lifecycle_condition.notify_all();
        }

        void channel_finished_for_input(std::string const& channel_name)
        {
            std::unique_lock<std::mutex> lg(lifecycle_lock);
            client_lifecycles[channel_name] = mtf::ClientLifecycleState::vanished;
            lifecycle_condition.notify_all();
        }
        std::mutex &lifecycle_lock;
        std::condition_variable &lifecycle_condition;
        std::map<std::string, mtf::ClientLifecycleState> &client_lifecycles;
    };
    
    if (!input_registrar)
    {
        auto registrar_listener = std::make_shared<LifecycleTracker>(lifecycle_lock,
            lifecycle_condition,
            client_lifecycles);
        input_registrar = std::make_shared<ProxyInputRegistrar>(the_input_configuration()->the_input_registrar(),
            registrar_listener);
    }

    return input_registrar;
}

void mtf::InputTestingServerConfiguration::wait_until_client_appears(std::string const& channel_name)
{
    std::unique_lock<std::mutex> lg(lifecycle_lock);
    
    std::chrono::seconds timeout(60);
    auto end_time = std::chrono::system_clock::now() + timeout;
    
    if (client_lifecycles[channel_name] == vanished)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("Waiting for a client (" + channel_name + ") to appear but it has already vanished"));
    }
    while (client_lifecycles[channel_name] != appeared)
    {
        if (lifecycle_condition.wait_until(lg, end_time) == std::cv_status::timeout)
            BOOST_THROW_EXCEPTION(std::runtime_error("Timed out waiting for client (" + channel_name + ") to appear"));
    }
}

void mtf::InputTestingServerConfiguration::wait_until_client_vanishes(std::string const& channel_name)
{
    std::unique_lock<std::mutex> lg(lifecycle_lock);

    std::chrono::seconds timeout(60);
    auto end_time = std::chrono::system_clock::now() + timeout;
    

    if (client_lifecycles[channel_name] == appeared)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("Waiting for a client (" + channel_name + ") to vanish but it has already appeared"));
    }
    while (client_lifecycles[channel_name] != vanished)
    {
        if (lifecycle_condition.wait_until(lg, end_time) == std::cv_status::timeout)
            BOOST_THROW_EXCEPTION(std::runtime_error("Timed out waiting for client (" + channel_name + ") to vanish"));
    }
}
