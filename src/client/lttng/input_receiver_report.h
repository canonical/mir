/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_CLIENT_LTTNG_INPUT_RECEIVER_REPORT_H_
#define MIR_CLIENT_LTTNG_INPUT_RECEIVER_REPORT_H_

#include "mir/input/input_receiver_report.h"
#include "client_tracepoint_provider.h"

namespace mir
{
namespace client
{
namespace lttng
{

class InputReceiverReport : public input::receiver::InputReceiverReport
{
public:
    void received_event(MirEvent const& event) override;
private:
    void report(MirKeyEvent const& event) const;
    void report(MirMotionEvent const& event) const;
    ClientTracepointProvider tp_provider;
};

}
}
}

#endif /* MIR_CLIENT_LTTNG_INPUT_RECEIVER_REPORT_H_ */
