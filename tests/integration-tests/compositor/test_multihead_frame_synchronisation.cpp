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

#include "src/server/compositor/multi_threaded_compositor.h"
#include "src/server/report/null_report_factory.h"
#include "src/server/scene/surface_stack.h"

#include <mir/compositor/display_buffer_compositor.h>
#include <mir/compositor/display_buffer_compositor_factory.h>
#include <mir/compositor/display_listener.h>
#include <mir/compositor/scene_element.h>
#include <mir/compositor/stream.h>
#include <mir/executor.h>
#include <mir/graphics/renderable.h>
#include <mir/scene/basic_surface.h>
#include <mir/synchronised.h>
#include <mir/test/doubles/fake_display_configuration_observer_registrar.h>
#include <mir/test/doubles/mock_display.h>
#include <mir/test/doubles/null_display_sync_group.h>
#include <mir/test/doubles/stub_buffer.h>
#include <mir/test/doubles/stub_cursor.h>
#include <mir/test/fake_shared.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <condition_variable>
#include <future>
#include <optional>
#include <vector>

namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace mi = mir::input;
namespace mr = mir::report;
namespace ms = mir::scene;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;
namespace geom = mir::geometry;

using namespace testing;
using namespace std::chrono_literals;

namespace
{
auto const timeout = 2s;

/* A DisplaySyncGroup whose post() blocks until the test releases it.
 *
 * This lets the test act as each output's vsync, so outputs that post at
 * different rates (or phases) can be interleaved deterministically.
 */
class GatedDisplaySyncGroup : public mtd::StubDisplaySyncGroup
{
public:
    explicit GatedDisplaySyncGroup(geom::Rectangle const& output)
        : mtd::StubDisplaySyncGroup{std::vector<geom::Rectangle>{output}}
    {
    }

    bool post() override
    {
        auto locked = gate.lock();
        auto const this_post = ++locked->posts_started;
        cv.notify_all();
        locked.wait(cv, [&]() { return locked->free_running || locked->posts_released >= this_post; });
        return true;
    }

    void release_post()
    {
        ++gate.lock()->posts_released;
        cv.notify_all();
    }

    void run_freely()
    {
        gate.lock()->free_running = true;
        cv.notify_all();
    }

    auto wait_for_posts_started(unsigned count) -> bool
    {
        auto locked = gate.lock();
        return locked.wait_for(cv, timeout, [&]() { return locked->posts_started >= count; });
    }

private:
    struct Gate
    {
        unsigned posts_started{0};
        unsigned posts_released{0};
        bool free_running{false};
    };

    mir::Synchronised<Gate> gate;
    std::condition_variable cv;
};

class CompositionRecord
{
public:
    void composited(mg::BufferID id)
    {
        *last.lock() = id;
        cv.notify_all();
    }

    auto last_composited() -> std::optional<mg::BufferID>
    {
        return *last.lock();
    }

    auto wait_for_composition_of(mg::BufferID id) -> bool
    {
        auto locked = last.lock();
        return locked.wait_for(cv, timeout, [&]() { return *locked == id; });
    }

private:
    mir::Synchronised<std::optional<mg::BufferID>> last;
    std::condition_variable cv;
};

/* Claims the buffer of every scene element, as a real compositor does when
 * rendering, and records which buffer each output last composited.
 */
class StubDisplayBufferCompositorFactory : public mc::DisplayBufferCompositorFactory
{
public:
    StubDisplayBufferCompositorFactory(
        std::function<CompositionRecord&(mg::DisplaySink&)> record_for)
        : record_for{std::move(record_for)}
    {
    }

    auto create_compositor_for(mg::DisplaySink& sink) -> std::unique_ptr<mc::DisplayBufferCompositor> override
    {
        struct RecordingCompositor : mc::DisplayBufferCompositor
        {
            explicit RecordingCompositor(CompositionRecord& record) : record{record} {}

            bool composite(mc::SceneElementSequence&& elements, mc::CompositionMode) override
            {
                for (auto const& element : elements)
                    record.composited(element->renderable()->buffer()->id());
                return true;
            }

            CompositionRecord& record;
        };

        return std::make_unique<RecordingCompositor>(record_for(sink));
    }

private:
    std::function<CompositionRecord&(mg::DisplaySink&)> const record_for;
};

struct StubDisplayListener : mc::DisplayListener
{
    void add_display(geom::Rectangle const&) override {}
    void remove_display(geom::Rectangle const&) override {}
};

struct MultiheadFrameSynchronisation : Test
{
    MultiheadFrameSynchronisation()
    {
        ON_CALL(display, for_each_display_sync_group(_))
            .WillByDefault(
                [this](auto const& f)
                {
                    f(fast_group);
                    f(slow_group);
                });
    }

