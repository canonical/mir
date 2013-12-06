/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "drm_mode_resources.h"

#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mgm = mir::graphics::mesa;

namespace
{

struct CrtcDeleter
{
    void operator()(drmModeCrtc* p) { if (p) drmModeFreeCrtc(p); }
};

struct EncoderDeleter
{
    void operator()(drmModeEncoder* p) { if (p) drmModeFreeEncoder(p); }
};

struct ConnectorDeleter
{
    void operator()(drmModeConnector* p) { if (p) drmModeFreeConnector(p); }
};

struct ResourcesDeleter
{
    void operator()(drmModeRes* p) { if (p) drmModeFreeResources(p); }
};

}

mgm::DRMModeResources::DRMModeResources(int drm_fd)
    : drm_fd{drm_fd},
      resources{DRMModeResUPtr{drmModeGetResources(drm_fd), ResourcesDeleter()}}
{
    if (!resources)
        BOOST_THROW_EXCEPTION(std::runtime_error("Couldn't get DRM resources\n"));
}

void mgm::DRMModeResources::for_each_connector(std::function<void(DRMModeConnectorUPtr)> const& f) const
{
    for (int i = 0; i < resources->count_connectors; i++)
    {
        auto connector_ptr = connector(resources->connectors[i]);

        if (!connector_ptr)
            continue;

        f(std::move(connector_ptr));
    }
}

void mgm::DRMModeResources::for_each_encoder(std::function<void(DRMModeEncoderUPtr)> const& f) const
{
    for (int i = 0; i < resources->count_encoders; i++)
    {
        auto encoder_ptr = encoder(resources->encoders[i]);

        if (!encoder_ptr)
            continue;

        f(std::move(encoder_ptr));
    }
}

void mgm::DRMModeResources::for_each_crtc(std::function<void(DRMModeCrtcUPtr)> const& f) const
{
    for (int i = 0; i < resources->count_crtcs; i++)
    {
        auto crtc_ptr = crtc(resources->crtcs[i]);

        if (!crtc_ptr)
            continue;

        f(std::move(crtc_ptr));
    }
}

size_t mgm::DRMModeResources::num_connectors()
{
    return resources->count_connectors;
}

size_t mgm::DRMModeResources::num_encoders()
{
    return resources->count_encoders;
}

size_t mgm::DRMModeResources::num_crtcs()
{
    return resources->count_crtcs;
}

mgm::DRMModeConnectorUPtr mgm::DRMModeResources::connector(uint32_t id) const
{
    auto connector_raw = drmModeGetConnector(drm_fd, id);
    return DRMModeConnectorUPtr{connector_raw, ConnectorDeleter()};
}

mgm::DRMModeEncoderUPtr mgm::DRMModeResources::encoder(uint32_t id) const
{
    auto encoder_raw = drmModeGetEncoder(drm_fd, id);
    return DRMModeEncoderUPtr{encoder_raw, EncoderDeleter()};
}

mgm::DRMModeCrtcUPtr mgm::DRMModeResources::crtc(uint32_t id) const
{
    auto crtc_raw = drmModeGetCrtc(drm_fd, id);
    return DRMModeCrtcUPtr{crtc_raw, CrtcDeleter()};
}
