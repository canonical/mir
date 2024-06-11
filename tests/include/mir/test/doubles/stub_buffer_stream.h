/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_TEST_DOUBLES_NULL_BUFFER_STREAM_H_
#define MIR_TEST_DOUBLES_NULL_BUFFER_STREAM_H_

#include "mir/geometry/forward.h"
#include "mir_toolkit/common.h"
#include <mir/compositor/buffer_stream.h>
#include <mir/test/doubles/stub_buffer.h>

namespace mir
{
namespace test
{
namespace doubles
{

class StubBufferStream : public compositor::BufferStream
{
public:
    StubBufferStream()
    {
        stub_compositor_buffer = std::make_shared<StubBuffer>();
    }

    auto next_submission_for_compositor(void const*) -> std::shared_ptr<Submission> override
    {
        class DummySubmission : public Submission
        {
        public:
            DummySubmission(std::shared_ptr<graphics::Buffer> buf)
                : buf{std::move(buf)}
            {
            }

            auto claim_buffer() -> std::shared_ptr<graphics::Buffer> override
            {
                return buf;
            }

            auto size() const -> geometry::Size override
            {
                return buf->size();
            }

            auto source_rect() const -> geometry::RectangleD override
            {
                return {{0, 0}, geometry::SizeD{buf->size()}};
            }

            auto pixel_format() const -> graphics::DRMFormat override
            {
                return graphics::DRMFormat::from_mir_format(mir_pixel_format_xbgr_8888);
            }
        private:
            std::shared_ptr<graphics::Buffer> const buf;
        };

        --nready;
        return std::make_shared<DummySubmission>(stub_compositor_buffer);
    }

    void submit_buffer(
        std::shared_ptr<graphics::Buffer> const& b,
        geometry::Size /*dst_size*/,
        geometry::RectangleD /*src_bounds*/) override
    {
        if (b) ++nready;
    }
    void set_frame_posted_callback(std::function<void(geometry::Size const&)> const&) override {}
    bool has_submitted_buffer() const override { return true; }

    std::shared_ptr<graphics::Buffer> stub_compositor_buffer;
    int nready = 0;
};

}
}
} // namespace mir

#endif /* MIR_TEST_DOUBLES_NULL_BUFFER_STREAM_H_ */
