/*
 * Copyright © Canonical Ltd.
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
 */

#include "test_wlcs_display_server_internals.h"

#include <miral/test_wlcs_display_server.h>
#include <wlcs/pointer.h>
#include <wlcs/touch.h>

#include <mir_test_framework/executable_path.h>
#include <mir_test_framework/fake_input_device.h>
#include <mir_test_framework/stub_server_platform_factory.h>
#include <mir/test/signal.h>
#include <mir/test/fake_shared.h>
#include <mir/test/null_input_device_observer.h>

#include <mir/executor.h>
#include <mir/errno_utils.h>
#include <mir/fd.h>
#include <mir/input/cursor_observer.h>
#include <mir/input/cursor_observer_multiplexer.h>
#include <mir/input/device.h>
#include <mir/input/input_device_hub.h>
#include <mir/input/input_device_info.h>
#include <mir/input/input_device_observer.h>
#include <mir/input/seat_observer.h>
#include <mir/log.h>
#include <mir/observer_registrar.h>
#include <mir/scene/session.h>
#include <mir/scene/surface.h>
#include <mir/server.h>

#include <wayland-client.h>

#include <fcntl.h>
#include <sys/eventfd.h>

#include <cstring>
#include <deque>
#include <mutex>
#include <format>
#include <optional>
#include <unordered_map>


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
        return MutexGuard<Guarded>{std::unique_lock{mutex}, value};
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
        std::unique_lock lock{this->mutex};
        if (!notifier.wait_for(
            lock, timeout, [this, &predicate]()
                { return predicate(this->value); }))
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


}


/// end of anon

class miral::TestWlcsDisplayServer::RunnerCursorObserver : public mir::input::CursorObserver
{
public:
    RunnerCursorObserver(TestWlcsDisplayServer* runner) :
        runner{runner}
    {
    }

    void cursor_moved_to(float abs_x, float abs_y) override
    {
        runner->cursor_x = abs_x;
        runner->cursor_y = abs_y;
    }

    void pointer_usable() override
    {
    }

    void pointer_unusable() override
    {
    }

    void image_set_to(std::shared_ptr<mir::graphics::CursorImage>) override
    {
    }

private:
    TestWlcsDisplayServer* const runner;
};

class miral::TestWlcsDisplayServer::ResourceMapper
{
public:
    /// Record the association between the client socket `client_fd` (as returned
    /// to WLCS) and the Mir `session` created for that connection.
    void associate_client_socket(int client_fd, std::shared_ptr<mir::scene::Session> const& session)
    {
        std::lock_guard lock{mutex};
        session_for_fd_[client_fd] = session;
    }

    /// Look up the Mir session for a client socket, or nullptr if unknown.
    auto session_for_fd(int client_fd) -> std::shared_ptr<mir::scene::Session>
    {
        std::lock_guard lock{mutex};
        if (auto const iter = session_for_fd_.find(client_fd); iter != session_for_fd_.end())
            return iter->second;
        return nullptr;
    }

private:
    std::mutex mutex;
    std::unordered_map<int, std::shared_ptr<mir::scene::Session>> session_for_fd_;
};

class miral::TestWlcsDisplayServer::InputEventListener : public mir::input::SeatObserver
{
public:
    InputEventListener(TestWlcsDisplayServer& runner)
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
    TestWlcsDisplayServer& runner;
};

struct miral::TestWlcsDisplayServer::FakePointer : public WlcsPointer
{
    FakePointer();


    double cursor_x() const { return runner->cursor_x; }
    double cursor_y() const { return runner->cursor_y; }

    decltype(mtf::add_fake_input_device(mi::InputDeviceInfo())) pointer;
    TestWlcsDisplayServer* runner;
};

