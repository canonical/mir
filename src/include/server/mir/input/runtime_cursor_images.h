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

#ifndef MIR_INPUT_RUNTIME_CURSOR_IMAGES_H_
#define MIR_INPUT_RUNTIME_CURSOR_IMAGES_H_

#include <mir/input/cursor_images.h>
#include <mir/synchronised.h>

#include <functional>
#include <vector>

namespace mir
{
namespace input
{
/// A CursorImages proxy that delegates to a swappable inner implementation.
/// When the inner is swapped (e.g. on cursor theme change), registered
/// change callbacks are invoked to notify consumers.
class RuntimeCursorImages : public CursorImages
{
public:
    explicit RuntimeCursorImages(std::shared_ptr<CursorImages> inner);

    std::shared_ptr<graphics::CursorImage> image(std::string const& cursor_name,
        geometry::Size const& size) override;

    /// Replace the inner CursorImages delegate (e.g. when the cursor theme changes).
    /// All registered change callbacks are invoked after the swap.
    void set_cursor_images(std::shared_ptr<CursorImages> new_inner);

    /// Register a callback to be invoked when the inner CursorImages is swapped.
    void register_change_callback(std::function<void()> callback);

private:
    struct State
    {
        std::shared_ptr<CursorImages> inner;
        std::vector<std::function<void()>> change_callbacks;
    };
    Synchronised<State> state;
};
}
}

#endif // MIR_INPUT_RUNTIME_CURSOR_IMAGES_H_
