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
#include "mir/input/surface_info.h"
#include "mir/graphics/display.h"
#include "mir/graphics/viewable_area.h"
#include "mir/shell/surface_creation_parameters.h"
#include "mir/frontend/shell.h"

#include "mir_test/fake_event_hub.h"
#include "mir_test/fake_event_hub_input_configuration.h"

#include <boost/throw_exception.hpp>

#include <functional>
#include <stdexcept>

namespace mtf = mir_test_framework;

namespace ms = mir::surfaces;
namespace msh = mir::shell;
namespace mf = mir::frontend;
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

protected:
    SurfaceReadinessListener() = default;
    SurfaceReadinessListener(SurfaceReadinessListener const&) = delete;
    SurfaceReadinessListener& operator=(SurfaceReadinessListener const&) = delete;
};

class ProxyShell : public mf::Shell
{
public:
    ProxyShell(std::shared_ptr<mf::Shell> const& underlying_shell,
               std::shared_ptr<SurfaceReadinessListener> const listener)
        : underlying_shell(underlying_shell),
          listener(listener)
    {   
    }

    ~ProxyShell() noexcept(true) = default;
    
    mf::SurfaceId create_surface_for(std::shared_ptr<mf::Session> const& session,
        msh::SurfaceCreationParameters const& params)
    {
        auto id = underlying_shell->create_surface_for(session, params);
        listener->channel_ready_for_input(params.name);
        return id;
    }

    std::shared_ptr<mf::Session> open_session(std::string const& name, 
                                              std::shared_ptr<mir::events::EventSink> const& sink)
    {
        return underlying_shell->open_session(name, sink);
    }
    void close_session(std::shared_ptr<mf::Session> const& session)
    {
        underlying_shell->close_session(session);
    }

private:
    std::shared_ptr<mf::Shell> const underlying_shell;
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

std::shared_ptr<mf::Shell> mtf::InputTestingServerConfiguration::the_frontend_shell()
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

        std::mutex &lifecycle_lock;
        std::condition_variable &lifecycle_condition;
        std::map<std::string, mtf::ClientLifecycleState> &client_lifecycles;
    };
    
    if (!frontend_shell)
    {
        auto readiness_listener = std::make_shared<LifecycleTracker>(lifecycle_lock,
            lifecycle_condition,
            client_lifecycles);
        frontend_shell = std::make_shared<ProxyShell>(DefaultServerConfiguration::the_frontend_shell(), readiness_listener);
    }

    return frontend_shell;
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