namespace
{
WlcsPointer* wlcs_server_create_pointer(WlcsDisplayServer* server)
{
    return static_cast<miral::TestWlcsDisplayServer*>(server)->create_pointer();
}

void wlcs_destroy_pointer(WlcsPointer* pointer)
{
    delete static_cast<miral::TestWlcsDisplayServer::FakePointer*>(pointer);
}

void wlcs_pointer_move_relative(WlcsPointer* pointer, wl_fixed_t x, wl_fixed_t y)
{
    auto device = static_cast<miral::TestWlcsDisplayServer::FakePointer*>(pointer);

    auto event = mir::input::synthesis::a_pointer_event()
        .with_movement(wl_fixed_to_int(x), wl_fixed_to_int(y));

    emit_mir_event(device->runner, device->pointer, event);
}

void wlcs_pointer_move_absolute(WlcsPointer* pointer, wl_fixed_t x, wl_fixed_t y)
{
    auto device = static_cast<miral::TestWlcsDisplayServer::FakePointer*>(pointer);

    auto rel_x = wl_fixed_to_double(x) - device->cursor_x();
    auto rel_y = wl_fixed_to_double(y) - device->cursor_y();

    wlcs_pointer_move_relative(pointer, wl_fixed_from_double(rel_x), wl_fixed_from_double(rel_y));
}

void wlcs_pointer_button_down(WlcsPointer* pointer, int button)
{
    auto device = static_cast<miral::TestWlcsDisplayServer::FakePointer*>(pointer);

    auto event = mir::input::synthesis::a_button_down_event()
        .of_button(button);

    emit_mir_event(device->runner, device->pointer, event);
}

void wlcs_pointer_button_up(WlcsPointer* pointer, int button)
{
    auto device = static_cast<miral::TestWlcsDisplayServer::FakePointer*>(pointer);

    auto event = mir::input::synthesis::a_button_up_event()
        .of_button(button);

    emit_mir_event(device->runner, device->pointer, event);
}
}

miral::TestWlcsDisplayServer::FakePointer::FakePointer()
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
    miral::TestWlcsDisplayServer* runner;
};

namespace
{

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
}

FakeTouch::FakeTouch()
{
    version = 1;

    touch_down = &wlcs_touch_down;
    touch_move = &wlcs_touch_move;
    touch_up = &wlcs_touch_up;

    destroy = &wlcs_destroy_touch;
}

namespace
{
void wlcs_server_start(WlcsDisplayServer* server)
{
    static_cast<miral::TestWlcsDisplayServer*>(server)->start_server();
}

void wlcs_server_stop(WlcsDisplayServer* server)
{
    static_cast<miral::TestWlcsDisplayServer*>(server)->stop_server();
}

int wlcs_server_create_client_socket(WlcsDisplayServer* server)
{
    return static_cast<miral::TestWlcsDisplayServer*>(server)->create_client_socket();
}

void wlcs_server_position_window_absolute(WlcsDisplayServer* server, wl_display* client, wl_surface* surface, int x, int y)
try
{
    static_cast<miral::TestWlcsDisplayServer*>(server)->position_window(client, surface, mir::geometry::Point{x, y});
}
catch(std::out_of_range const&)
{
    abort();
    // TODO: Error handling.
}

WlcsTouch* wlcs_server_create_touch(WlcsDisplayServer* server)
{
    return static_cast<miral::TestWlcsDisplayServer*>(server)->create_touch();
}

[[maybe_unused]]
WlcsKeyboard* wlcs_server_create_keyboard(WlcsDisplayServer* server)
{
    return static_cast<miral::TestWlcsDisplayServer*>(server)->create_keyboard();
}
}

miral::TestWlcsDisplayServer::TestWlcsDisplayServer(int argc, char const** argv) :
    TestDisplayServer{argc, argv},
    resource_mapper{std::make_shared<ResourceMapper>()},
    event_listener{std::make_shared<InputEventListener>(*this)}
{
    WlcsDisplayServer::version = std::min(WLCS_DISPLAY_SERVER_VERSION, 4);
    WlcsDisplayServer::start = &wlcs_server_start;
    WlcsDisplayServer::stop = &wlcs_server_stop;
    WlcsDisplayServer::create_client_socket = &wlcs_server_create_client_socket;
    WlcsDisplayServer::position_window_absolute = &wlcs_server_position_window_absolute;
    WlcsDisplayServer::create_pointer = &wlcs_server_create_pointer;
    WlcsDisplayServer::create_touch = &wlcs_server_create_touch;
#if WLCS_DISPLAY_SERVER_VERSION >= 4
    WlcsDisplayServer::create_keyboard = &wlcs_server_create_keyboard;
#endif

    add_to_environment("MIR_SERVER_CURSOR", "null");
    add_to_environment("MIR_SERVER_ENABLE_KEY_REPEAT", "false");
    const auto wayland_display = std::format("wlcs-tests-{}", getpid());
    add_to_environment("WAYLAND_DISPLAY", wayland_display.c_str());

    add_server_init([this](mir::Server& server)
        {
            server.add_init_callback([this, &server]()
                {
                    cursor_observer = std::make_shared<RunnerCursorObserver>(this);
                    server.the_cursor_observer_multiplexer()->register_early_observer(
                        cursor_observer, mir::immediate_executor);
                });

            mir_server = &server;
        });
}

