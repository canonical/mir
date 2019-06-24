/*
 * Copyright © 2018 Canonical Ltd.
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 *              Alan Griffiths <alan@octopull.co.uk>
 */

#include <fcntl.h>
#include <unordered_map>
#include <deque>

#include <wayland-client.h>
#include <wayland-server.h>
#include <mir/scene/surface_creation_parameters.h>
#include <mir/input/composite_event_filter.h>
#include <mir/input/device.h>
#include <mir/input/input_device_hub.h>
#include <mir/input/input_device_observer.h>
#include <mir/input/cursor_listener.h>
#include <mir/input/seat_observer.h>
#include <mir/observer_registrar.h>
#include <mir/executor.h>
#include <atomic>
#include <sys/eventfd.h>
#include <mir/log.h>
#include <wlcs/display_server.h>
#include <wlcs/pointer.h>
#include <wlcs/touch.h>

#include "mir/fd.h"

#include "mir/server.h"
#include "mir/options/option.h"
#include "test_server.h"

#include "mir_test_framework/executable_path.h"
#include "mir_test_framework/stub_server_platform_factory.h"
#include "mir_test_framework/fake_input_device.h"
#include "mir/input/device_capability.h"
#include "mir/input/input_device_info.h"
#include "mir/test/doubles/mock_gl.h"
#include "mir/frontend/session.h"
#include "mir/scene/session_listener.h"
#include "mir/scene/surface.h"
#include "mir/scene/session.h"
#include "mir/test/signal.h"

namespace mtf = mir_test_framework;
namespace mi = mir::input;
namespace mf = mir::frontend;
using namespace std::chrono_literals;

namespace std
{
// std::chrono::nanoseconds doesn't have a standard hash<> implementation?!
template<>
struct hash<::std::chrono::nanoseconds>
{
    typedef std::chrono::nanoseconds argument_type;
    typedef size_t result_type;
    result_type operator()(argument_type const& arg) const noexcept
    {
        /*
         * The underlying type of std::chrono::nanoseconds is guaranteed to be an
         * integer of at least 50-something bits, which will already have a perfectly
         * functional std::hash<> implementation…
         */
        return std::hash<decltype(arg.count())>{}(arg.count());
    }
};
}

namespace
{
/**
 * Smart-pointer-esque accessor for Mutex<> protected data.
 *
 * Ensures exclusive access to the referenced data.
 *
 * \tparam Guarded Type of data guarded by the mutex.
 */
template<typename Guarded>
class MutexGuard
{
public:
    MutexGuard(std::unique_lock<std::mutex>&& lock, Guarded& value)
        : value{value},
          lock{std::move(lock)}
    {
    }
    MutexGuard(MutexGuard&& from) = default;
    ~MutexGuard() noexcept(false)
    {
        if (lock.owns_lock())
        {
            lock.unlock();
        }
    }

    Guarded& operator*()
    {
        return value;
    }
    Guarded* operator->()
    {
        return &value;
    }
private:
    Guarded& value;
    std::unique_lock<std::mutex> lock;
};

/**
 * A data-locking mutex
 *
 * This is a mutex which owns the data it guards, and can give out a
 * smart-pointer-esque lock to lock and access it.
 *
 * \tparam Guarded  The type of data guarded by the mutex
 */
template<typename Guarded>
class Mutex
{
public:
    Mutex() = default;
    Mutex(Guarded&& initial_value)
        : value{std::move(initial_value)}
    {
    }

    Mutex(Mutex const&) = delete;
    Mutex& operator=(Mutex const&) = delete;

    /**
     * Lock the mutex and return an accessor for the protected data.
     *
     * \return A smart-pointer-esque accessor for the contained data.
     *          While code has access to the MutexGuard it is guaranteed to have exclusive
     *          access to the contained data.
     */
    MutexGuard<Guarded> lock()
    {
        return MutexGuard<Guarded>{std::unique_lock<std::mutex>{mutex}, value};
    }

protected:
    std::mutex mutex;
    Guarded value;
};

template<typename Guarded>
class WaitableMutex : public Mutex<Guarded>
{
public:
    using Mutex<Guarded>::Mutex;

