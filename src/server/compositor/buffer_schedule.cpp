/*
 * Copyright © 2015 Canonical Ltd.
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
 */

#include "buffer_schedule.h"
namespace mg = mir::graphics;
namespace mc = mir::compositor;
namespace mf = mir::frontend;

mc::BufferSchedule::BufferSchedule(mf::BufferStreamId id, std::shared_ptr<mf::EventSink> const& sink)
{
    (void) id; (void) sink;
}

void mc::BufferSchedule::add_buffer(std::unique_ptr<mg::Buffer> buffer)
{
    (void) buffer;
}

void mc::BufferSchedule::remove_buffer(mg::BufferID id)
{
    (void) id;
}

void mc::BufferSchedule::schedule_buffer(mg::BufferID id)
{
    (void) id;
}

std::shared_ptr<mg::Buffer> mc::BufferSchedule::lock_compositor_buffer(compositor::CompositorID id)
{
    (void) id; return nullptr;
}
