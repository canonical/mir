/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir/graphics/display.h"
#include "mir/graphics/buffer.h"
#include "mir/frontend/session_authorizer.h"
#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/frontend/protobuf_ipc_factory.h"
#include "mir/frontend/resource_cache.h"
#include "mir/frontend/session_mediator.h"
#include "mir/frontend/global_event_sender.h"

#include "mir_test_framework/display_server_test_fixture.h"
#include "mir_test_framework/cross_process_sync.h"
#include "mir_test_doubles/stub_buffer.h"
#include "mir_test_doubles/null_display.h"
#include "mir_test_doubles/null_event_sink.h"
#include "mir_test_doubles/null_display_config.h"
#include "mir_test_doubles/null_display_changer.h"
#include "mir_test_doubles/stub_display_buffer.h"
#include "mir_test_doubles/null_platform.h"
#include "mir_test/display_config_matchers.h"
#include "mir_test_doubles/stub_display_configuration.h"
#include "mir_test/fake_shared.h"

#include "mir_toolkit/mir_client_library.h"

#include <condition_variable>
#include <thread>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace msh = mir::shell;
namespace mg = mir::graphics;
namespace mc = mir::compositor;
namespace geom = mir::geometry;
namespace mf = mir::frontend;
namespace mtf = mir_test_framework;
namespace mtd = mir::test::doubles;
namespace mt = mir::test;

namespace
{

class StubDisplay : public mtd::NullDisplay
{
public:
    StubDisplay()
    {
    }

    void for_each_display_buffer(std::function<void(mg::DisplayBuffer&)> const& f) override
    {
        f(display_buffer);
    }

private:
    mtd::NullDisplayBuffer display_buffer;
};

class StubChanger : public mtd::NullDisplayChanger
{
public:
    std::shared_ptr<mg::DisplayConfiguration> active_configuration() override
    {
        return mt::fake_shared(stub_display_config);
    }

    static mtd::StubDisplayConfig stub_display_config;
private:
    mtd::NullDisplayBuffer display_buffer;
};

mtd::StubDisplayConfig StubChanger::stub_display_config;

char const* const mir_test_socket = mtf::test_socket_file().c_str();

class StubGraphicBufferAllocator : public mg::GraphicBufferAllocator
{
public:
    std::shared_ptr<mg::Buffer> alloc_buffer(mg::BufferProperties const&)
    {
        return std::shared_ptr<mg::Buffer>(new mtd::StubBuffer());
    }

    std::vector<geom::PixelFormat> supported_pixel_formats()
    {
        return pixel_formats;
    }

    static std::vector<geom::PixelFormat> const pixel_formats;
};

std::vector<geom::PixelFormat> const StubGraphicBufferAllocator::pixel_formats{
    geom::PixelFormat::bgr_888,
    geom::PixelFormat::abgr_8888,
    geom::PixelFormat::xbgr_8888
};

class StubPlatform : public mtd::NullPlatform
{
public:
    std::shared_ptr<mg::GraphicBufferAllocator> create_buffer_allocator(
            std::shared_ptr<mg::BufferInitializer> const& /*buffer_initializer*/) override
    {
        return std::make_shared<StubGraphicBufferAllocator>();
    }

    std::shared_ptr<mg::Display> create_display(
        std::shared_ptr<mg::DisplayConfigurationPolicy> const&) override
    {
        if (!display)
            display = std::make_shared<StubDisplay>();
        return display;
    }

    std::shared_ptr<StubDisplay> display;
};

class StubAuthorizer : public mf::SessionAuthorizer
{
    bool connection_is_allowed(pid_t)
    {
        return true;
    }

    bool configure_display_is_allowed(pid_t)
    {
        return false;
    }
};

}

