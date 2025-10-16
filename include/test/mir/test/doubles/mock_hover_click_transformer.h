/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
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

#ifndef MIR_TEST_DOUBLES_MOCK_HOVER_CLICK_TRANSFORMER_H
#define MIR_TEST_DOUBLES_MOCK_HOVER_CLICK_TRANSFORMER_H

#include <mir/shell/hover_click_transformer.h>

#include <gmock/gmock.h>

namespace mir::test::doubles
{
class MockHoverClickTransformer : public mir::shell::HoverClickTransformer
{
public:
    MOCK_METHOD(void, hover_duration, (std::chrono::milliseconds delay), (override));
    MOCK_METHOD(void, cancel_displacement_threshold, (int displacement), (override));
    MOCK_METHOD(void, reclick_displacement_threshold, (int displacement), (override));
    MOCK_METHOD(void, on_hover_start, (std::function<void()>&& on_hover_start), (override));
    MOCK_METHOD(void, on_hover_cancel, (std::function<void()>&& on_hover_cancelled), (override));
    MOCK_METHOD(void, on_click_dispatched, (std::function<void()>&& on_click_dispatched), (override));
    MOCK_METHOD(bool, transform_input_event, (EventDispatcher const&, mir::input::EventBuilder*, MirEvent const&), (override));
};
}

#endif // MIR_TEST_DOUBLES_MOCK_HOVER_CLICK_TRANSFORMER_H