    template<typename Predicate, typename Rep, typename Period>
    MutexGuard<Guarded> wait_for(Predicate predicate, std::chrono::duration<Rep, Period> timeout)
    {
        std::unique_lock<std::mutex> lock{this->mutex};
        if (!notifier.wait_for(lock, timeout, [this, &predicate]() { return predicate(this->value); }))
        {
            BOOST_THROW_EXCEPTION((std::runtime_error{"Notification timeout"}));
        }
        return MutexGuard<Guarded>{std::move(lock), this->value};
    }

    void notify_all()
    {
        notifier.notify_all();
    }
private:
    std::condition_variable notifier;
};


auto constexpr a_long_time = 5s;

using ClientFd = int;

class ResourceMapper : public mir::scene::SessionListener
{
public:
    ResourceMapper()
        : listeners{&this->state}
    {
    }

    void starting(std::shared_ptr<mir::scene::Session> const&) override
    {
    }

    void stopping(std::shared_ptr<mir::scene::Session> const&) override
    {
    }

    void focused(std::shared_ptr<mir::scene::Session> const&) override
    {
    }

    void unfocused() override
    {
    }

    void surface_created(
        mir::scene::Session&,
        std::shared_ptr<mir::scene::Surface> const& surface) override
    {
        auto state_accessor = state.lock();
        if (std::this_thread::get_id() == state_accessor->wayland_thread)
        {
            if (listeners.last_wl_window == nullptr)
            {
                BOOST_THROW_EXCEPTION((
                    std::runtime_error{
                        "Called Shell::create_surface() without first creating a wl_shell_surface?"}));
            }

            auto stream = surface->primary_buffer_stream();
            auto wl_surface = state_accessor->stream_map.at(stream);

            state_accessor->surface_map[wl_surface] = surface;
        }
    }

    void destroying_surface(
        mir::scene::Session&,
        std::shared_ptr<mir::scene::Surface> const&) override
    {
        // TODO: Maybe delete from map?
    }

    void buffer_stream_created(
        mir::scene::Session&,
        std::shared_ptr<mir::frontend::BufferStream> const& stream) override
    {
        auto state_accessor = state.lock();
        if (std::this_thread::get_id() == state_accessor->wayland_thread)
        {
            if (listeners.last_wl_surface == nullptr)
            {
                BOOST_THROW_EXCEPTION((
                    std::runtime_error{"BufferStream created without first constructing a wl_surface?"}));
            }

            state_accessor->stream_map[stream] = listeners.last_wl_surface;
            listeners.last_wl_surface = nullptr;
        }
    }

    void buffer_stream_destroyed(
        mir::scene::Session&,
        std::shared_ptr<mir::frontend::BufferStream> const& stream) override
    {
        state.lock()->stream_map.erase(stream);
    }

    void init(wl_display* display)
    {
        state.lock()->wayland_thread = std::this_thread::get_id();

        listeners.client_listener.notify = &client_created;

        wl_display_add_client_created_listener(display, &listeners.client_listener);
    }

    std::weak_ptr<mir::scene::Surface> surface_for_resource(wl_resource* surface)
    {
        if (strcmp(wl_resource_get_class(surface), "wl_surface") != 0)
        {
            BOOST_THROW_EXCEPTION((
                std::logic_error{
                    std::string{"Expected a wl_surface, got: "} +
                    wl_resource_get_class(surface)
                }));
        }

        auto state_accessor = state.lock();
        return state_accessor->surface_map.at(surface);
    }

    wl_client* client_for_fd(int client_socket)
    {
        return listeners.state->lock()->client_session_map.at(client_socket);
    }

