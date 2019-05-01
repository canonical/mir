/*
 * Copyright © 2018 Canonical Ltd.
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
 * Authored by: William Wold <william.wold@canonical.com>
 */

#ifndef MIR_FRONTEND_XDG_OUTPUT_V1_H
#define MIR_FRONTEND_XDG_OUTPUT_V1_H

#include "xdg-output-unstable-v1_wrapper.h"

namespace mir
{
namespace frontend
{
class OutputManager;

class XdgOutputManagerV1 : public wayland::XdgOutputManagerV1
{
public:
    XdgOutputManagerV1(struct wl_display* display, OutputManager* const output_manager);
    ~XdgOutputManagerV1() = default;

private:
    void destroy(struct wl_client* client, struct wl_resource* resource) override;

    void get_xdg_output(
        struct wl_client* client,
        struct wl_resource* resource,
        uint32_t id,
        struct wl_resource* output) override;

    OutputManager* const output_manager;
};

}
}

#endif // MIR_FRONTEND_XDG_OUTPUT_V1_H
