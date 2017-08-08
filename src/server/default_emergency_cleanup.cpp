/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "default_emergency_cleanup.h"

void mir::DefaultEmergencyCleanup::add(EmergencyCleanupHandler const& handler)
{
    std::lock_guard<std::mutex> lock{handlers_mutex};

    last_item()->next.reset(
        new ListItem{std::make_shared<EmergencyCleanupHandler>(handler), nullptr});
    ++num_handlers;
}

void mir::DefaultEmergencyCleanup::add(ModuleEmergencyCleanupHandler handler)
{
    std::lock_guard<std::mutex> lock{handlers_mutex};

    last_item()->next.reset(new ListItem{std::move(handler), nullptr});
    ++num_handlers;
}

void mir::DefaultEmergencyCleanup::operator()() const
{
    int handlers_left = num_handlers;
    ListItem const* item = &head;
    while (handlers_left-- > 0)
    {
        item = item->next.get();
        (*item->handler)();
    }
}

mir::DefaultEmergencyCleanup::ListItem* mir::DefaultEmergencyCleanup::last_item()
{
    ListItem* item = &head;
    while (item->next)
        item = item->next.get();
    return item;
}
