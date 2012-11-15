/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */
#include "src/input/android/android_input_lexicon.h"
// Is this the right place for this header?
#include "mir/input/event.h"

#include <androidfw/Input.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mi = mir::input;
namespace mia = mir::input::android;

namespace
{

class AndroidInputLexiconSetup : public testing::Test
{
public:
    mia::InputLexicon lexicon;
}

}

TEST_F(AndroidInputLexiconSetup, translates_key_events)
{
    using namespace ::testing;
    auto android_key_ev = new android::KeyEvent();
    
    const int32_t device_id = 1;
    const int32_t source_id = 2;
    const int32_t action = 3;
    const int32_t flags = 4;
    const int32_t key_code = 5;
    const int32_t scan_code = 6;
    const int32_t meta_state = 7;
    const int32_t repeat_count = 8;
    const nsecs_t down_time = 9;
    const nsecs_t event_time = 10;
    
    android_key_ev->initialize(device_id, source_id, action, flags, key_code,
			       scan_code, meta_state, repeat_count,
			       down_time, event_time);
    
    MirEvent mir_ev;
    lexicon.translate(android_key_ev, &mir_ev);
    
    // Common event properties
    EXPECT_EQ(device_id, mir_ev.device_id);
    EXPECT_EQ(source_id, mir_ev.source_id);
    EXPECT_EQ(action, mir_ev.action);
    EXPECT_EQ(flags, mir_ev.flags);
    EXPECT_EQ(meta_state, mir_ev.meta_state);
    
    // Key event specific properties
    EXPECT_EQ(mir_ev.type, MIR_EVENT_TYPE_KEY);
    EXPECT_EQ(mir_ev.details.key.key_code, key_code);
    EXPECT_EQ(mir_ev.details.key.scan_code, scan_code);
    EXPECT_EQ(mir_ev.details.key.repeat_count, repeat_count);
    EXPECT_EQ(mir_ev.details.key.down_time, down_time);
    EXPECT_EQ(mir_ev.details.key.event_time, event_time);
    // What is this flag and where does it come from?
    EXPECT_EQ(mir_ev.details.key.is_system_key, false);
    
}

