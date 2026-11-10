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

#include <mir_test_framework/recorded_input_platform.h>
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
#include <mir/raii.h>
#include <mir/shared_library.h>
#include <mir/synchronised.h>
#include <mir/udev/wrapper.h>

#include <boost/throw_exception.hpp>

#include <dlfcn.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <unistd.h>

#include <cerrno>
#include <condition_variable>
#include <cstdarg>
#include <cstdint>
#include <filesystem>
#include <future>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <system_error>
#include <unordered_map>

namespace mtf = mir_test_framework;
namespace mtd = mir::test::doubles;
namespace mi = mir::input;
namespace geom = mir::geometry;
namespace md = mir::dispatch;

using namespace std::chrono_literals;

/**
 * This whole `dup` business is unfortunate.
 *
 * umockdev's ioctl emulation tracks descriptor numbers, not open file descriptions.
 * As a result, a call to `dup` loses ioctl emulation and - for the replay PTYs - the
 * emulated fstat identity. libinput then rejects the copied fd as a non-input device.
 *
 * To fix this, we interpose dup/fctnl/fctnl64 and reopen registered devices while also
 * registering the copies. Everything else uses libc luke usual.
 *
 * The evdev-rs implementation is correct to use `dup` here, so the solution is not
 * to just not use `dup`.
 */
namespace
{
struct Registration
{
    std::string devnode;
    dev_t rdev;
};

mir::Synchronised<std::unordered_map<int, Registration>> registrations;

// The caller still owns fd and must not close it concurrently with duplication.
void reopen_on_dup(int fd, std::string const& devnode)
{
    struct stat st{};
    if (fstat(fd, &st) != 0)
    {
        BOOST_THROW_EXCEPTION(std::system_error(errno, std::system_category(), "Failed to stat recorded input fd"));
    }
    registrations.lock()->insert_or_assign(fd, Registration{devnode, st.st_rdev});
}

auto registered_devnode(int fd) -> std::optional<std::string>
{
    auto const locked = registrations.lock();
    auto const registration = locked->find(fd);
    if (registration == locked->end())
    {
        return std::nullopt;
    }

    // Closed descriptor numbers can be reused by unrelated files between tests.
    struct stat st{};
    if (fstat(fd, &st) != 0 || !S_ISCHR(st.st_mode) || st.st_rdev != registration->second.rdev)
    {
        locked->erase(registration);
        return std::nullopt;
    }
    return registration->second.devnode;
}

using Fcntl = int (*)(int, int, ...);

auto reopen(Fcntl real_fcntl, int fd, int extra_flags, int minimum = 0) -> std::optional<int>
{
    auto const devnode = registered_devnode(fd);
    if (!devnode)
    {
        return std::nullopt;
    }

    if (minimum < 0)
    {
        errno = EINVAL;
        return -1;
    }

    auto const flags = real_fcntl(fd, F_GETFL);
    if (flags < 0)
    {
        return -1;
    }
    auto const reopened = open(devnode->c_str(), flags | extra_flags);
    if (reopened >= 0)
    {
        auto cleanup = mir::raii::deleter_for(&reopened, [](int const* fd) { close(*fd); });
        // Moving the fd with dup2() would lose umockdev's ioctl tracking again.
        if (reopened < minimum)
        {
            cleanup.reset();
            errno = EOPNOTSUPP;
            return -1;
        }
        reopen_on_dup(reopened, *devnode);
        cleanup.release();
    }
    return reopened;
}

auto real_fcntl() -> Fcntl
{
    static auto const real = reinterpret_cast<Fcntl>(dlsym(RTLD_NEXT, "fcntl"));
    return real;
}

auto forward_fcntl(Fcntl real, int fd, int cmd, va_list args) -> int
{
    switch (cmd)
    {
    case F_DUPFD:
    case F_DUPFD_CLOEXEC:
    {
        auto const minimum = va_arg(args, int);
        if (auto const reopened = reopen(real, fd, cmd == F_DUPFD_CLOEXEC ? O_CLOEXEC : 0, minimum))
        {
            return *reopened;
        }
        return real(fd, cmd, minimum);
    }
    case F_SETFD:
    case F_SETFL:
    case F_SETOWN:
    case F_SETSIG:
    case F_SETLEASE:
    case F_NOTIFY:
    case F_SETPIPE_SZ:
    case F_ADD_SEALS:
#ifdef F_DUPFD_QUERY
    case F_DUPFD_QUERY:
#endif
        return real(fd, cmd, va_arg(args, int));
    case F_GETLK:
    case F_SETLK:
    case F_SETLKW:
    case F_OFD_GETLK:
    case F_OFD_SETLK:
    case F_OFD_SETLKW:
#if defined(__GLIBC__) && F_GETLK64 == 12 && F_GETLK == F_GETLK64
    // 32-bit libraries without _FILE_OFFSET_BITS=64 still use the original Linux commands.
    case 5:
    case 6:
    case 7:
#endif
        return real(fd, cmd, va_arg(args, struct flock*));
#if F_GETLK != F_GETLK64
    case F_GETLK64:
    case F_SETLK64:
    case F_SETLKW64:
        return real(fd, cmd, va_arg(args, struct flock64*));
#endif
    case F_GETOWN_EX:
    case F_SETOWN_EX:
        return real(fd, cmd, va_arg(args, struct f_owner_ex*));
#ifdef F_GET_RW_HINT
    case F_GET_RW_HINT:
    case F_SET_RW_HINT:
    case F_GET_FILE_RW_HINT:
    case F_SET_FILE_RW_HINT:
        return real(fd, cmd, va_arg(args, uint64_t*));
#endif
    case F_GETFD:
    case F_GETFL:
    case F_GETOWN:
    case F_GETSIG:
    case F_GETLEASE:
    case F_GETPIPE_SZ:
    case F_GET_SEALS:
#ifdef F_CREATED_QUERY
    case F_CREATED_QUERY:
#endif
        return real(fd, cmd);
    default:
        errno = EINVAL;
        return -1;
    }
}
}

