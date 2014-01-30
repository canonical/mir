/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_FRONTEND_SCREENCAST_H_
#define MIR_FRONTEND_SCREENCAST_H_

#include "mir/int_wrapper.h"
#include "mir/graphics/display_configuration.h"

#include <memory>

namespace mir
{
namespace graphics { class Buffer; }
namespace frontend
{
namespace detail { struct ScreencastSessionIdTag; }

typedef IntWrapper<detail::ScreencastSessionIdTag,uint32_t> ScreencastSessionId;

class Screencast
{
public:
    virtual ~Screencast() = default;

    virtual ScreencastSessionId create_session(
        graphics::DisplayConfigurationOutputId output_id) = 0;
    virtual void destroy_session(ScreencastSessionId id) = 0;
    virtual std::shared_ptr<graphics::Buffer> capture(ScreencastSessionId id) = 0;

protected:
    Screencast() = default;
    Screencast(Screencast const&) = delete;
    Screencast& operator=(Screencast const&) = delete;
};

}
}

#endif /* MIR_FRONTEND_SCREENCAST_H_ */
