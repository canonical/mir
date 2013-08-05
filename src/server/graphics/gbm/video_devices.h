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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_GRAPHICS_GBM_VIDEO_DEVICES_H_
#define MIR_GRAPHICS_GBM_VIDEO_DEVICES_H_

#include <functional>

namespace mir
{
namespace graphics
{
class EventHandlerRegister;

namespace gbm
{

class VideoDevices
{
public:
    virtual ~VideoDevices() = default;

    virtual void register_change_handler(
        EventHandlerRegister& handlers,
        std::function<void()> const& change_handler) = 0;

protected:
    VideoDevices() = default;
    VideoDevices(VideoDevices const&) = delete;
    VideoDevices& operator=(VideoDevices const&) = delete;
};

}
}
}

#endif /* MIR_GRAPHICS_GBM_VIDEO_DEVICES_H__ */
