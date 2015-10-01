/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "src/server/frontend/event_sink_factory.h"

#ifndef MIR_TEST_DOUBLES_NULL_EVENT_SINK_FACTORY_H_
#define MIR_TEST_DOUBLES_NULL_EVENT_SINK_FACTORY_H_

namespace mir
{
namespace test
{
namespace doubles
{
class NullEventSinkFactory : public frontend::EventSinkFactory
{
public:
    std::unique_ptr<frontend::EventSink>
       create_sink(std::shared_ptr<frontend::MessageSender> const&) override;
};
}
}
}

#endif //MIR_TEST_DOUBLES_NULL_EVENT_SINK_FACTORY_H_
