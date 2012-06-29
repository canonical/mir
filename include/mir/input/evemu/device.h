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

#ifndef MIR_INPUT_EVEMU_DEVICE_H_
#define MIR_INPUT_EVEMU_DEVICE_H_

#include "mir/input/logical_device.h"
#include "mir/input/position_info.h"

#include <map>
#include <vector>

namespace mir
{
namespace input
{
namespace evemu
{
/**
 * An Evemu-derived logical device
 *
 * Evemu is an abstraction layer for Linux Evdev devices. It operates on Evemu device property files or on evdev input
 * nodes. This class may be used to instantiate a mir logical input device from either property files or input nodes.
 */
class EvemuDevice : public LogicalDevice {
 public:
    /**
     * Construct a new logical device from an Evemu property file or an evdev input node
     *
     * @param[in] path Path to the evemu property file or evdev input node
     */
    EvemuDevice(const std::string& path, EventHandler* event_handler);

    // From EventProducer
    virtual EventProducer::State current_state() const;
    virtual void start();
    virtual void stop();
    
    // From LogicalDevice
    virtual const std::string& get_name() const;
    
    virtual int get_simultaneous_instances() const;

    // FIXME: Reenable once under test.
    // virtual bool is_button_supported(const Button& button) const;

    virtual const mir::input::PositionInfo& get_position_info() const;

    virtual const std::map<AxisType,Axis>& get_axes() const;

    EvemuDevice(const EvemuDevice&) = delete;
    EvemuDevice& operator=(const EvemuDevice&) = delete;

 private:
    std::string name;
    int simultaneous_instances;
    std::vector<bool> buttons;
    mir::input::PositionInfo position_info;
    std::map<AxisType, Axis> axes;
};
    
} // evemu
} // input
} // mir

#endif // MIR_INPUT_EVEMU_DEVICE_H_
