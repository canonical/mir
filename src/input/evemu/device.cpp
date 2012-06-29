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

namespace mir
{
namespace input
{
namespace evemu
{

#include <evemu.h>

}
}
}

#include <cstdio>
#include <stdexcept>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

namespace mi = mir::input;
namespace mie = mir::input::evemu;

namespace {

struct EvemuDeleter
{
    void operator() (struct mie::evemu_device*& device)
    {
        mie::evemu_delete(device);
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
    std::unique_ptr<struct mie::evemu_device, EvemuDeleter> evemu(mie::evemu_new(NULL), EvemuDeleter());

    boost::filesystem::file_status status = boost::filesystem::status(path);
    if (status.type() == boost::filesystem::regular_file)
    {
        std::FILE *file = fopen(path.c_str(), "r");
        if (!file)
            throw std::runtime_error("Failed to open evemu file");

        if (mie::evemu_read(evemu.get(), file) < 0)
            throw std::runtime_error("Failed to read evemu parameters from file");
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

    name = mie::evemu_get_name(evemu.get());

    if (mie::evemu_has_event(evemu.get(), EV_ABS, ABS_MT_SLOT))
    {
        simultaneous_instances =
                mie::evemu_get_abs_maximum(evemu.get(), ABS_MT_SLOT) - mie::evemu_get_abs_minimum(evemu.get(), ABS_MT_SLOT) + 1;
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

        if (mie::evemu_has_event(evemu.get(), EV_KEY, b))
            buttons[b] = 1;
    }

    if (mie::evemu_has_event(evemu.get(), EV_ABS, ABS_MT_POSITION_X) || mie::evemu_has_event(evemu.get(), EV_ABS, ABS_X))
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

bool mi::evemu::EvemuDevice::is_button_supported(const Button& button) const
{
    return buttons[button];
}

const mi::PositionInfo& mi::evemu::EvemuDevice::get_position_info() const
{
    return position_info;
}

bool mi::evemu::EvemuDevice::has_axis_type(AxisType axisType) const
{
    auto it = axes.find(axisType);
    return it != axes.end();
}

const mi::Axis& mi::evemu::EvemuDevice::get_axis_for_type(mi::AxisType axisType) const
{
    auto it = axes.find(axisType);
    if (it == axes.end())
        throw NoAxisForTypeException();
        
    return it->second;
}
