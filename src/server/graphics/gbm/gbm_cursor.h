/*
 * Copyright © 2013 Canonical Ltd.
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


#ifndef MIR_GRAPHICS_GBM_GBM_CURSOR_H_
#define MIR_GRAPHICS_GBM_GBM_CURSOR_H_

#include "mir/graphics/cursor.h"

#include <gbm.h>
#include <memory>

namespace mir
{
namespace geometry
{
struct Rectangle;
}
namespace graphics
{
namespace gbm
{
class KMSOutputContainer;
class KMSOutput;
class KMSDisplayConfiguration;
class GBMPlatform;

class CurrentConfiguration
{
public:
    virtual ~CurrentConfiguration() = default;

    virtual void with_current_configuration_do(
        std::function<void(KMSDisplayConfiguration const&)> const& exec) = 0;

protected:
    CurrentConfiguration() = default;
    CurrentConfiguration(CurrentConfiguration const&) = delete;
    CurrentConfiguration& operator=(CurrentConfiguration const&) = delete;
};

class GBMCursor : public Cursor
{
public:
    GBMCursor(
        gbm_device* device,
        KMSOutputContainer& output_container,
        std::shared_ptr<CurrentConfiguration> const& current_configuration);

    ~GBMCursor() noexcept;

    void set_image(const void* raw_argb, geometry::Size size);

    void move_to(geometry::Point position);

    void show_at_last_known_position();
    void hide();

private:
    enum ForceCursorState { UpdateState, ForceState };
    void for_each_used_output(std::function<void(KMSOutput&, geometry::Rectangle const&)> const& f);
    void place_cursor_at(geometry::Point position, ForceCursorState force_state);

    KMSOutputContainer& output_container;
    geometry::Point current_position;

    struct GBMBOWrapper
    {
        GBMBOWrapper(gbm_device* gbm);
        operator gbm_bo*();
        ~GBMBOWrapper();
    private:
        gbm_bo* buffer;
        GBMBOWrapper(GBMBOWrapper const&) = delete;
        GBMBOWrapper& operator=(GBMBOWrapper const&) = delete;
    } buffer;

    std::shared_ptr<CurrentConfiguration> const current_configuration;
};
}
}
}


#endif /* MIR_GRAPHICS_GBM_GBM_CURSOR_H_ */