extern "C" int dup(int fd)
{
    static auto const real_dup = reinterpret_cast<int (*)(int)>(dlsym(RTLD_NEXT, "dup"));
    if (auto const reopened = reopen(real_fcntl(), fd, 0))
    {
        return *reopened;
    }
    return real_dup(fd);
}

// Explicit names avoid <fcntl.h>'s large-file redirection; modules may call either symbol.
extern "C" int interposed_fcntl(int fd, int cmd, ...) __asm__("fcntl");
extern "C" int interposed_fcntl64(int fd, int cmd, ...) __asm__("fcntl64");

extern "C" int interposed_fcntl(int fd, int cmd, ...)
{
    va_list args;
    va_start(args, cmd);
    auto const cleanup = mir::raii::paired_calls([] {}, [&] { va_end(args); });
    return forward_fcntl(real_fcntl(), fd, cmd, args);
}

extern "C" int interposed_fcntl64(int fd, int cmd, ...)
{
#ifdef __GLIBC__
    static auto const real = reinterpret_cast<Fcntl>(dlsym(RTLD_NEXT, "fcntl64"));
#else
    auto const real = real_fcntl();
#endif
    va_list args;
    va_start(args, cmd);
    auto const cleanup = mir::raii::paired_calls([] {}, [&] { va_end(args); });
    return forward_fcntl(real, fd, cmd, args);
}

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
            reopen_on_dup(fd, device->devnode());
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

std::chrono::milliseconds const mtf::RecordedInputPlatform::default_timeout{2s};

struct mtf::RecordedInputPlatform::Self : mi::InputSink, mi::InputDeviceRegistry
{
    explicit Self(std::string const& module_name)
        : library{mtf::server_input_platform(module_name)}
    {
    }

    void handle_input(std::shared_ptr<MirEvent> const& event) override
    {
        {
            std::lock_guard lock{mutex};
            events.emplace_back(event->clone());
        }
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
            std::lock_guard lock{mutex};
            auto& builder = builders.emplace_back(std::make_unique<EventBuilder>(next_device_id++));
            device->start(this, builder.get());
            devices.push_back(device);
        }
        changed.notify_all();
        return {};
    }

    void remove_device(std::shared_ptr<mi::InputDevice> const& device) override
    {
        device->stop();
        {
            std::lock_guard lock{mutex};
            std::erase(devices, device);
        }
        changed.notify_all();
    }

    mir::SharedLibrary const library;
    std::shared_ptr<mi::Platform> platform;
    std::shared_ptr<md::ActionQueue> const actions{std::make_shared<md::ActionQueue>()};
    std::shared_ptr<md::MultiplexingDispatchable> const multiplexer{std::make_shared<md::MultiplexingDispatchable>()};
    std::unique_ptr<md::ThreadedDispatcher> input_thread;

    std::mutex mutex;
    std::condition_variable changed;
    MirInputDeviceId next_device_id{1};
    std::vector<std::unique_ptr<EventBuilder>> builders;
    Devices devices;
    Events events;
};

auto mtf::RecordedInputPlatform::module_available(std::string const& module_name) -> bool
{
    try
    {
        return std::filesystem::exists(mtf::server_input_platform(module_name));
    }
    catch (std::runtime_error const&)
    {
        return false;
    }
}

mtf::RecordedInputPlatform::RecordedInputPlatform(std::string const& module_name)
    : self{std::make_unique<Self>(module_name)}
{
}

mtf::RecordedInputPlatform::~RecordedInputPlatform()
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

void mtf::RecordedInputPlatform::add_device(std::string const& name)
{
    udev.add_standard_device(name);
}

void mtf::RecordedInputPlatform::start()
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

auto mtf::RecordedInputPlatform::wait_for_devices(std::size_t count, std::chrono::milliseconds timeout) -> Devices
{
    std::unique_lock lock{self->mutex};
    self->changed.wait_for(lock, timeout, [&] { return self->devices.size() >= count; });
    return self->devices;
}

void mtf::RecordedInputPlatform::run_on_input_thread(std::function<void()> const& action)
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

void mtf::RecordedInputPlatform::play(std::string const& name)
{
    udev.wait_for_device_script(udev.load_device_evemu(name));
}

auto mtf::RecordedInputPlatform::wait_for_events(
    std::function<bool(Events const&)> const& done,
    std::chrono::milliseconds timeout) -> Events
{
    std::unique_lock lock{self->mutex};
    self->changed.wait_for(lock, timeout, [&] { return done(self->events); });
    return self->events;
}

auto mtf::RecordedInputPlatform::wait_for_events(std::size_t count, std::chrono::milliseconds timeout) -> Events
{
    return wait_for_events([count](Events const& events) { return events.size() >= count; }, timeout);
}

auto mtf::RecordedInputPlatform::settled_events(std::chrono::milliseconds quiet_period) -> Events
{
    // Two round trips ensure the input thread has polled the (already written) device fds at least once
    run_on_input_thread([]{});
    run_on_input_thread([]{});

    // Some events (eg: touchpad timeouts) are driven by timers rather than device input
    auto const deadline = std::chrono::steady_clock::now() + default_timeout;
    std::unique_lock lock{self->mutex};
    for (auto seen = self->events.size(); std::chrono::steady_clock::now() < deadline; seen = self->events.size())
    {
        if (!self->changed.wait_for(lock, quiet_period, [&] { return self->events.size() != seen; }))
        {
            break;
        }
    }
    return self->events;
}
