/*
 * Copyright © 2017 Canonical Ltd.
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

#include <mir_toolkit/extensions/drag_and_drop.h>
#include <mir_toolkit/mir_blob.h>

#include <mir_test_framework/connected_client_with_a_surface.h>
#include <mir/test/signal.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace std::chrono_literals;
using namespace testing;
using mir::test::Signal;

namespace
{
struct DragAndDrop : mir_test_framework::ConnectedClientWithASurface
{
    MirDragAndDropV1 const* dnd = nullptr;
    Signal initiated;

    MirCookie* const cookie = nullptr; // TODO get a valid cookie

    void signal_when_initiated(MirWindow* window, MirEvent const* event);
    static void signal_when_initiated(MirWindow* window, MirEvent const* event, void* context);

    void SetUp() override
    {
        mir_test_framework::ConnectedClientWithASurface::SetUp();

        dnd = mir_drag_and_drop_v1(connection);
        mir_window_set_event_handler(window, &signal_when_initiated, this);

        ASSERT_THAT(dnd, Ne(nullptr));
    }
};

void DragAndDrop::signal_when_initiated(MirWindow* window, MirEvent const* event, void* context)
{
    static_cast<DragAndDrop*>(context)->signal_when_initiated(window, event);
}

void DragAndDrop::signal_when_initiated(MirWindow* /*window*/, MirEvent const* event)
{
    if (mir_event_get_type(event) != mir_event_type_window)
        return;

    if (auto handle = dnd->start_drag(mir_event_get_window_event(event)))
    {
        initiated.raise();
        mir_blob_release(handle);
    }
}
}

TEST_F(DragAndDrop, DISABLED_can_initiate)
{
    dnd->begin_drag_and_drop(window, cookie);

    EXPECT_TRUE(initiated.wait_for(30s));
}