TEST_F(BespokeDisplayServerTestFixture, display_info_reaches_client)
{
    struct ServerConfig : TestingServerConfiguration
    {
        std::shared_ptr<mg::Platform> the_graphics_platform()
        {
            if (!platform)
                platform = std::make_shared<StubPlatform>();

            return platform;
        }

        std::shared_ptr<msh::DisplayChanger> the_shell_display_changer()
        {
            if (!changer)
                changer = std::make_shared<StubChanger>();
            return changer; 
        }

        std::shared_ptr<StubChanger> changer;
        std::shared_ptr<StubPlatform> platform;
    } server_config;

    launch_server_process(server_config);

    struct Client : TestingClientConfiguration
    {
        void exec()
        {
            auto connection = mir_connect_sync(mir_test_socket, __PRETTY_FUNCTION__);
            auto configuration = mir_connection_create_display_config(connection);

            EXPECT_THAT(configuration, mt::ClientTypeConfigMatches(StubChanger::stub_display_config.outputs));

            mir_display_config_destroy(configuration);
            mir_connection_release(connection);
        }
    } client_config;

    launch_client_process(client_config);
}

TEST_F(BespokeDisplayServerTestFixture, display_change_request_for_unauthorized_client)
{
    struct ServerConfig : TestingServerConfiguration
    {
        std::shared_ptr<mg::Platform> the_graphics_platform()
        {
            if (!platform)
                platform = std::make_shared<StubPlatform>();
            return platform;
        }

        std::shared_ptr<mf::SessionAuthorizer> the_session_authorizer()
        {
            if (!authorizer)
            {
                authorizer = std::make_shared<StubAuthorizer>();
            }
            return authorizer;
        }

        std::shared_ptr<msh::DisplayChanger> the_shell_display_changer()
        {
            if (!changer)
                changer = std::make_shared<StubChanger>();
            return changer; 
        }

        std::shared_ptr<StubChanger> changer;
        std::shared_ptr<StubAuthorizer> authorizer;
        std::shared_ptr<StubPlatform> platform;
    } server_config;

    launch_server_process(server_config);
    
    struct Client : TestingClientConfiguration
    {
        void exec()
        {
            auto connection = mir_connect_sync(mir_test_socket, __PRETTY_FUNCTION__);
            auto configuration = mir_connection_create_display_config(connection);

            mir_wait_for(mir_connection_apply_display_config(connection, configuration));
            EXPECT_THAT(mir_connection_get_error_message(connection),
                        testing::HasSubstr("not authorized to apply display configurations"));
 
            mir_display_config_destroy(configuration);
            mir_connection_release(connection);
        }
    } client_config;

    launch_client_process(client_config);
}

/* TODO: this test currently checks the same format list against both the surface formats
         and display formats. Improve test to return different format lists for both concepts */ 
TEST_F(BespokeDisplayServerTestFixture, display_surface_pfs_reaches_client)
{
    struct ServerConfig : TestingServerConfiguration
    {
        std::shared_ptr<mg::Platform> the_graphics_platform()
        {
            using namespace testing;

            if (!platform)
                platform = std::make_shared<StubPlatform>();

            return platform;
        }

        std::shared_ptr<msh::DisplayChanger> the_shell_display_changer()
        {
            if (!changer)
                changer = std::make_shared<StubChanger>();
            return changer; 
        }

        std::shared_ptr<StubChanger> changer;
        std::shared_ptr<StubPlatform> platform;
    } server_config;

    launch_server_process(server_config);

    struct Client : TestingClientConfiguration
    {
        void exec()
        {
            MirConnection* connection = mir_connect_sync(mir_test_socket, __PRETTY_FUNCTION__);

            unsigned int const format_storage_size = 4;
            MirPixelFormat formats[format_storage_size]; 
            unsigned int returned_format_size = 0;
            mir_connection_get_available_surface_formats(connection,
                formats, format_storage_size, &returned_format_size);

            ASSERT_EQ(returned_format_size, StubGraphicBufferAllocator::pixel_formats.size());
            for (auto i=0u; i < returned_format_size; ++i)
            {
                EXPECT_EQ(StubGraphicBufferAllocator::pixel_formats[i],
                          static_cast<geom::PixelFormat>(formats[i]));
            }

            mir_connection_release(connection);
        }
    } client_config;

    launch_client_process(client_config);
}