    void associate_client_socket(int client_socket)
    {
        auto state_accessor = state.wait_for(
            [](State& state) { return static_cast<bool>(state.latest_client); },
            std::chrono::seconds{30});
        state_accessor->client_session_map[client_socket] = state_accessor->latest_client.value();
        state_accessor->latest_client = {};
    }

private:
    struct Listeners;
    struct ResourceListener
    {
        ResourceListener(Listeners* const listeners)
            : listeners{listeners}
        {
        }

        wl_listener resource_listener;
        Listeners* const listeners;
    };
    struct State
    {
        std::thread::id wayland_thread;
        std::unordered_map<wl_resource*, std::weak_ptr<mir::scene::Surface>> surface_map;
        std::unordered_map<std::shared_ptr<mir::frontend::BufferStream>, wl_resource*> stream_map;

        std::experimental::optional<wl_client*> latest_client;
        std::unordered_map<ClientFd, wl_client*> client_session_map;
        std::unordered_map<wl_client*, ResourceListener> resource_listener;
    };
    WaitableMutex<State> state;

    struct Listeners
    {
        Listeners(WaitableMutex<State>* const state)
            : state{state}
        {
        }

        wl_listener client_listener;

        wl_resource* last_wl_surface{nullptr};
        wl_resource* last_wl_window{nullptr};

        WaitableMutex<State>* const state;
    } listeners;

    static void resource_created(wl_listener* listener, void* ctx)
    {
        auto resource = static_cast<wl_resource*>(ctx);
        ResourceListener* resource_listener;
        resource_listener =
            wl_container_of(listener, resource_listener, resource_listener);

        bool const is_surface = strcmp(
            wl_resource_get_class(resource),
            "wl_surface") == 0;

        bool const is_window = strcmp(
                wl_resource_get_class(resource),
                "wl_shell_surface") == 0 ||
            strcmp(
                wl_resource_get_class(resource),
                "zxdg_surface_v6") == 0 ||
            strcmp(
                wl_resource_get_class(resource),
                "xdg_surface") == 0;

        if (is_surface)
        {
            resource_listener->listeners->last_wl_surface = resource;
        }
        else if (is_window)
        {
            resource_listener->listeners->last_wl_window = resource;
        }
    }

    static void client_created(wl_listener* listener, void* ctx)
    {
        auto client = static_cast<wl_client*>(ctx);
        Listeners* listeners;
        listeners =
            wl_container_of(listener, listeners, client_listener);

        wl_listener* resource_listener;
        {
            auto state_accessor = listeners->state->lock();
            state_accessor->latest_client = client;
            auto rl = state_accessor->resource_listener.emplace(client, listeners);
            rl.first->second.resource_listener.notify = &resource_created;
            resource_listener = &rl.first->second.resource_listener;
        }
        listeners->state->notify_all();

        wl_client_add_resource_created_listener(client, resource_listener);
    }
};

namespace
{
class WaylandExecutor : public mir::Executor
{
public:
    void spawn (std::function<void()>&& work) override
    {
        {
            std::lock_guard<std::recursive_mutex> lock{mutex};
            workqueue.emplace_back(std::move(work));
        }
        if (auto err = eventfd_write(notify_fd, 1))
        {
            BOOST_THROW_EXCEPTION((std::system_error{err, std::system_category(), "eventfd_write failed to notify event loop"}));
        }
    }

