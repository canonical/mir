/*
 * Copyright © 2014 Canonical Ltd.
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
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_DEFAULT_EMERGENCY_CLEANUP_H_
#define MIR_DEFAULT_EMERGENCY_CLEANUP_H_

#include <mir/emergency_cleanup.h>

#include <memory>
#include <atomic>
#include <mutex>

namespace mir
{

class DefaultEmergencyCleanup : public EmergencyCleanup
{
public:
    void add(EmergencyCleanupHandler const& handler) override;
    void operator()() const override;

private:
    struct ListItem
    {
        EmergencyCleanupHandler handler;
        std::unique_ptr<ListItem> next;
    };

    ListItem* last_item();

    ListItem head;
    std::atomic<int> num_handlers{0};
    std::mutex handlers_mutex;
};

}

#endif
