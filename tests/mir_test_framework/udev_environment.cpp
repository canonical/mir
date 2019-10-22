/*
 * Copyright © 2013 Canonical Ltd.
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
 *
 * Authored by:
 * Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "mir_test_framework/udev_environment.h"
#include "mir_test_framework/executable_path.h"

#include "mir/udev/wrapper.h"

#include <umockdev.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <stdexcept>
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
    if (!getenv("LD_PRELOAD"))
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
    GError* err = nullptr;
    if (!umockdev_testbed_add_from_file(testbed, descriptor_filename.c_str(), &err))
    {
        BOOST_THROW_EXCEPTION(std::runtime_error(std::string("Failed to create mock udev device: ") +
                                                 err->message));
    }
    
    auto ioctls_filename = recordings_path + "/" + name + ".ioctl";
    struct stat sb;
    if (stat(ioctls_filename.c_str(), &sb) == 0)
    {
        if (S_ISREG(sb.st_mode) || S_ISLNK(sb.st_mode))
        {
            if (!umockdev_testbed_load_ioctl(testbed, NULL, ioctls_filename.c_str(), &err))
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
            if (!umockdev_testbed_load_script(testbed, NULL, script_filename.c_str(), &err))
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
    if (!umockdev_testbed_load_ioctl(testbed, NULL, ioctls_filename.c_str(), &err))
    {
        BOOST_THROW_EXCEPTION(std::runtime_error(std::string("Failed to load ioctl recording: ") +
                                                 err->message));
    }
}

void mtf::UdevEnvironment::load_device_evemu(std::string const& name)
{
    auto evemu_filename = recordings_path + "/" + name + ".evemu";

    GError* err = nullptr;
    if (!umockdev_testbed_load_evemu_events(testbed, NULL, evemu_filename.c_str(), &err))
    {
        BOOST_THROW_EXCEPTION(std::runtime_error(std::string("Failed to load evemu recording: ") +
                                                 err->message));
    }
}
