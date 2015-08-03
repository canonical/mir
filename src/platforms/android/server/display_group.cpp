/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "display_group.h"
#include "configurable_display_buffer.h"
#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mg = mir::graphics;
namespace mga = mir::graphics::android;
namespace geom = mir::geometry;

mga::DisplayGroup::DisplayGroup(
    std::shared_ptr<mga::DisplayDevice> const& device,
    std::unique_ptr<mga::ConfigurableDisplayBuffer> primary_buffer) :
    device(device),
    hotplugging(false)
{
    dbs.emplace(std::make_pair(mga::DisplayName::primary, std::move(primary_buffer)));
}

void mga::DisplayGroup::for_each_display_buffer(std::function<void(mg::DisplayBuffer&)> const& f)
{
    std::unique_lock<decltype(guard)> lk(guard);
    for(auto const& db : dbs)
        if (db.second->power_mode() != mir_power_mode_off)
            f(*db.second);
}

void mga::DisplayGroup::add(DisplayName name, std::unique_ptr<ConfigurableDisplayBuffer> buffer)
{
    std::unique_lock<decltype(guard)> lk(guard);
    dbs.emplace(std::make_pair(name, std::move(buffer)));
}

void mga::DisplayGroup::remove(DisplayName name)
{
    if (name == mga::DisplayName::primary)
        BOOST_THROW_EXCEPTION(std::logic_error("cannot remove primary display"));

    std::unique_lock<decltype(guard)> lk(guard);
    auto it = dbs.find(name);
    if (it != dbs.end())
        dbs.erase(it);
}

bool mga::DisplayGroup::display_present(DisplayName name) const
{
    std::unique_lock<decltype(guard)> lk(guard);
    return (dbs.end() != dbs.find(name));
}

void mga::DisplayGroup::configure(
    DisplayName name, MirPowerMode mode, MirOrientation orientation, geom::Displacement offset)
{
    std::unique_lock<decltype(guard)> lk(guard);
    auto it = dbs.find(name);
    if (it != dbs.end())
        it->second->configure(mode, orientation, offset);
}

void mga::DisplayGroup::post()
{
    std::list<DisplayContents> contents;
    {
        std::unique_lock<decltype(guard)> lk(guard);
        for(auto const& db : dbs)
            contents.emplace_back(db.second->contents());
        hotplugging = false;
    }

    try
    {
        device->commit(contents);
    }
    catch (std::runtime_error& e)
    {
        std::unique_lock<decltype(guard)> lk(guard);
        if (!hotplugging)
            throw e;
    }
}

std::chrono::milliseconds mga::DisplayGroup::recommended_sleep() const
{
    return device->recommended_sleep();
}

void mga::DisplayGroup::hotplug_occurred()
{
    std::unique_lock<decltype(guard)> lk(guard);
    hotplugging = true;
}
