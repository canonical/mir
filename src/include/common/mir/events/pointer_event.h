/*
 * Copyright © Canonical Ltd.
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
#include "mir/events/scroll_axis.h"

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
                    mir::events::ScrollAxisH h_scroll,
                    mir::events::ScrollAxisV v_scroll);

    auto clone() const -> MirPointerEvent* override;

    auto axis_source() const -> MirPointerAxisSource;
    void set_axis_source(MirPointerAxisSource source);

    auto position() const -> std::optional<mir::geometry::PointF>;
    void set_position(std::optional<mir::geometry::PointF> value);

    auto local_position() const -> std::optional<mir::geometry::PointF>;
    void set_local_position(std::optional<mir::geometry::PointF> value);

    auto motion() const -> mir::geometry::DisplacementF;
    void set_motion(mir::geometry::DisplacementF value);

    auto h_scroll() const -> mir::events::ScrollAxisH;
    void set_h_scroll(mir::events::ScrollAxisH value);

    auto v_scroll() const -> mir::events::ScrollAxisV;
    void set_v_scroll(mir::events::ScrollAxisV value);

    MirPointerAction action() const;
    void set_action(MirPointerAction action);

    MirPointerButtons buttons() const;
    void set_buttons(MirPointerButtons buttons);

    void set_dnd_handle(std::vector<uint8_t> const& handle);
    MirBlob* dnd_handle() const;

private:
    std::optional<mir::geometry::PointF> local_position_;
    std::optional<mir::geometry::PointF> position_;
    mir::geometry::DisplacementF motion_;
    MirPointerAxisSource axis_source_;
    mir::events::ScrollAxisH h_scroll_;
    mir::events::ScrollAxisV v_scroll_;

    MirPointerAction action_ = {};
    MirPointerButtons buttons_ = {};

    std::optional<std::vector<uint8_t>> dnd_handle_;
};

#endif