    /**
     * Get an Executor which dispatches onto a wl_event_loop
     *
     * \note    The executor may outlive the wl_event_loop, but no tasks will be dispatched
     *          after the wl_event_loop is destroyed.
     *
     * \param [in]  loop    The event loop to dispatch on
     * \return              An Executor that queues onto the wl_event_loop
     */
    static std::shared_ptr<mir::Executor> executor_for_event_loop(wl_event_loop* loop)
    {
        if (auto notifier = wl_event_loop_get_destroy_listener(loop, &on_display_destruction))
        {
            DestructionShim* shim;
            shim = wl_container_of(notifier, shim, destruction_listener);

            return shim->executor;
        }
        else
        {
            auto const executor = std::shared_ptr<WaylandExecutor>{new WaylandExecutor{loop}};
            auto shim = std::make_unique<DestructionShim>(executor);

            shim->destruction_listener.notify = &on_display_destruction;
            wl_event_loop_add_destroy_listener(loop, &(shim.release())->destruction_listener);

            return executor;
        }
    }

private:
    WaylandExecutor(wl_event_loop* loop)
        : notify_fd{eventfd(0, EFD_CLOEXEC | EFD_SEMAPHORE | EFD_NONBLOCK)},
          notify_source{wl_event_loop_add_fd(loop, notify_fd, WL_EVENT_READABLE, &on_notify, this)}
    {
        if (notify_fd == mir::Fd::invalid)
        {
            BOOST_THROW_EXCEPTION((std::system_error{
                errno,
                std::system_category(),
                "Failed to create IPC pause notification eventfd"}));
        }
    }

    std::function<void()> get_work()
    {
        std::lock_guard<std::recursive_mutex> lock{mutex};
        if (!workqueue.empty())
        {
            auto const work = std::move(workqueue.front());
            workqueue.pop_front();
            return work;
        }

        return {};
    }

    static int on_notify(int fd, uint32_t, void* data)
    {
        auto executor = static_cast<WaylandExecutor*>(data);

        eventfd_t unused;
        if (auto err = eventfd_read(fd, &unused))
        {
            mir::log(
                mir::logging::Severity::error,
                "wlcs-integration",
                "eventfd_read failed to consume wakeup notification: %s (%i)",
                strerror(err),
                err);
        }

        std::unique_lock<std::recursive_mutex> lock{executor->mutex};
        while (auto work = executor->get_work())
        {
            try
            {
                work();
            }
            catch(...)
            {
                mir::log(
                    mir::logging::Severity::critical,
                    "wlcs-integration",
                    std::current_exception(),
                    "Exception processing Wayland event loop work item");
            }
        }

        return 0;
    }

    static void on_display_destruction(wl_listener* listener, void*)
    {
        DestructionShim* shim;
        shim = wl_container_of(listener, shim, destruction_listener);

        {
            std::lock_guard<std::recursive_mutex> lock{shim->executor->mutex};
            wl_event_source_remove(shim->executor->notify_source);
        }
        delete shim;
    }

    std::recursive_mutex mutex;
    mir::Fd const notify_fd;
    std::deque<std::function<void()>> workqueue;

    wl_event_source* const notify_source;

    struct DestructionShim
    {
        explicit DestructionShim(std::shared_ptr<WaylandExecutor> const& executor)
            : executor{executor}
        {
        }

        std::shared_ptr<WaylandExecutor> const executor;
        wl_listener destruction_listener;
    };
    static_assert(
        std::is_standard_layout<DestructionShim>::value,
        "DestructionShim must be Standard Layout for wl_container_of to be defined behaviour");
};
}

class InputEventListener;

struct MirWlcsDisplayServer : miral::TestDisplayServer, public WlcsDisplayServer
{
    MirWlcsDisplayServer();

    auto build_window_manager_policy(miral::WindowManagerTools const& tools)
    -> std::unique_ptr<TestWindowManagerPolicy> override;

    testing::NiceMock<mir::test::doubles::MockGL> mockgl;
    std::shared_ptr<ResourceMapper> const resource_mapper{std::make_shared<ResourceMapper>()};
    std::shared_ptr<InputEventListener> event_listener;
    std::shared_ptr<mir::Executor> executor;
    std::atomic<double> cursor_x{0}, cursor_y{0};

    mir::Server* server = nullptr;
};

class InputEventListener : public mir::input::SeatObserver
{
public:
    InputEventListener(MirWlcsDisplayServer& runner)
        : runner{runner}
    {
    }