namespace
{
class EventSinkSkimmingIpcFactory : public mf::ProtobufIpcFactory
{
public:
    EventSinkSkimmingIpcFactory(
        std::shared_ptr<mf::Shell> const& shell,
        std::shared_ptr<mf::SessionMediatorReport> const& sm_report,
        std::shared_ptr<mf::MessageProcessorReport> const& mr_report,
        std::shared_ptr<mg::Platform> const& graphics_platform,
        std::shared_ptr<msh::DisplayChanger> const& graphics_changer,
        std::shared_ptr<mg::GraphicBufferAllocator> const& buffer_allocator)
        : shell(shell),
          sm_report(sm_report),
          mp_report(mr_report),
          cache(std::make_shared<mf::ResourceCache>()),
          graphics_platform(graphics_platform),
          graphics_changer(graphics_changer),
          buffer_allocator(buffer_allocator),
          last_event_sink(std::make_shared<mtd::NullEventSink>())
    {
    }

    std::shared_ptr<mf::EventSink> last_clients_event_sink()
    {
        return std::move(last_event_sink);
    }

private:
    std::shared_ptr<mf::Shell> shell;
    std::shared_ptr<mf::SessionMediatorReport> const sm_report;
    std::shared_ptr<mf::MessageProcessorReport> const mp_report;
    std::shared_ptr<mf::ResourceCache> const cache;
    std::shared_ptr<mg::Platform> const graphics_platform;
    std::shared_ptr<msh::DisplayChanger> const graphics_changer;
    std::shared_ptr<mg::GraphicBufferAllocator> const buffer_allocator;
    std::shared_ptr<mf::EventSink> last_event_sink;

    virtual std::shared_ptr<mir::protobuf::DisplayServer> make_ipc_server(
        std::shared_ptr<mf::EventSink> const& sink, bool)
    {
        last_event_sink = sink;
        return std::make_shared<mf::SessionMediator>(
            shell,
            graphics_platform,
            graphics_changer,
            buffer_allocator,
            sm_report,
            sink,
            resource_cache());
    }

    virtual std::shared_ptr<mf::ResourceCache> resource_cache()
    {
        return cache;
    }

    virtual std::shared_ptr<mf::MessageProcessorReport> report()
    {
        return mp_report;
    }

};

mtd::StubDisplayConfig changed_stub_display_config{1};
}