    ~MultiheadFrameSynchronisation()
    {
        // Let any compositor thread blocked in post() finish so the compositor can stop.
        fast_group.run_freely();
        slow_group.run_freely();
    }

    void submit(std::shared_ptr<mg::Buffer> const& buffer)
    {
        stream->submit_buffer(buffer, buffer->size(), {{0, 0}, geom::SizeD{buffer->size()}});
        wait_for_damage_notifications();
    }

    // Surface damage reaches the compositor via mir::linearising_executor; drain it so each
    // submission has scheduled compositing before the test releases any output's vsync.
    static void wait_for_damage_notifications()
    {
        std::promise<void> drained;
        mir::linearising_executor.spawn([&drained]() { drained.set_value(); });
        ASSERT_THAT(drained.get_future().wait_for(timeout), Eq(std::future_status::ready));
    }

    geom::Rectangle const fast_output_area{{0, 0}, {100, 100}};
    geom::Rectangle const slow_output_area{{100, 0}, {100, 100}};

    GatedDisplaySyncGroup fast_group{fast_output_area};
    GatedDisplaySyncGroup slow_group{slow_output_area};
    NiceMock<mtd::MockDisplay> display;

    CompositionRecord fast_output;
    CompositionRecord slow_output;
    StubDisplayBufferCompositorFactory dbc_factory{
        [this](mg::DisplaySink& sink) -> CompositionRecord&
        {
            return sink.view_area() == fast_output_area ? fast_output : slow_output;
        }};

    std::shared_ptr<ms::SceneReport> const scene_report{mr::null_scene_report()};
    ms::SurfaceStack stack{scene_report};
    StubDisplayListener display_listener;
    std::shared_ptr<mc::CompositorReport> const compositor_report{mr::null_compositor_report()};
    std::shared_ptr<mtd::StubCursor> const cursor{std::make_shared<mtd::StubCursor>()};

    std::shared_ptr<mc::Stream> const stream{std::make_shared<mc::Stream>()};
    std::shared_ptr<ms::BasicSurface> const surface{std::make_shared<ms::BasicSurface>(
        std::string("spanning both outputs"),
        geom::Rectangle{{0, 0}, {200, 100}},
        mir_pointer_unconfined,
        std::list<ms::StreamInfo>{{stream, {0, 0}}},
        std::shared_ptr<mg::CursorImage>{},
        scene_report,
        std::make_shared<mtd::FakeDisplayConfigurationObserverRegistrar>())};

    std::shared_ptr<mg::Buffer> const buffer_a{std::make_shared<mtd::StubBuffer>(geom::Size{200, 100})};
    std::shared_ptr<mg::Buffer> const buffer_b{std::make_shared<mtd::StubBuffer>(geom::Size{200, 100})};
    std::shared_ptr<mg::Buffer> const buffer_c{std::make_shared<mtd::StubBuffer>(geom::Size{200, 100})};

    // Declared last so it is destroyed (and its threads stopped) first
    mc::MultiThreadedCompositor compositor{
        mt::fake_shared(display),
        mt::fake_shared(dbc_factory),
        mt::fake_shared(stack),
        mt::fake_shared(display_listener),
        compositor_report,
        cursor,
        std::chrono::milliseconds{-1},
        true};
};
}

// Regression test for https://github.com/canonical/mir/issues/5325
TEST_F(MultiheadFrameSynchronisation, every_output_composites_the_latest_buffer_when_submissions_outpace_a_slow_output)
{
    submit(buffer_a);
    stack.add_surface(surface, mi::InputReceptionMode::normal);
    compositor.start();

    // Both outputs composite A, then wait for their vsync
    ASSERT_TRUE(fast_group.wait_for_posts_started(1));
    ASSERT_TRUE(slow_group.wait_for_posts_started(1));

    // B arrives; only the fast output's vsync comes in, so it composites B
    submit(buffer_b);
    fast_group.release_post();
    ASSERT_TRUE(fast_group.wait_for_posts_started(2));
    ASSERT_THAT(fast_output.last_composited(), Eq(buffer_b->id()));

    // C (the client's final frame) arrives while both outputs wait for vsync;
    // the slow output's vsync comes in first
    submit(buffer_c);
    slow_group.release_post();
    ASSERT_TRUE(slow_group.wait_for_posts_started(2));

    fast_group.run_freely();
    slow_group.run_freely();

    EXPECT_TRUE(fast_output.wait_for_composition_of(buffer_c->id()));
    EXPECT_TRUE(slow_output.wait_for_composition_of(buffer_c->id()))
        << "slow output is stuck on "
        << (slow_output.last_composited() == buffer_b->id() ? "buffer B" : "an unexpected buffer");
}