    std::shared_ptr<mir::test::Signal> expect_event_with_time(
        std::chrono::nanoseconds event_time)
    {
        auto done_signal = std::make_shared<mir::test::Signal>();
        expected_events.lock()->insert(std::make_pair(event_time, done_signal));
        return done_signal;
    }

    void seat_add_device(uint64_t /*id*/) override
    {
    }

    void seat_remove_device(uint64_t /*id*/) override
    {
    }

    void seat_dispatch_event(std::shared_ptr<MirEvent const> const& event) override
    {
        auto iev = mir_event_get_input_event(event.get());
        auto event_time = std::chrono::nanoseconds{mir_input_event_get_event_time(iev)};

        auto expected_events_accessor = expected_events.lock();
        if (expected_events_accessor->count(event_time))
        {
            expected_events_accessor->at(event_time)->raise();
            expected_events_accessor->erase(event_time);
        }
    }

    void seat_set_key_state(
        uint64_t /*id*/,
        std::vector<uint32_t> const& /*scan_codes*/) override
    {
    }

    void seat_set_pointer_state(
        uint64_t /*id*/,
        unsigned /*buttons*/) override
    {
    }

    void seat_set_cursor_position(
        float cursor_x,
        float cursor_y) override
    {
        runner.cursor_x = cursor_x;
        runner.cursor_y = cursor_y;
    }

    void seat_set_confinement_region_called(
        mir::geometry::Rectangles const& /*regions*/) override
    {
    }

    void seat_reset_confinement_regions() override
    {
    }

private:
    Mutex<std::unordered_map<std::chrono::nanoseconds, std::shared_ptr<mir::test::Signal>>> expected_events;
    MirWlcsDisplayServer& runner;
};

template<typename T>
void emit_mir_event(MirWlcsDisplayServer* runner,
                    mir::UniqueModulePtr<mir_test_framework::FakeInputDevice>& emitter,
                    T event)
{
    auto event_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch());

    auto event_sent = runner->event_listener->expect_event_with_time(event_time);

    emitter->emit_event(event.with_event_time(event_time));

    EXPECT_THAT(event_sent->wait_for(a_long_time), testing::Eq(true)) << "fake event failed to go through";
}
}

void wlcs_server_start(WlcsDisplayServer* server)
{
    mir::test::Signal started;

    auto runner = static_cast<MirWlcsDisplayServer*>(server);

    runner->start_server();

    runner->server->run_on_wayland_display(
        [runner, &started](auto wayland_display)
            {
                runner->resource_mapper->init(wayland_display);
                runner->executor = WaylandExecutor::executor_for_event_loop(
                    wl_display_get_event_loop(wayland_display));

                // Execute all observations on the Wayland event loop…
                runner->server->the_seat_observer_registrar()->register_interest(
                    runner->event_listener,
                    *runner->executor);

                started.raise();
            });


    started.wait_for(a_long_time);
}

void wlcs_server_stop(WlcsDisplayServer* server)
{
    auto runner = static_cast<MirWlcsDisplayServer*>(server);

    runner->stop_server();
}


