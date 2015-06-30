/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_NULL_DISPLAY_SYNC_GROUP_H_
#define MIR_TEST_DOUBLES_NULL_DISPLAY_SYNC_GROUP_H_

#include "mir/graphics/display.h"
#include "mir/geometry/size.h"
#include "mir/test/doubles/null_display_buffer.h"
#include "mir/test/doubles/stub_display_buffer.h"
#include <thread>

namespace mir
{
namespace test
{
namespace doubles
{

struct StubDisplaySyncGroup : graphics::DisplaySyncGroup
{
public:
    StubDisplaySyncGroup(std::vector<geometry::Rectangle> const& output_rects)
        : output_rects{output_rects}
    {
        for (auto const& output_rect : output_rects)
            display_buffers.emplace_back(output_rect);
    }
    StubDisplaySyncGroup(geometry::Size sz) : StubDisplaySyncGroup({{{0,0}, sz}}) {}

    void for_each_display_buffer(std::function<void(graphics::DisplayBuffer&)> const& f) override
    {
        for (auto& db : display_buffers)
            f(db);
    }

    void post() override
    {
        /* yield() is needed to ensure reasonable runtime under valgrind for some tests */
        std::this_thread::yield();
    }

    std::chrono::milliseconds recommended_sleep() const override
    {
        return std::chrono::milliseconds::zero();
    }

private:
    std::vector<geometry::Rectangle> const output_rects;
    std::vector<StubDisplayBuffer> display_buffers;
};

struct NullDisplaySyncGroup : graphics::DisplaySyncGroup
{
    void for_each_display_buffer(std::function<void(graphics::DisplayBuffer&)> const& f) override
    {
        f(db);
    }
    virtual void post() override
    {
        /* yield() is needed to ensure reasonable runtime under valgrind for some tests */
        std::this_thread::yield();
    }

    std::chrono::milliseconds recommended_sleep() const override
    {
        return std::chrono::milliseconds::zero();
    }

    NullDisplayBuffer db;
};

}
}
}

#endif /* MIR_TEST_DOUBLES_NULL_DISPLAY_SYNC_GROUP_H_ */
