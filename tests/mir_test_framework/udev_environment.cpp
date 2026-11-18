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

#include <mir_test_framework/udev_environment.h>
#include <mir_test_framework/executable_path.h>

#include <mir/udev/wrapper.h>
#include <mir/raii.h>

#include <umockdev.h>

#include <sys/stat.h>
#include <linux/input.h>
#include <unistd.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <optional>
#include <span>
#include <string>
#include <vector>
#include <stdexcept>
#include <system_error>
#include <boost/throw_exception.hpp>
#include <boost/exception/errinfo_errno.hpp>
#include <unordered_set>


namespace mtf = mir_test_framework;

/*
 * Conveniently, umockdev doesn't do the setup required to make
 * udev_device_get_is_initialized return true; this breaks the invariant
 * "either udev_device_get_is_initialized returns true, or you will receive
 * an ADD event for it once it is initialized".
 *
 * Just always return initialised for now, until we want to verify.
 */
int udev_device_get_is_initialized(udev_device*)
{
    return 1;
}

mtf::UdevEnvironment::UdevEnvironment()
    : recordings_path(mtf::test_data_path() + "/udev-recordings")
{
    if (!std::getenv("LD_PRELOAD"))
    {
        puts("This test expects LD_PRELOAD=libumockdev-preload.so.0");
        abort();
    }
    testbed = umockdev_testbed_new();
}

mtf::UdevEnvironment::~UdevEnvironment() noexcept
{
    g_object_unref(testbed);
}

std::string mtf::UdevEnvironment::add_device(char const* subsystem,
                                             char const* name,
                                             char const* parent,
                                             std::initializer_list<char const*> attributes,
                                             std::initializer_list<char const*> properties)
{
    std::vector<char const*> attrib(attributes);
    std::vector<char const*> props(properties);

    attrib.push_back(nullptr);
    props.push_back(nullptr);

    gchar* syspath =  umockdev_testbed_add_devicev(testbed,
                                                   subsystem,
                                                   name,
                                                   parent,
                                                   const_cast<gchar**>(attrib.data()),
                                                   const_cast<gchar**>(props.data()));

    if (syspath == nullptr)
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to create mock udev device"));

    std::string retval(syspath);
    g_free(syspath);
    return retval;
}

namespace
{
void remove_subtree(UMockdevTestbed* testbed, mir::udev::Device const& device)
{
    auto ctx = std::make_shared<mir::udev::Context>();

    mir::udev::Enumerator children{ctx};
    children.match_parent(device);
    children.scan_devices();

    for (auto const& child : children)
    {
        // A udev device is always a child of itself(‽)
        if (child != device)
        {
            remove_subtree(testbed, child);
        }
    }
    umockdev_testbed_uevent(testbed, device.syspath(), "remove");
    umockdev_testbed_remove_device(testbed, device.syspath());
}
}

void mtf::UdevEnvironment::remove_device(std::string const& device_path)
{
    /* Because removing a device from umockdev will recursively remove
     * any children, if we want to have a REMOVED event sent (and we do),
     * we need to walk the tree of children and emit REMOVED events for them.
     *
     * Since we're already walking the tree, we might as well remove the
     * devices as we go.
     */
    auto ctx = std::make_shared<mir::udev::Context>();
    auto device_to_remove = ctx->device_from_syspath(device_path);

    remove_subtree(testbed, *device_to_remove);
}

void mtf::UdevEnvironment::emit_device_changed(std::string const& device_path)
{
    umockdev_testbed_uevent(testbed, device_path.c_str(), "change");
}

void mtf::UdevEnvironment::add_from_string(std::string_view device_description)
{
    using namespace std::string_literals;
    GError* err = nullptr;
    if (!umockdev_testbed_add_from_string(testbed, device_description.data(), &err))
    {
        BOOST_THROW_EXCEPTION((std::runtime_error{"Failed to create mock device from description:"s + err->message}));
    }
}

void mtf::UdevEnvironment::set_attribute_link(std::string const& devname, std::string const& name, std::string const& value)
{
    umockdev_testbed_set_attribute_link(testbed, devname.c_str(), name.c_str(), value.c_str());
}