TEST_F(BespokeDisplayServerTestFixture, display_change_notification_reaches_all_clients)
{
    mtf::CrossProcessSync client_ready_fence;
    mtf::CrossProcessSync unsubscribed_client_ready_fence;
    mtf::CrossProcessSync unsubscribed_check_fence;
    mtf::CrossProcessSync send_event_fence;
    mtf::CrossProcessSync events_all_sent;

    struct ServerConfig : TestingServerConfiguration
    {
        ServerConfig(mtf::CrossProcessSync const& send_event_fence,
                     mtf::CrossProcessSync const& events_sent)
            : send_event_fence(send_event_fence),
              events_sent(events_sent)
        {
        }

        std::shared_ptr<mg::Platform> the_graphics_platform() override
        {
            if (!platform)
                platform = std::make_shared<StubPlatform>();
            return platform;
        }

        void exec() override
        {
            change_thread = std::move(std::thread([this](){
                auto global_sender = the_global_event_sink();

                send_event_fence.wait_for_signal_ready_for(std::chrono::milliseconds(1000));
                global_sender->handle_display_config_change(changed_stub_display_config);
                events_sent.try_signal_ready_for(std::chrono::milliseconds(1000));
            })); 
        }

        void on_exit() override
        {
            change_thread.join();
        }

        mtf::CrossProcessSync send_event_fence;
        mtf::CrossProcessSync events_sent;
        std::shared_ptr<StubPlatform> platform;
        std::thread change_thread;
    } server_config(send_event_fence, events_all_sent);

    struct SubscribedClient : TestingClientConfiguration
    {
        SubscribedClient(mtf::CrossProcessSync const& client_ready_fence)
         : client_ready_fence(client_ready_fence)
        {
        }

        static void change_handler(MirConnection* connection, void* context)
        {
            auto configuration = mir_connection_create_display_config(connection);

            EXPECT_THAT(configuration, mt::ClientTypeConfigMatches(changed_stub_display_config.outputs));
            mir_display_config_destroy(configuration);

            auto client_config = static_cast<SubscribedClient*>(context); 
            client_config->notify_callback();
        }

        void notify_callback()
        {
            callback_called = true;
            callback_wait.notify_all();
        }

        void exec()
        {
            std::unique_lock<std::mutex> lk(mut);
            MirConnection* connection = mir_connect_sync(mir_test_socket, "notifier");
            callback_called = false;
        
            mir_connection_set_display_config_change_callback(connection, &change_handler, this);

            client_ready_fence.try_signal_ready_for(std::chrono::milliseconds(1000));

            while(!callback_called)
            {
                callback_wait.wait(lk);
            } 

            mir_connection_release(connection);
        }

        mtf::CrossProcessSync client_ready_fence;

        std::mutex mut;
        std::condition_variable callback_wait;
        bool callback_called;
    } client_config(client_ready_fence);

    struct UnsubscribedClient : TestingClientConfiguration
    {
        UnsubscribedClient(mtf::CrossProcessSync const& client_ready_fence,
                           mtf::CrossProcessSync const& client_check_fence)
         : client_ready_fence(client_ready_fence),
           client_check_fence(client_check_fence)
        {
        }

        void exec()
        {
            MirConnection* connection = mir_connect_sync(mir_test_socket, "notifier");
        
            client_ready_fence.try_signal_ready_for(std::chrono::milliseconds(1000));

            //wait for display change signal sent
            client_check_fence.wait_for_signal_ready_for(std::chrono::milliseconds(1000));

            //at this point, the message has gone out on the wire. since we're emulating a client
            //that is passively subscribed, we will just wait for the display configuration to change
            //and then will check the new config. 
            int const fail_limit = 100;
            int fail_count = 0;
            auto configuration = mir_connection_create_display_config(connection);
            while( (configuration->num_displays != changed_stub_display_config.outputs.size()) 
                    && (fail_count < fail_limit))
            {
                fail_count++;
                mir_display_config_destroy(configuration);
                std::this_thread::sleep_for(std::chrono::microseconds(500));
                configuration = mir_connection_create_display_config(connection);
            }

            EXPECT_LT(fail_count, fail_limit);
            EXPECT_THAT(configuration, mt::ClientTypeConfigMatches(changed_stub_display_config.outputs));
            mir_display_config_destroy(configuration);

            mir_connection_release(connection);
        }

        mtf::CrossProcessSync client_ready_fence;
        mtf::CrossProcessSync client_check_fence;
    } unsubscribed_client_config(unsubscribed_client_ready_fence, unsubscribed_check_fence);

    launch_server_process(server_config);
    launch_client_process(client_config);
    launch_client_process(unsubscribed_client_config);

    run_in_test_process([&]
    {
        client_ready_fence.wait_for_signal_ready_for(std::chrono::milliseconds(1000));
        unsubscribed_client_ready_fence.wait_for_signal_ready_for(std::chrono::milliseconds(1000));

        send_event_fence.try_signal_ready_for(std::chrono::milliseconds(1000));
        events_all_sent.wait_for_signal_ready_for(std::chrono::milliseconds(1000));

        unsubscribed_check_fence.try_signal_ready_for(std::chrono::milliseconds(1000));
    });
}
