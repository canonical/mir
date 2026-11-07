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

#ifndef MIR_COMPOSITOR_DISPLAY_BUFFER_COMPOSITOR_H_
#define MIR_COMPOSITOR_DISPLAY_BUFFER_COMPOSITOR_H_

#include <mir/compositor/scene.h>

namespace mir
{
namespace compositor
{

/**
 * How a composition attempt should present its content.
 */
enum class CompositionMode
{
    normal,    ///< Direct presentation (bypass / overlays / planes) is allowed
    recovery,  ///< Render the visible scene through the renderer; client-buffer bypass and plane promotion are disabled
};

class DisplayBufferCompositor
{
public:
    virtual ~DisplayBufferCompositor() = default;

    /**
     * Returns true if any compositing happened, otherwise false.
     *
     * In CompositionMode::recovery the implementation must render the visible
     * scene itself, without client-buffer bypass or plane promotion. The
     * hardware cursor plane is not affected.
     */
    [[nodiscard]] virtual bool composite(SceneElementSequence&& scene_sequence, CompositionMode mode) = 0;

protected:
    DisplayBufferCompositor() = default;
    DisplayBufferCompositor& operator=(DisplayBufferCompositor const&) = delete;
    DisplayBufferCompositor(DisplayBufferCompositor const&) = delete;
};

}
}

#endif /* MIR_COMPOSITOR_DISPLAY_BUFFER_COMPOSITOR_H_ */