void mtf::UdevEnvironment::add_standard_device(std::string const& name)
{
    auto const existing_devices =
        []()
        {
            std::unordered_set<std::string> devices;
            auto ctx = std::make_shared<mir::udev::Context>();
            mir::udev::Enumerator enumerator{ctx};

            enumerator.scan_devices();
            for (auto const& device : enumerator)
            {
                devices.insert(device.syspath());
            }

            return devices;
        }();

    auto descriptor_filename = recordings_path + "/" + name + ".umockdev";
    std::ifstream descriptor_file{descriptor_filename};
    if (!descriptor_file)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to open mock udev device recording: " + descriptor_filename));
    }

    // Devices recorded from the same machine share ancestors, which umockdev refuses to add twice
    std::string descriptor;
    std::string record;
    bool skip_record{false};
    for (std::string line; std::getline(descriptor_file, line);)
    {
        if (line.starts_with("P: "))
        {
            skip_record = existing_devices.contains("/sys" + line.substr(3));
        }
        record += line + '\n';
        if (line.empty())
        {
            if (!skip_record)
            {
                descriptor += record;
            }
            record.clear();
            skip_record = false;
        }
    }
    if (!skip_record)
    {
        descriptor += record;
    }

    GError* err = nullptr;
    if (!umockdev_testbed_add_from_string(testbed, descriptor.c_str(), &err))
    {
        BOOST_THROW_EXCEPTION(std::runtime_error(std::string("Failed to create mock udev device: ") +
                                                 err->message));
    }

    auto ioctls_filename = recordings_path + "/" + name + ".ioctl";
    struct stat sb{};
    if (stat(ioctls_filename.c_str(), &sb) == 0)
    {
        if (S_ISREG(sb.st_mode) || S_ISLNK(sb.st_mode))
        {
            if (!umockdev_testbed_load_ioctl(testbed, nullptr, ioctls_filename.c_str(), &err))
            {
                BOOST_THROW_EXCEPTION(std::runtime_error(std::string("Failed to load ioctl recording: ") +
                                                         err->message));
            }
        }
    }

    auto script_filename = recordings_path + "/" + name + ".script";
    if (stat(script_filename.c_str(), &sb) == 0)
    {
        if (S_ISREG(sb.st_mode) || S_ISLNK(sb.st_mode))
        {
            if (!umockdev_testbed_load_script(testbed, nullptr, script_filename.c_str(), &err))
            {
                BOOST_THROW_EXCEPTION(std::runtime_error(std::string("Failed to load device recording: ") +
                                                         err->message));
            }
        }
    }

    // Send an “ADDED” uevent for everything we've added. umockdev does not do this automatically
    auto ctx = std::make_shared<mir::udev::Context>();
    mir::udev::Enumerator enumerator{ctx};

    enumerator.scan_devices();
    for (auto const& device : enumerator)
    {
        if (existing_devices.count(device.syspath()) == 0)
        {
            umockdev_testbed_uevent(testbed, device.syspath(), "add");
        }
    }
}

void mtf::UdevEnvironment::load_device_ioctls(std::string const& name)
{
    auto ioctls_filename = recordings_path + "/" + name + ".ioctl";

    GError* err = nullptr;
    if (!umockdev_testbed_load_ioctl(testbed, nullptr, ioctls_filename.c_str(), &err))
    {
        BOOST_THROW_EXCEPTION(std::runtime_error(std::string("Failed to load ioctl recording: ") +
                                                 err->message));
    }
}

namespace
{
/// Quote binary data as umockdev's script format expects (see ScriptRunner.encode() in umockdev)
auto encode_script_data(std::span<std::byte const> data) -> std::string
{
    std::string quoted;
    for (auto const byte : data)
    {
        auto const c = static_cast<unsigned char>(byte);
        if (c < 32)
        {
            quoted += '^';
            quoted += static_cast<char>(c + 64);
        }
        else if (c == '^')
        {
            quoted += "^`";
        }
        else
        {
            quoted += static_cast<char>(c);
        }
    }
    return quoted;
}
}