void miral::TestWlcsDisplayServer::start_server()
{
    TestDisplayServer::start_server();

    mir::test::Signal started;

    mir_server->run_on_wayland_display(
        [this, &started](mir::Executor& wayland_executor)
            {
                // The executor is owned by the running Wayland backend and
                // outlives our use of it, so wrap it in a non-owning shared_ptr.
                executor = std::shared_ptr<mir::Executor>(std::shared_ptr<void>{}, &wayland_executor);

                // Execute all observations on the Wayland event loop…
                mir_server->the_seat_observer_registrar()->register_interest(event_listener, *executor);

                started.raise();
            });

    started.wait_for(a_long_time);
}

int miral::TestWlcsDisplayServer::create_client_socket()
{
    try
    {
        auto const session_ready = std::make_shared<mir::test::Signal>();
        auto const session_holder = std::make_shared<std::shared_ptr<mir::scene::Session>>();

        auto const raw_fd = mir_server->open_client_wayland(
            [session_ready, session_holder](std::shared_ptr<mir::scene::Session> const& session)
            {
                *session_holder = session;
                session_ready->raise();
            });

        auto const client_fd = fcntl(raw_fd, F_DUPFD_CLOEXEC, 3);
        ::close(raw_fd);

        if (!session_ready->wait_for(a_long_time))
            BOOST_THROW_EXCEPTION((std::runtime_error{"Timeout waiting for client session"}));

        resource_mapper->associate_client_socket(client_fd, *session_holder);

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

void miral::TestWlcsDisplayServer::position_window(wl_display* client_, wl_surface* surface, mir::geometry::Point point)
{
    auto const fd = wl_display_get_fd(client_);
    auto const session = resource_mapper->session_for_fd(fd);
    auto const id = wl_proxy_get_id(reinterpret_cast<wl_proxy*>(surface));

    if (!session)
        BOOST_THROW_EXCEPTION((std::out_of_range{"No session for client socket"}));

    if (auto const scene_surface = mir_server->scene_surface_for_wayland_surface(*session, id))
    {
        scene_surface->move_to(point);
    }
    else
    {
        abort();
        // TODO: log? Error handling?
    }
}

WlcsPointer* miral::TestWlcsDisplayServer::create_pointer()
{
    auto constexpr uid = "mouse-uid";

    class DeviceObserver : public mir::input::NullInputDeviceObserver
    {
    public:
        explicit DeviceObserver(std::shared_ptr<mir::test::Signal> const& done)
            : done{done}
        {
        }

        void device_added(std::shared_ptr<mir::input::Device> const& device) override
        {
            if (device->unique_id() == uid)
                seen_device = true;
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
    mir_server->the_input_device_hub()->add_observer(observer);


    auto fake_mouse = mtf::add_fake_input_device(
        mi::InputDeviceInfo{"mouse", uid, mi::DeviceCapability::pointer});

    mouse_added->wait_for(a_long_time);
    executor->spawn([observer=std::move(observer), the_input_device_hub=mir_server->the_input_device_hub()]
                        { the_input_device_hub->remove_observer(observer); });

    auto fake_pointer = new FakePointer{};
    fake_pointer->runner = this;
    fake_pointer->pointer = std::move(fake_mouse);

    return static_cast<WlcsPointer*>(fake_pointer);
}

WlcsTouch* miral::TestWlcsDisplayServer::create_touch()
{
    auto constexpr uid = "touch-uid";

    class DeviceObserver : public mir::input::NullInputDeviceObserver
    {
    public:
        explicit DeviceObserver(std::shared_ptr<mir::test::Signal> const& done)
            : done{done}
        {
        }

        void device_added(std::shared_ptr<mir::input::Device> const& device) override
        {
            if (device->unique_id() == uid)
                seen_device = true;
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
    mir_server->the_input_device_hub()->add_observer(observer);

    auto fake_touch_dev = mtf::add_fake_input_device(
        mi::InputDeviceInfo{"touch", uid, mi::DeviceCapability::multitouch});

    touch_added->wait_for(a_long_time);
    executor->spawn([observer=std::move(observer), the_input_device_hub=mir_server->the_input_device_hub()]
                        { the_input_device_hub->remove_observer(observer); });

    auto fake_touch = new FakeTouch{};
    fake_touch->runner = this;
    fake_touch->touch = std::move(fake_touch_dev);

    return static_cast<WlcsTouch*>(fake_touch);
}

std::shared_ptr<mir::test::Signal> miral::TestWlcsDisplayServer::expect_event_with_time(std::chrono::nanoseconds event_time)
{
    return event_listener->expect_event_with_time(event_time);
}