WlcsDisplayServer* wlcs_create_server(int argc, char const** argv)
{
    auto runner = new MirWlcsDisplayServer;
    runner->add_to_environment("MIR_SERVER_ENABLE_KEY_REPEAT", "false");
    runner->add_to_environment("MIR_SERVER_WAYLAND_SOCKET_NAME", "wlcs-tests");
    runner->add_to_environment("WAYLAND_DISPLAY", "wlcs-tests");

    runner->event_listener = std::make_shared<InputEventListener>(*runner);

    runner->add_server_init([runner, argc, argv](mir::Server& server)
        {
            server.override_the_session_listener(
                [runner]()
                    {
                        return runner->resource_mapper;
                    });

            server.set_command_line(argc, argv);

            server.wrap_cursor_listener(
                [runner](auto const& wrappee)
                    {
                        class ListenerWrapper : public mir::input::CursorListener
                        {
                        public:
                            ListenerWrapper(
                                MirWlcsDisplayServer* runner,
                                std::shared_ptr<mir::input::CursorListener> const& wrapped)
                                : runner{runner},
                                wrapped{wrapped}
                            {
                            }

                            void cursor_moved_to(float abs_x, float abs_y) override
                            {
                                runner->cursor_x = abs_x;
                                runner->cursor_y = abs_y;
                                wrapped->cursor_moved_to(abs_x, abs_y);
                            }

                        private:
                            MirWlcsDisplayServer* const runner;
                            std::shared_ptr<mir::input::CursorListener> const wrapped;
                        };
                        return std::make_shared<ListenerWrapper>(runner, wrappee);
                    });

            runner->server = &server;
        });

    return static_cast<WlcsDisplayServer*>(runner);
}

void wlcs_destroy_server(WlcsDisplayServer* server)
{
    auto runner = static_cast<MirWlcsDisplayServer*>(server);
    delete runner;
}

int wlcs_server_create_client_socket(WlcsDisplayServer* server)
{
    auto runner = static_cast<MirWlcsDisplayServer*>(server);

    try
    {
        auto client_fd = fcntl(
            runner->server->open_wayland_client_socket(),
            F_DUPFD_CLOEXEC,
            3);

        runner->resource_mapper->associate_client_socket(client_fd);

        return client_fd;
    }
    catch (std::exception const&)
    {
        mir::log(
            mir::logging::Severity::critical,
            "wlcs-bindings",
            std::current_exception(),
            "Failed to create Wayland client socket");
    }

    return -1;
}

struct FakePointer : public WlcsPointer
{
    FakePointer();

    decltype(mtf::add_fake_input_device(mi::InputDeviceInfo())) pointer;
    MirWlcsDisplayServer* runner;
};

WlcsPointer* wlcs_server_create_pointer(WlcsDisplayServer* server)
{
    auto runner = static_cast<MirWlcsDisplayServer*>(server);

    auto constexpr uid = "mouse-uid";

    class DeviceObserver : public mir::input::InputDeviceObserver
    {
    public:
        DeviceObserver(std::shared_ptr<mir::test::Signal> const& done)
            : done{done}
        {
        }

        void device_added(std::shared_ptr<mir::input::Device> const& device) override
        {
            if (device->unique_id() == uid)
                seen_device = true;
        }

        void device_changed(std::shared_ptr<mir::input::Device> const&) override
        {
        }

        void device_removed(std::shared_ptr<mir::input::Device> const&) override
        {
        }

        void changes_complete() override
        {
            if (seen_device)
                done->raise();
        }

    private:
        std::shared_ptr<mir::test::Signal> const done;
        bool seen_device{false};
    };

    auto mouse_added = std::make_shared<mir::test::Signal>();
    auto observer = std::make_shared<DeviceObserver>(mouse_added);
    runner->server->the_input_device_hub()->add_observer(observer);


    auto fake_mouse = mtf::add_fake_input_device(
        mi::InputDeviceInfo{"mouse", uid, mi::DeviceCapability::pointer});

    mouse_added->wait_for(a_long_time);
    runner->executor->spawn([observer=std::move(observer), the_input_device_hub=runner->server->the_input_device_hub()]
        { the_input_device_hub->remove_observer(observer); });

    auto fake_pointer = new FakePointer;
    fake_pointer->runner = runner;
    fake_pointer->pointer = std::move(fake_mouse);

    return static_cast<WlcsPointer*>(fake_pointer);
}

void wlcs_destroy_pointer(WlcsPointer* pointer)
{
    delete static_cast<FakePointer*>(pointer);
}

