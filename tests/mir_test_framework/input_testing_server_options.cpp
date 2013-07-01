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
#include "mir/graphics/display.h"
#include "mir/graphics/viewable_area.h"

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
    
    virtual void surface_ready_for_input(std::string const& surface_name) = 0;
    virtual void surface_finished_for_input(std::string const& surface_name) = 0;

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
    
    void input_surface_opened(std::shared_ptr<mi::InputChannel> const& opened_surface)
    {
        underlying_registrar->input_surface_opened(opened_surface);
        listener->surface_ready_for_input(opened_surface->name());
    }
    void input_surface_closed(std::shared_ptr<mi::InputChannel> const& closed_surface)
    {
        underlying_registrar->input_surface_closed(closed_surface);
        listener->surface_finished_for_input(closed_surface->name());
    }

private:
    std::shared_ptr<ms::InputRegistrar> const underlying_registrar;
    std::shared_ptr<SurfaceReadinessListener> const listener;
};

struct SizedViewArea : public mg::ViewableArea
{
    SizedViewArea(geom::Rectangle const& area)
        : area(area)
    {
    }
    geom::Rectangle view_area() const override
    {
        return area;
    }
    geom::Rectangle area;
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

        input_configuration = std::make_shared<mtd::FakeEventHubInputConfiguration>(the_event_filters(),
            the_display(),
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
        void surface_ready_for_input(std::string const& surface_name)
        {
            std::unique_lock<std::mutex> lg(lifecycle_lock);
            client_lifecycles[surface_name] = mtf::ClientLifecycleState::appeared;
            lifecycle_condition.notify_all();
        }

        void surface_finished_for_input(std::string const& surface_name)
        {
            std::unique_lock<std::mutex> lg(lifecycle_lock);
            client_lifecycles[surface_name] = mtf::ClientLifecycleState::vanished;
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

geom::Rectangle mtf::InputTestingServerConfiguration::the_screen_geometry()
{
    static geom::Rectangle const default_geometry{geom::Point{geom::X{0}, geom::Y{0}},
        geom::Size{geom::Width{1600}, geom::Height{1600}}};
    return default_geometry;
}

std::shared_ptr<mg::ViewableArea> mtf::InputTestingServerConfiguration::the_viewable_area()
{
    if (!view_area)
        view_area = std::make_shared<SizedViewArea>(the_screen_geometry());
    return view_area;
}

void mtf::InputTestingServerConfiguration::wait_until_client_appears(std::string const& surface_name)
{
    std::unique_lock<std::mutex> lg(lifecycle_lock);
    
    std::chrono::seconds timeout(60);
    auto end_time = std::chrono::system_clock::now() + timeout;
    
    if (client_lifecycles[surface_name] == vanished)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("Waiting for a client (" + surface_name + ") to appear but it has already vanished"));
    }
    while (client_lifecycles[surface_name] != appeared)
    {
        if (lifecycle_condition.wait_until(lg, end_time) == std::cv_status::timeout)
            BOOST_THROW_EXCEPTION(std::runtime_error("Timed out waiting for client (" + surface_name + ") to appear"));
    }
}

void mtf::InputTestingServerConfiguration::wait_until_client_vanishes(std::string const& surface_name)
{
    std::unique_lock<std::mutex> lg(lifecycle_lock);

    std::chrono::seconds timeout(60);
    auto end_time = std::chrono::system_clock::now() + timeout;
    

    if (client_lifecycles[surface_name] == appeared)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("Waiting for a client (" + surface_name + ") to vanish but it has already appeared"));
    }
    while (client_lifecycles[surface_name] != vanished)
    {
        if (lifecycle_condition.wait_until(lg, end_time) == std::cv_status::timeout)
            BOOST_THROW_EXCEPTION(std::runtime_error("Timed out waiting for client (" + surface_name + ") to vanish"));
    }
}
