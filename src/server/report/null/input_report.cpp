/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "input_report.h"

namespace mrn = mir::report::null;

void mrn::InputReport::received_event_from_kernel(int64_t /* when */, int /* type */, int /* code */, int /* value */)
{
}

void mrn::InputReport::published_key_event(int /* dest_fd */, uint32_t /* seq_id */, int64_t /* event_time */)
{
}

void mrn::InputReport::published_motion_event(int /* dest_fd */, uint32_t /* seq_id */, int64_t /* event_time */)
{
}

void mrn::InputReport::received_event_finished_signal(int /* src_fd */, uint32_t /* seq_id */)
{
}

void mrn::InputReport::opened_input_device(char const* /* name */, char const* /* platform */)
{
}

void mrn::InputReport::failed_to_open_input_device(char const* /* name */, char const* /* platform */)
{
}
