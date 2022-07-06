/*
 * Copyright © 2016-2017 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_COMMON_POINTER_EVENT_H_
#define MIR_COMMON_POINTER_EVENT_H_

#include "mir/events/input_event.h"
#include "mir/geometry/displacement_f.h"
#include "mir/geometry/point_f.h"

#include <optional>

typedef struct MirBlob MirBlob;

struct MirPointerEvent : MirInputEvent
{
    MirPointerEvent();
    MirPointerEvent(MirInputDeviceId dev,
                    std::chrono::nanoseconds et,
                    std::vector<uint8_t> const& cookie,
                    MirInputEventModifiers mods,
                    MirPointerAction action,
                    MirPointerButtons buttons,
                    std::optional<mir::geometry::PointF> position,
                    mir::geometry::DisplacementF motion,
                    MirPointerAxisSource axis_source,
                    mir::geometry::DisplacementF scroll,
                    mir::geometry::Displacement scroll_discrete,
                    mir::geometry::generic::Displacement<mir::geometry::generic::Value<bool>::Wrapper> scroll_stop);

    auto clone() const -> MirPointerEvent* override;

    auto axis_source() const -> MirPointerAxisSource;
    void set_axis_source(MirPointerAxisSource source);

    float x() const;
    void set_x(float x);

    float y() const;
    void set_y(float y);

    bool has_absolute_position() const;
    void set_has_absolute_position(bool value);

    float dx() const;
    void set_dx(float x);

    float dy() const;
    void set_dy(float y);

    float vscroll() const;
    void set_vscroll(float v);

    float hscroll() const;
    void set_hscroll(float h);

    bool vscroll_stop() const;
    void set_vscroll_stop(bool stop);

    bool hscroll_stop() const;
    void set_hscroll_stop(bool stop);

    float vscroll_discrete() const;
    void set_vscroll_discrete(float v);

    float hscroll_discrete() const;
    void set_hscroll_discrete(float h);

    MirPointerAction action() const;
    void set_action(MirPointerAction action);

    MirPointerButtons buttons() const;
    void set_buttons(MirPointerButtons buttons);

    void set_dnd_handle(std::vector<uint8_t> const& handle);
    MirBlob* dnd_handle() const;

private:
    std::optional<mir::geometry::PointF> position_;
    mir::geometry::DisplacementF motion_;
    MirPointerAxisSource axis_source_;
    mir::geometry::DisplacementF scroll_;
    mir::geometry::Displacement scroll_discrete_;
    mir::geometry::generic::Displacement<mir::geometry::generic::Value<bool>::Wrapper> scroll_stop_;

    MirPointerAction action_ = {};
    MirPointerButtons buttons_ = {};

    std::optional<std::vector<uint8_t>> dnd_handle_;
};

#endif
