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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */


#ifndef MIR_INPUT_NULL_INPUT_REPORT_H_
#define MIR_INPUT_NULL_INPUT_REPORT_H_

#include "mir/input/input_report.h"

namespace mir
{
namespace input
{

class NullInputReport : public InputReport
{
public:
    NullInputReport() = default;
    virtual ~NullInputReport() noexcept(true) = default;

    void received_event_from_kernel(int64_t when, int type, int code, int value);

    void published_key_event(int dest_fd, uint32_t seq_id, int64_t event_time);
    void published_motion_event(int dest_fd, uint32_t seq_id, int64_t event_time);

    void received_event_finished_signal(int src_fd, uint32_t seq_id);
};

}
}

#endif /* MIR_INPUT_NULL_INPUT_REPORT_H_ */