auto mtf::UdevEnvironment::load_device_evemu(std::string const& name) -> std::string
{
    auto const evemu_filename = recordings_path + "/" + name + ".evemu";
    std::ifstream evemu_file{evemu_filename};
    if (!evemu_file)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to open evemu recording: " + evemu_filename));
    }

    // The recorded timestamps are replayed verbatim. Rebase them onto the current CLOCK_MONOTONIC
    // time (as libinput uses) so that timestamp-driven behaviour (debouncing, acceleration…) is realistic.
    timespec now{};
    if (clock_gettime(CLOCK_MONOTONIC, &now) < 0)
    {
        BOOST_THROW_EXCEPTION(std::system_error(errno, std::system_category(), "Failed to get replay timestamp"));
    }
    long long const now_us = now.tv_sec * 1'000'000LL + now.tv_nsec / 1'000;
    std::optional<long long> offset_us;

    // umockdev_testbed_load_evemu_events() writes each event to the device separately, so a reader can see a
    // partial frame (which libinput "fixes" by splitting it in two). The kernel only ever delivers whole frames,
    // so convert the recording into a umockdev script that writes each frame (up to SYN_REPORT) in one go.
    std::string script_filename = std::string{umockdev_testbed_get_root_dir(testbed)} + "/evemu-XXXXXX.script";
    int const fd = mkstemps(script_filename.data(), std::strlen(".script"));
    if (fd < 0)
    {
        BOOST_THROW_EXCEPTION(
            std::system_error(errno, std::system_category(), "Failed to create evemu replay script"));
    }
    close(fd);
    auto const remove_script = mir::raii::paired_calls(
        [] {}, [&] { unlink(script_filename.c_str()); });
    std::ofstream output{script_filename, std::ios::trunc};

    std::string device_node;
    std::vector<input_event> frame;
    std::optional<long long> previous_frame_us;
    auto const write_frame =
        [&]
        {
            if (frame.empty())
            {
                return;
            }
            auto const frame_us = static_cast<long long>(frame.front().input_event_sec) * 1'000'000 +
                static_cast<long long>(frame.front().input_event_usec);
            auto const delay_ms = previous_frame_us ? std::max(0LL, (frame_us - *previous_frame_us) / 1'000) : 0LL;
            previous_frame_us = frame_us;
            output << "r " << delay_ms << " " << encode_script_data(std::as_bytes(std::span{frame})) << '\n';
            frame.clear();
        };

    std::string_view constexpr device_prefix = "# device ";
    for (std::string line; std::getline(evemu_file, line);)
    {
        long long seconds{};
        long long microseconds{};
        unsigned type{};
        unsigned code{};
        int value{};
        if (line.starts_with(device_prefix) && device_node.empty())
        {
            device_node = line.substr(device_prefix.size());
        }
        else if (sscanf(line.c_str(), "E: %lld.%lld %x %x %d", &seconds, &microseconds, &type, &code, &value) == 5)
        {
            auto const timestamp_us = seconds * 1'000'000 + microseconds;
            if (!offset_us)
            {
                offset_us = now_us - timestamp_us;
            }
            auto const rebased_us = timestamp_us + *offset_us;

            input_event event{};
            event.input_event_sec = rebased_us / 1'000'000;
            event.input_event_usec = rebased_us % 1'000'000;
            event.type = static_cast<__u16>(type);
            event.code = static_cast<__u16>(code);
            event.value = value;
            frame.push_back(event);

            if (type == EV_SYN && code == SYN_REPORT)
            {
                write_frame();
            }
        }
        else if (!line.empty() && !line.starts_with('#'))
        {
            BOOST_THROW_EXCEPTION(
                std::runtime_error("Invalid line in evemu recording " + evemu_filename + ": " + line));
        }
    }
    write_frame();

    if (device_node.empty())
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("evemu recording has no \"# device\" header: " + evemu_filename));
    }

    output.close();
    if (!output)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to write evemu replay script: " + script_filename));
    }

    GError* err = nullptr;
    if (!umockdev_testbed_load_script(testbed, device_node.c_str(), script_filename.c_str(), &err))
    {
        BOOST_THROW_EXCEPTION(std::runtime_error(std::string("Failed to load evemu recording: ") +
                                                 err->message));
    }
    return device_node;
}

void mtf::UdevEnvironment::wait_for_device_script(std::string const& device_node)
{
    GError* err = nullptr;
    if (!umockdev_testbed_wait_script(testbed, device_node.c_str(), &err))
    {
        BOOST_THROW_EXCEPTION(std::runtime_error(std::string("Failed waiting for device script: ") +
                                                 err->message));
    }
}
