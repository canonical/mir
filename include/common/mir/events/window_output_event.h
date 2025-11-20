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

#ifndef MIR_COMMON_SURFACE_OUTPUT_EVENT_H_
#define MIR_COMMON_SURFACE_OUTPUT_EVENT_H_

#include <cstdint>

#include "mir/events/event.h"

struct MirWindowOutputEvent : MirEvent
{
    MirWindowOutputEvent();
    auto clone() const -> MirWindowOutputEvent* override;

    int surface_id() const;
    void set_surface_id(int id);

    int dpi() const;
    void set_dpi(int dpi);

    float scale() const;
    void set_scale(float scale);

    double refresh_rate() const;
    void set_refresh_rate(double);

    MirFormFactor form_factor() const;
    void set_form_factor(MirFormFactor factor);

    uint32_t output_id() const;
    void set_output_id(uint32_t id);

private:
    int surface_id_ = 0;
    int dpi_ = 0;
    float scale_ = 0.0;
    double refresh_rate_ = 0.0;
    MirFormFactor form_factor_ = mir_form_factor_unknown;
    uint32_t output_id_ = 0;
};

#endif /* MIR_COMMON_SURFACE_OUTPUT_EVENT_H_ */
