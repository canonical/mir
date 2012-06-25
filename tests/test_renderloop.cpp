/*
 * Copyright © 2012 Canonical Ltd.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Canonical Ltd. not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.
 * Canonical Ltd. makes no representations about the suitability of this
 * software for any purpose. It is provided "as is" without express or
 * implied warranty.
 *
 * CANONICAL, LTD. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL CANONICAL, LTD. BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/graphics/framebuffer_backend.h"
#include "mir/compositor/drawer.h"
#include "mir/compositor/compositor.h"
#include "mir/compositor/buffer_manager.h"
#include "mir/surfaces/scenegraph.h"
#include "mir/geometry/rectangle.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace ms = mir::surfaces;
namespace geom = mir::geometry;

namespace
{
class mock_framebuffer_backend : public mg::framebuffer_backend
{
public:
    MOCK_METHOD0(render, void ());
};

class mock_scenegraph : public ms::scenegraph
{
public:
    MOCK_METHOD1(get_surfaces_in, ms::surfaces_to_render (geom::rectangle const&));
};
}

TEST(compositor_renderloop, notify_sync_and_see_paint)
{
    using namespace testing;

    mock_framebuffer_backend graphics;
    mock_scenegraph scenegraph;

    mc::buffer_manager buffer_manager(&graphics);
    mc::drawer&& comp = mc::compositor(&scenegraph, &buffer_manager);

    EXPECT_CALL(graphics, render()).Times(AtLeast(1));

    EXPECT_CALL(scenegraph, get_surfaces_in(_))
    		.WillRepeatedly(Return(ms::surfaces_to_render()));

    comp.render(nullptr);
}

TEST(compositor_renderloop, notify_sync_and_see_scenegraph_query)
{
    using namespace testing;

    mock_framebuffer_backend graphics;
    mock_scenegraph scenegraph;

    mc::buffer_manager buffer_manager(&graphics);
    mc::drawer&& comp = mc::compositor(&scenegraph, &buffer_manager);

    EXPECT_CALL(graphics, render());

    EXPECT_CALL(scenegraph, get_surfaces_in(_)).Times(AtLeast(1))
    		.WillRepeatedly(Return(ms::surfaces_to_render()));

    comp.render(nullptr);
}
