
#ifndef TESTS_ACCEPTANCE_TESTS_BUFFER_STREAM_ARRANGEMENT_H_
#define TESTS_ACCEPTANCE_TESTS_BUFFER_STREAM_ARRANGEMENT_H_

#include "mir_test_framework/connected_client_with_a_surface.h"
#include "mir/geometry/displacement.h"
#include "mir/geometry/rectangle.h"
#include "mir/compositor/display_buffer_compositor.h"
#include "mir/compositor/display_buffer_compositor_factory.h"
#include <mutex>
#include <condition_variable>

namespace mir
{
namespace test
{
struct RelativeRectangle
{
    RelativeRectangle() = default;
    RelativeRectangle(
        geometry::Displacement const& displacement,
        geometry::Size const& logical_size,
        geometry::Size const& physical_size);
    geometry::Displacement displacement;
    geometry::Size logical_size;
    geometry::Size physical_size;
};
bool operator==(mir::test::RelativeRectangle const& a, mir::test::RelativeRectangle const& b);

struct Stream
{
    MirBufferStream* handle() const;
    geometry::Point position();
    geometry::Size logical_size();
    geometry::Size physical_size();
    void set_size(geometry::Size size);
    void swap_buffers();
    Stream(Stream const&) = delete;
    Stream& operator=(Stream const&) = delete;
protected:
    Stream(geometry::Rectangle, std::function<MirBufferStream*()> const& create_stream);
    MirPixelFormat an_available_format(MirConnection* connection);
    geometry::Rectangle const position_;
    MirBufferStream* stream;
};

struct LegacyStream : Stream
{
    LegacyStream(MirConnection* connection,
        geometry::Size physical_size,
        geometry::Rectangle position);
    ~LegacyStream();
};

struct Ordering
{
    void note_scene_element_sequence(compositor::SceneElementSequence& sequence);
    template<typename T, typename S>
    bool wait_for_positions_within(
        std::vector<RelativeRectangle> const& awaited_positions,
        std::chrono::duration<T,S> duration)
    {
        std::unique_lock<decltype(mutex)> lk(mutex);
        return cv.wait_for(lk, duration, [this, awaited_positions] {
            for (auto& position : positions)
                if (position == awaited_positions) return true;
            positions.clear();
            return false;
        });
    }

private:
    std::mutex mutex;
    std::condition_variable cv;
    std::vector<std::vector<RelativeRectangle>> positions;
};

struct OrderTrackingDBC : compositor::DisplayBufferCompositor
{
    OrderTrackingDBC(std::shared_ptr<Ordering> const& ordering);
    void composite(compositor::SceneElementSequence&& scene_sequence) override;
    std::shared_ptr<Ordering> const ordering;
};

struct OrderTrackingDBCFactory : compositor::DisplayBufferCompositorFactory
{
    OrderTrackingDBCFactory(std::shared_ptr<Ordering> const& ordering);
    std::unique_ptr<compositor::DisplayBufferCompositor> create_compositor_for(graphics::DisplayBuffer&) override;

    std::shared_ptr<OrderTrackingDBC> last_dbc{nullptr};
    std::shared_ptr<compositor::DisplayBufferCompositorFactory> const wrapped;
    std::shared_ptr<Ordering> const ordering;
};

struct BufferStreamArrangementBase : mir_test_framework::ConnectedClientWithASurface
{
    void SetUp() override;
    void TearDown() override;

    std::shared_ptr<Ordering> ordering;
    std::shared_ptr<OrderTrackingDBCFactory> order_tracker{nullptr};
    std::vector<std::unique_ptr<Stream>> streams;
};
}
}

#endif /* TESTS_ACCEPTANCE_TESTS_BUFFER_STREAM_ARRANGEMENT_H_ */
