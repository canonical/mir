/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <mir_test_framework/input_recording_test_harness.h>
#include <mir_test_framework/executable_path.h>
#include <mir/test/doubles/null_emergency_cleanup.h>
#include <mir/test/doubles/null_input_report.h>
#include <mir/test/doubles/stub_console_services.h>

#include <mir/dispatch/action_queue.h>
#include <mir/dispatch/multiplexing_dispatchable.h>
#include <mir/dispatch/threaded_dispatcher.h>
#include <mir/events/event.h>
#include <mir/events/event_builders.h>
#include <mir/geometry/rectangle.h>
#include <mir/input/event_builder.h>
#include <mir/input/input_device.h>
#include <mir/input/input_device_registry.h>
#include <mir/input/input_report.h>
#include <mir/input/input_sink.h>
#include <mir/input/platform.h>
#include <mir/options/program_option.h>
#include <mir/shared_library.h>
#include <mir/synchronised.h>
#include <mir/udev/wrapper.h>

#include <boost/throw_exception.hpp>

#include <fcntl.h>
#include <sys/sysmacros.h>

#include <cerrno>
#include <condition_variable>
#include <cstdint>
#include <filesystem>
#include <future>
#include <optional>
#include <stdexcept>
#include <system_error>

namespace mtf = mir_test_framework;
namespace mtd = mir::test::doubles;
namespace mi = mir::input;
namespace geom = mir::geometry;
namespace md = mir::dispatch;

using namespace std::chrono_literals;

namespace
{
geom::Rectangle const screen{{0, 0}, {1920, 1080}};

// StubConsoleServices opens /dev/null for input devices, which libinput rejects. Open the (umockdev) device node.
class DevnodeConsoleServices : public mtd::StubConsoleServices
{
public:
    auto acquire_device(int major, int minor, std::unique_ptr<mir::Device::Observer> observer)
        -> std::future<std::unique_ptr<mir::Device>> override
    {
        std::promise<std::unique_ptr<mir::Device>> promise;
        try
        {
            auto const device = udev.char_device_from_devnum(makedev(major, minor));
            mir::Fd fd{::open(device->devnode(), O_RDWR | O_NONBLOCK | O_CLOEXEC)};
            if (fd == mir::Fd::invalid)
            {
                BOOST_THROW_EXCEPTION((std::system_error{errno, std::system_category(), "open() failed"}));
            }
            observer->activated(std::move(fd));
            promise.set_value(nullptr);
        }
        catch (...)
        {
            promise.set_exception(std::current_exception());
        }
        return promise.get_future();
    }

private:
    mir::udev::Context udev;
};

class EventBuilder : public mi::EventBuilder
{
public:
    explicit EventBuilder(MirInputDeviceId device_id)
        : device_id{device_id}
    {
    }

    auto key_event(
        std::optional<Timestamp> timestamp,
        MirKeyboardAction action,
        xkb_keysym_t keysym,
        int scan_code) -> mir::EventUPtr override
    {
        return mir::events::make_key_event(
            device_id, timestamp.value_or(now()), action, keysym, scan_code, mir_input_event_modifier_none);
    }

    auto pointer_event(
        std::optional<Timestamp> timestamp,
        MirPointerAction action,
        MirPointerButtons buttons,
        std::optional<geom::PointF> position,
        geom::DisplacementF motion,
        MirPointerAxisSource axis_source,
        mir::events::ScrollAxisH h_scroll,
        mir::events::ScrollAxisV v_scroll) -> mir::EventUPtr override
    {
        return mir::events::make_pointer_event(
            device_id,
            timestamp.value_or(now()),
            mir_input_event_modifier_none,
            action,
            buttons,
            position,
            motion,
            axis_source,
            h_scroll,
            v_scroll);
    }

    auto touch_event(
        std::optional<Timestamp> timestamp,
        std::vector<mir::events::TouchContact> const& contacts) -> mir::EventUPtr override
    {
        return mir::events::make_touch_event(device_id, timestamp.value_or(now()), mir_input_event_modifier_none, contacts);
    }

private:
    static auto now() -> Timestamp
    {
        return std::chrono::steady_clock::now().time_since_epoch();
    }

    MirInputDeviceId const device_id;
};
}

std::chrono::milliseconds const mtf::InputRecordingTestHarness::default_timeout{2s};

struct mtf::InputRecordingTestHarness::Self : mi::InputSink, mi::InputDeviceRegistry
{
    explicit Self(std::string const& module_name)
        : library{mtf::server_input_platform(module_name)}
    {
    }

    void handle_input(std::shared_ptr<MirEvent> const& event) override
    {
        state.lock()->events.emplace_back(event->clone());
        changed.notify_all();
    }

    auto bounding_rectangle() const -> geom::Rectangle override
    {
        return screen;
    }

    auto output_info(uint32_t) const -> mi::OutputInfo override
    {
        return mi::OutputInfo{true, screen.size, mi::OutputInfo::Matrix{{1, 0, 0, 0, 1, 0}}};
    }

