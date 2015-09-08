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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_TEST_DISPLAY_CONFIG_MATCHERS_H_
#define MIR_TEST_DISPLAY_CONFIG_MATCHERS_H_

#include "mir_toolkit/client_types.h"

#include <gmock/gmock.h>

//avoid a valgrind complaint by defining printer for this type
static void PrintTo(MirDisplayConfiguration const&, ::std::ostream*) __attribute__ ((unused));
void PrintTo(MirDisplayConfiguration const&, ::std::ostream*)
{
}

namespace mir
{
namespace protobuf
{

class DisplayConfiguration;
class Connection;
static void PrintTo(mir::protobuf::DisplayConfiguration const&, ::std::ostream*) __attribute__ ((unused));
void PrintTo(mir::protobuf::DisplayConfiguration const&, ::std::ostream*) {}

static void PrintTo(mir::protobuf::Connection const&, ::std::ostream*) __attribute__ ((unused));
void PrintTo(mir::protobuf::Connection const&, ::std::ostream*)
{
}

}

namespace graphics
{
class DisplayConfiguration;
}

namespace test
{

bool compare_display_configurations(graphics::DisplayConfiguration const& display_config1,
                                    graphics::DisplayConfiguration const& display_config2);

bool compare_display_configurations(MirDisplayConfiguration const& client_config,
                                    graphics::DisplayConfiguration const& display_config);

bool compare_display_configurations(protobuf::DisplayConfiguration const& protobuf_config,
                                    graphics::DisplayConfiguration const& display_config);

bool compare_display_configurations(MirDisplayConfiguration const& client_config,
                                    protobuf::DisplayConfiguration const& protobuf_config);

bool compare_display_configurations(graphics::DisplayConfiguration const& display_config1,
                                    MirDisplayConfiguration const* display_config2);

MATCHER_P(DisplayConfigMatches, config, "")
{
    return compare_display_configurations(arg, config);
}

}
}

#endif /* MIR_TEST_DISPLAY_CONFIG_MATCHERS_H_ */