void wlcs_pointer_move_relative(WlcsPointer* pointer, wl_fixed_t x, wl_fixed_t y)
{
    auto device = static_cast<FakePointer*>(pointer);

    auto event = mir::input::synthesis::a_pointer_event()
                    .with_movement(wl_fixed_to_int(x), wl_fixed_to_int(y));

    emit_mir_event(device->runner, device->pointer, event);
}

void wlcs_pointer_move_absolute(WlcsPointer* pointer, wl_fixed_t x, wl_fixed_t y)
{
    auto device = static_cast<FakePointer*>(pointer);

    auto rel_x = wl_fixed_to_double(x) - device->runner->cursor_x;
    auto rel_y = wl_fixed_to_double(y) - device->runner->cursor_y;

    wlcs_pointer_move_relative(pointer, wl_fixed_from_double(rel_x), wl_fixed_from_double(rel_y));
}

void wlcs_pointer_button_down(WlcsPointer* pointer, int button)
{
    auto device = static_cast<FakePointer*>(pointer);

    auto event = mir::input::synthesis::a_button_down_event()
                    .of_button(button);

    emit_mir_event(device->runner, device->pointer, event);
}

void wlcs_pointer_button_up(WlcsPointer* pointer, int button)
{
    auto device = static_cast<FakePointer*>(pointer);

    auto event = mir::input::synthesis::a_button_up_event()
                    .of_button(button);

    emit_mir_event(device->runner, device->pointer, event);
}

FakePointer::FakePointer()
{
    version = 1;
    move_absolute = &wlcs_pointer_move_absolute;
    move_relative = &wlcs_pointer_move_relative;
    button_up = &wlcs_pointer_button_up;
    button_down = &wlcs_pointer_button_down;
    destroy = &wlcs_destroy_pointer;
}

struct FakeTouch : public WlcsTouch
{
    FakeTouch();

    decltype(mtf::add_fake_input_device(mi::InputDeviceInfo())) touch;
    int last_x{0}, last_y{0};
    MirWlcsDisplayServer* runner;
};

WlcsTouch* wlcs_server_create_touch(WlcsDisplayServer* server)
{
    auto runner = static_cast<MirWlcsDisplayServer*>(server);

    auto constexpr uid = "touch-uid";

    class DeviceObserver : public mir::input::InputDeviceObserver
    {
    public:
        DeviceObserver(std::shared_ptr<mir::test::Signal> const& done)
            : done{done}
        {
        }

        void device_added(std::shared_ptr<mir::input::Device> const& device) override
        {
            if (device->unique_id() == uid)
                seen_device = true;
        }

        void device_changed(std::shared_ptr<mir::input::Device> const&) override
        {
        }

        void device_removed(std::shared_ptr<mir::input::Device> const&) override
        {
        }

        void changes_complete() override
        {
            if (seen_device)
                done->raise();
        }

    private:
        std::shared_ptr<mir::test::Signal> const done;
        bool seen_device{false};
    };

    auto touch_added = std::make_shared<mir::test::Signal>();
    auto observer = std::make_shared<DeviceObserver>(touch_added);
    runner->server->the_input_device_hub()->add_observer(observer);

    auto fake_touch_dev = mtf::add_fake_input_device(
        mi::InputDeviceInfo{"touch", uid, mi::DeviceCapability::multitouch});

    touch_added->wait_for(a_long_time);
    runner->executor->spawn([observer=std::move(observer), the_input_device_hub=runner->server->the_input_device_hub()]
        { the_input_device_hub->remove_observer(observer); });

    auto fake_touch = new FakeTouch;
    fake_touch->runner = runner;
    fake_touch->touch = std::move(fake_touch_dev);

    return static_cast<WlcsTouch*>(fake_touch);
}

void wlcs_destroy_touch(WlcsTouch* touch)
{
    delete static_cast<FakeTouch*>(touch);
}

