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
 * Authored by: Andreas Pokorny <andreas.pokorny@gmail.com>
 */

#include "src/server/input/android/input_reader_dispatchable.h"

#include "mir/test/doubles/mock_event_hub.h"
#include "mir/test/doubles/mock_input_reader.h"
#include "InputReader.h"

namespace mtd = mir::test::doubles;
namespace mia = mir::input::android;
namespace droidinput = android;
namespace
{
struct TestInputReaderDispatchable : public ::testing::Test
{
    std::shared_ptr<mtd::MockInputReader> reader = std::make_shared<mtd::MockInputReader>();
    std::shared_ptr<mtd::MockEventHub> hub = std::make_shared<mtd::MockEventHub>();
    mia::InputReaderDispatchable dispatchable{hub, reader};

};
}

TEST_F(TestInputReaderDispatchable,loops_on_readable)
{
    EXPECT_CALL(*reader, loopOnce());

    EXPECT_TRUE(dispatchable.dispatch(mir::dispatch::FdEvent::readable));
}

TEST_F(TestInputReaderDispatchable, false_on_error)
{
    EXPECT_CALL(*reader, loopOnce()).Times(0);

    EXPECT_FALSE(dispatchable.dispatch(mir::dispatch::FdEvent::error));
}