    void key_state(std::vector<uint32_t> const&) override {}
    void pointer_state(MirPointerButtons) override {}

    // Called on the input thread, so the device is started before it can process any events
    auto add_device(std::shared_ptr<mi::InputDevice> const& device) -> std::weak_ptr<mi::Device> override
    {
        {
            auto const locked = state.lock();
            auto& builder = locked->builders.emplace_back(std::make_unique<EventBuilder>(locked->next_device_id++));
            device->start(this, builder.get());
            locked->devices.push_back(device);
        }
        changed.notify_all();
        return {};
    }

    void remove_device(std::shared_ptr<mi::InputDevice> const& device) override
    {
        device->stop();
        std::erase(state.lock()->devices, device);
        changed.notify_all();
    }

    mir::SharedLibrary const library;
    std::shared_ptr<mi::Platform> platform;
    std::shared_ptr<md::ActionQueue> const actions{std::make_shared<md::ActionQueue>()};
    std::shared_ptr<md::MultiplexingDispatchable> const multiplexer{std::make_shared<md::MultiplexingDispatchable>()};
    std::unique_ptr<md::ThreadedDispatcher> input_thread;

    struct State
    {
        MirInputDeviceId next_device_id{1};
        std::vector<std::unique_ptr<EventBuilder>> builders;
        Devices devices;
        Events events;
    };

    mir::Synchronised<State> state;
    std::condition_variable changed;
};

mtf::InputRecordingTestHarness::InputRecordingTestHarness(std::string const& module_name)
    : self{std::make_unique<Self>(module_name)}
{
}

mtf::InputRecordingTestHarness::~InputRecordingTestHarness()
{
    if (self->input_thread)
    {
        run_on_input_thread(
            [this]
            {
                self->multiplexer->remove_watch(self->platform->dispatchable());
                self->platform->stop();
            });
        self->input_thread.reset();
    }
}

void mtf::InputRecordingTestHarness::add_device(std::string const& name)
{
    udev.add_standard_device(name);
}

void mtf::InputRecordingTestHarness::start()
{
    auto const create_platform = self->library.load_function<mi::CreatePlatform>("create_input_platform");
    self->platform = create_platform(
        mir::options::ProgramOption{},
        std::make_shared<mtd::NullEmergencyCleanup>(),
        std::shared_ptr<mi::InputDeviceRegistry>{self.get(), [](auto*) {}},
        std::make_shared<DevnodeConsoleServices>(),
        std::make_shared<mtd::NullInputReport>());

    self->multiplexer->add_watch(self->actions);
    self->input_thread = std::make_unique<md::ThreadedDispatcher>("Recorded input", self->multiplexer);

    // As the server does, start the platform on the input thread
    run_on_input_thread(
        [this]
        {
            self->platform->start();
            self->multiplexer->add_watch(self->platform->dispatchable());
        });
}

auto mtf::InputRecordingTestHarness::wait_for_devices(std::size_t count, std::chrono::milliseconds timeout) -> Devices
{
    auto locked = self->state.lock();
    locked.wait_for(self->changed, timeout, [&] { return locked->devices.size() >= count; });
    return locked->devices;
}

void mtf::InputRecordingTestHarness::run_on_input_thread(std::function<void()> const& action)
{
    std::promise<void> done;
    auto finished = done.get_future();
    self->actions->enqueue(
        [&]
        {
            try
            {
                action();
                done.set_value();
            }
            catch (...)
            {
                done.set_exception(std::current_exception());
            }
        });
    finished.get();
}

void mtf::InputRecordingTestHarness::play(std::string const& name)
{
    udev.play_device_evemu(name);
}

auto mtf::InputRecordingTestHarness::wait_for_events(
    std::function<bool(Events const&)> const& done,
    std::chrono::milliseconds timeout) -> Events
{
    auto locked = self->state.lock();
    locked.wait_for(self->changed, timeout, [&] { return done(locked->events); });
    return locked->events;
}

auto mtf::InputRecordingTestHarness::wait_for_events(std::size_t count, std::chrono::milliseconds timeout) -> Events
{
    return wait_for_events([count](Events const& events) { return events.size() >= count; }, timeout);
}

auto mtf::InputRecordingTestHarness::settled_events(std::chrono::milliseconds quiet_period) -> Events
{
    // Two round trips ensure the input thread has polled the (already written) device fds at least once
    run_on_input_thread([]{});
    run_on_input_thread([]{});

    // Some events (eg: touchpad timeouts) are driven by timers rather than device input
    auto const deadline = std::chrono::steady_clock::now() + default_timeout;
    auto locked = self->state.lock();
    for (auto seen = locked->events.size(); std::chrono::steady_clock::now() < deadline; seen = locked->events.size())
    {
        if (!locked.wait_for(self->changed, quiet_period, [&] { return locked->events.size() != seen; }))
        {
            break;
        }
    }
    return locked->events;
}
