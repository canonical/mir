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
 * Authored by: Chase Douglas <chase.douglas@canonical.com>
 */

#include "mir/input/evemu/device.h"

#include <boost/filesystem.hpp>

#include <evemu.h>

#include <cstdio>
#include <stdexcept>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

namespace mi = mir::input;

namespace {

struct EvemuDeleter
{
    void operator() (struct evemu_device*& device)
    {
        evemu_delete(device);
    }
};

}

mi::evemu::EvemuDevice::EvemuDevice(
    const std::string& path,
    EventHandler* event_handler) : LogicalDevice(event_handler),
                                   simultaneous_instances{1},
    buttons(static_cast<size_t>(KEY_MAX), false),
    position_info{Mode::none}    
{
    std::unique_ptr<evemu_device, EvemuDeleter> evemu(evemu_new(NULL), EvemuDeleter());

    boost::filesystem::file_status status = boost::filesystem::status(path);
    if (status.type() == boost::filesystem::regular_file)
    {
        std::FILE *file = fopen(path.c_str(), "r");
        if (!file)
            throw std::runtime_error("Failed to open evemu file");

        if (evemu_read(evemu.get(), file) < 0)
        	fclose(file),
            throw std::runtime_error("Failed to read evemu parameters from file");
        fclose(file);
    }
    /* FIXME: Need test for evdev device nodes before uncommenting */
    /*else if (status.type() == boost::filesystem::character_file)
      {
      int fd = open(path.c_str(), O_RDONLY);
      if (fd < 0)
      throw std::runtime_error("Failed to open evdev node");

      if (evemu_extract(evemu.get(), fd) < 0)
      throw std::runtime_error("Failed to extract evdev node evemu parameters");
      }*/ else
        throw std::runtime_error("Device path is not a file nor a character device");

    name = evemu_get_name(evemu.get());

    if (evemu_has_event(evemu.get(), EV_ABS, ABS_MT_SLOT))
    {
        simultaneous_instances =
                evemu_get_abs_maximum(evemu.get(), ABS_MT_SLOT) - evemu_get_abs_minimum(evemu.get(), ABS_MT_SLOT) + 1;
    }

    for (int b = 0; b <= KEY_MAX; ++b)
    {
        switch (b)
        {
            case BTN_TOUCH:
                continue;
            default:
                break;
        }

        if (evemu_has_event(evemu.get(), EV_KEY, b))
            buttons[b] = 1;
    }

    if (evemu_has_event(evemu.get(), EV_ABS, ABS_MT_POSITION_X) || evemu_has_event(evemu.get(), EV_ABS, ABS_X))
        position_info.mode = Mode::absolute;
    else if (evemu_has_event(evemu.get(), EV_REL, REL_X))
        position_info.mode = Mode::relative;
}

mi::EventProducer::State mi::evemu::EvemuDevice::current_state() const
{
    return mi::EventProducer::State::stopped;
}

void mi::evemu::EvemuDevice::start()
{
    // TODO
}

void mi::evemu::EvemuDevice::stop()
{
    // TODO
}

const std::string& mi::evemu::EvemuDevice::get_name() const
{
    return name;
}
    
int mi::evemu::EvemuDevice::get_simultaneous_instances() const
{
    return simultaneous_instances;
}

/* FIXME: Reenable once under test.
bool mi::evemu::EvemuDevice::is_button_supported(const Button& button) const
{
    return buttons[button];
}
*/

const mi::PositionInfo& mi::evemu::EvemuDevice::get_position_info() const
{
    return position_info;
}

const std::map<mi::AxisType, mi::Axis>& mi::evemu::EvemuDevice::get_axes() const
{
    return axes;
}
