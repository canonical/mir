/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Brandon Schaefer <brandon.schaefer@canonical.com>
 */

#ifndef MIR_COMMON_SURFACE_OUTPUT_EVENT_H_
#define MIR_COMMON_SURFACE_OUTPUT_EVENT_H_

#include <cstdint>
#include <limits>

#include "mir/events/event.h"

struct MirSurfaceOutputEvent : MirEvent
{
    MirSurfaceOutputEvent();

    int surface_id() const;
    void set_surface_id(int id);

    int dpi() const;
    void set_dpi(int dpi);

    float scale() const;
    void set_scale(float scale);

    MirFormFactor form_factor() const;
    void set_form_factor(MirFormFactor factor);

    uint32_t output_id() const;
    void set_output_id(uint32_t id);

private:
    int surface_id_{-1};
    int dpi_{0};
    float scale_{0.0f};
    MirFormFactor form_factor_;
    uint32_t output_id_{std::numeric_limits<uint32_t>::max()};
};

#endif /* MIR_COMMON_SURFACE_OUTPUT_EVENT_H_ */