void wlcs_touch_down(WlcsTouch* touch, int x, int y)
{
    auto device = static_cast<FakeTouch*>(touch);

    device->last_x = x;
    device->last_y = y;

    auto event = mir::input::synthesis::a_touch_event()
                    .with_action(mir::input::synthesis::TouchParameters::Action::Tap)
                    .at_position({x, y});

    emit_mir_event(device->runner, device->touch, event);
}

void wlcs_touch_move(WlcsTouch* touch, int x, int y)
{
    auto device = static_cast<FakeTouch*>(touch);

    device->last_x = x;
    device->last_y = y;

    auto event = mir::input::synthesis::a_touch_event()
                    .with_action(mir::input::synthesis::TouchParameters::Action::Move)
                    .at_position({x, y});

    emit_mir_event(device->runner, device->touch, event);
}

void wlcs_touch_up(WlcsTouch* touch)
{
    auto device = static_cast<FakeTouch*>(touch);

    auto event = mir::input::synthesis::a_touch_event()
                    .with_action(mir::input::synthesis::TouchParameters::Action::Release)
                    .at_position({device->last_x, device->last_y});

    emit_mir_event(device->runner, device->touch, event);
}

FakeTouch::FakeTouch()
{
    version = 1;

    touch_down = &wlcs_touch_down;
    touch_move = &wlcs_touch_move;
    touch_up = &wlcs_touch_up;

    destroy = &wlcs_destroy_touch;
}

void wlcs_server_position_window_absolute(WlcsDisplayServer* server, wl_display* client, wl_surface* surface, int x, int y)
{
    auto runner = static_cast<MirWlcsDisplayServer*>(server);

    try
    {
        auto const fd = wl_display_get_fd(client);
        auto const client = runner->resource_mapper->client_for_fd(fd);
        auto const id = wl_proxy_get_id(reinterpret_cast<wl_proxy*>(surface));

        auto resource = wl_client_get_object(client, id);

        auto mir_surface = runner->resource_mapper->surface_for_resource(resource);

        if (auto live_surface = mir_surface.lock())
        {
            live_surface->move_to(mir::geometry::Point{x, y});
        }
        else
        {
            abort();
            // TODO: log? Error handling?
        }
    }
    catch(std::out_of_range const&)
    {
        abort();
        // TODO: Error handling.
    }
}

extern WlcsServerIntegration const wlcs_server_integration {
    1,
    &wlcs_create_server,
    &wlcs_destroy_server,
};

namespace
{
WlcsExtensionDescriptor const extensions[] = {
    {"wl_compositor", 4},
    {"wl_shm", 1},
    {"wl_data_device_manager", 3},
    {"wl_shell", 1},
    {"wl_seat", 6},
    {"wl_output", 3},
    {"wl_subcompositor", 1},
    {"xdg_wm_base", 1},
    {"zxdg_shell_unstable_v6", 1},
    {"wlr_layer_shell_unstable_v1", 1}
};

WlcsIntegrationDescriptor const descriptor {
    1,
    sizeof(extensions) / sizeof(extensions[0]),
    extensions
};

WlcsIntegrationDescriptor const* get_descriptor(WlcsDisplayServer const* /*server*/)
{
    return &descriptor;
}

MirWlcsDisplayServer::MirWlcsDisplayServer()
{
    version = 2;
    start = &wlcs_server_start;
    stop = &wlcs_server_stop;
    create_client_socket = &wlcs_server_create_client_socket;
    position_window_absolute = &wlcs_server_position_window_absolute;
    create_pointer = &wlcs_server_create_pointer;
    create_touch = &wlcs_server_create_touch;
    get_descriptor = &::get_descriptor;
}

auto MirWlcsDisplayServer::build_window_manager_policy(miral::WindowManagerTools const& tools)
->std::unique_ptr<TestWindowManagerPolicy>
{
    return std::make_unique<TestWindowManagerPolicy>(tools, *this);
}
}
