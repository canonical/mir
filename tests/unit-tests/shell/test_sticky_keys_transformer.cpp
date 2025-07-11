/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "src/server/shell/basic_sticky_keys_transformer.h"
#include "src/server/input/default_event_builder.h"
#include "mir/test/doubles/advanceable_clock.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace testing;
namespace msh = mir::shell;
namespace mi = mir::input;
namespace mt = mir::time;
namespace mtd = mir::test::doubles;

class TestStickyKeysTransformer : public Test
{
public:
    TestStickyKeysTransformer() : event_builder(0, clock)
    {
    }

    msh::BasicStickyKeysTransformer transformer = msh::BasicStickyKeysTransformer();
    std::function<void(std::shared_ptr<MirEvent> const& event)> dispatch
        = [](std::shared_ptr<MirEvent> const&){};
    std::shared_ptr<mt::Clock> clock = std::make_shared<mtd::AdvanceableClock>();
    mi::DefaultEventBuilder event_builder;
};

