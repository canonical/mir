/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "real_kms_output.h"
#include "page_flipper.h"
#include "mir/fatal.h"
#include <string.h> // strcmp

namespace mg = mir::graphics;
namespace mgm = mg::mesa;
namespace geom = mir::geometry;

namespace
{

bool encoder_is_used(mgm::DRMModeResources const& resources, uint32_t encoder_id)
{
    bool encoder_used{false};

    resources.for_each_connector([&](mgm::DRMModeConnectorUPtr connector)
    {
        if (connector->encoder_id == encoder_id &&
            connector->connection == DRM_MODE_CONNECTED)
        {
            auto encoder = resources.encoder(connector->encoder_id);
            if (encoder)
            {
                auto crtc = resources.crtc(encoder->crtc_id);
                if (crtc)
                    encoder_used = true;
            }
        }
    });

    return encoder_used;
}

bool crtc_is_used(mgm::DRMModeResources const& resources, uint32_t crtc_id)
{
    bool crtc_used{false};

    resources.for_each_connector([&](mgm::DRMModeConnectorUPtr connector)
    {
        if (connector->connection == DRM_MODE_CONNECTED)
        {
            auto encoder = resources.encoder(connector->encoder_id);
            if (encoder)
            {
                if (encoder->crtc_id == crtc_id)
                    crtc_used = true;
            }
        }
    });

    return crtc_used;
}

std::vector<mgm::DRMModeEncoderUPtr>
connector_available_encoders(mgm::DRMModeResources const& resources,
                             drmModeConnector const* connector)
{
    std::vector<mgm::DRMModeEncoderUPtr> encoders;

    for (int i = 0; i < connector->count_encoders; i++)
    {
        if (!encoder_is_used(resources, connector->encoders[i]))
            encoders.push_back(resources.encoder(connector->encoders[i]));
    }

    return encoders;
}

bool encoder_supports_crtc_index(drmModeEncoder const* encoder, uint32_t crtc_index)
{
    return (encoder->possible_crtcs & (1 << crtc_index));
}

const char *connector_type_name(uint32_t type)
{
    static const int nnames = 15;
    static const char * const names[nnames] =
    {   // Ordered according to xf86drmMode.h
        "Unknown",
        "VGA",
        "DVII",
        "DVID",
        "DVIA",
        "Composite",
        "SVIDEO",
        "LVDS",
        "Component",
        "9PinDIN",
        "DisplayPort",
        "HDMIA",
        "HDMIB",
        "TV",
        "eDP"
    };

    if (type >= nnames)
        type = 0;

    return names[type];
}

std::string connector_name(const drmModeConnector *conn)
{
    std::string name = connector_type_name(conn->connector_type);
    name += '-';
    name += std::to_string(conn->connector_type_id);
    return name;
}

}

mgm::RealKMSOutput::RealKMSOutput(int drm_fd, uint32_t connector_id,
                                  std::shared_ptr<PageFlipper> const& page_flipper)
    : drm_fd{drm_fd}, connector_id{connector_id}, page_flipper{page_flipper},
      connector(), mode_index{0}, current_crtc(), saved_crtc(),
      using_saved_crtc{true}, has_cursor_{false},
      power_mode(mir_power_mode_on)
{
    reset();

    DRMModeResources resources{drm_fd};

    auto encoder = resources.encoder(connector->encoder_id);
    if (encoder)
    {
        auto crtc = resources.crtc(encoder->crtc_id);
        if (crtc)
            saved_crtc = *crtc;
    }
}

mgm::RealKMSOutput::~RealKMSOutput()
{
    restore_saved_crtc();
}

void mgm::RealKMSOutput::reset()
{
    DRMModeResources resources{drm_fd};

    /* Update the connector to ensure we have the latest information */
    connector = resources.connector(connector_id);

    if (!connector)
        fatal_error("No DRM connector found");

    // TODO: What if we can't locate the DPMS property?
    for (int i = 0; i < connector->count_props; i++)
    {
        auto prop = drmModeGetProperty(drm_fd, connector->props[i]);
        if (prop && (prop->flags & DRM_MODE_PROP_ENUM)) {
            if (!strcmp(prop->name, "DPMS"))
            {
                dpms_enum_id = connector->props[i];
                drmModeFreeProperty(prop);
                break;
            }
            drmModeFreeProperty(prop);
        }
    }

    /* Discard previously current crtc */
    current_crtc = nullptr;
}

geom::Size mgm::RealKMSOutput::size() const
{
    drmModeModeInfo const& mode(connector->modes[mode_index]);
    return {mode.hdisplay, mode.vdisplay};
}

int mgm::RealKMSOutput::max_refresh_rate() const
{
    // TODO: In future when DRM exposes FreeSync/Adaptive Sync/G-Sync info
    //       this value may be calculated differently.
    drmModeModeInfo const& current_mode = connector->modes[mode_index];
    return current_mode.vrefresh;
}

void mgm::RealKMSOutput::configure(geom::Displacement offset, size_t kms_mode_index)
{
    fb_offset = offset;
    mode_index = kms_mode_index;
}

bool mgm::RealKMSOutput::set_crtc(uint32_t fb_id)
{
    if (!ensure_crtc())
    {
        fatal_error("Output %s has no associated CRTC to set a framebuffer on",
                    connector_name(connector.get()).c_str());
    }

    auto ret = drmModeSetCrtc(drm_fd, current_crtc->crtc_id,
                              fb_id, fb_offset.dx.as_int(), fb_offset.dy.as_int(),
                              &connector->connector_id, 1,
                              &connector->modes[mode_index]);
    if (ret)
    {
        current_crtc = nullptr;
        return false;
    }

    using_saved_crtc = false;
    return true;
}

void mgm::RealKMSOutput::clear_crtc()
{
    /*
     * In order to actually clear the output, we need to have a crtc
     * connected to the output/connector so that we can disconnect
     * it. However, not being able to get a crtc is OK, since it means
     * that the output cannot be displaying anything anyway.
     */
    if (!ensure_crtc())
        return;

    auto result = drmModeSetCrtc(drm_fd, current_crtc->crtc_id,
                                 0, 0, 0, nullptr, 0, nullptr);
    if (result)
    {
        fatal_error("Couldn't clear output %s (drmModeSetCrtc = %d)",
                   connector_name(connector.get()).c_str(), result);
    }

    current_crtc = nullptr;
}

bool mgm::RealKMSOutput::schedule_page_flip(uint32_t fb_id)
{
    std::unique_lock<std::mutex> lg(power_mutex);
    if (power_mode != mir_power_mode_on)
        return true;
    if (!current_crtc)
    {
        fatal_error("Output %s has no associated CRTC to schedule page flips on",
                   connector_name(connector.get()).c_str());
    }
    return page_flipper->schedule_flip(current_crtc->crtc_id, fb_id);
}

void mgm::RealKMSOutput::wait_for_page_flip()
{
    std::unique_lock<std::mutex> lg(power_mutex);
    if (power_mode != mir_power_mode_on)
        return;
    if (!current_crtc)
    {
        fatal_error("Output %s has no associated CRTC to wait on",
                   connector_name(connector.get()).c_str());
    }
    page_flipper->wait_for_flip(current_crtc->crtc_id);
}

void mgm::RealKMSOutput::set_cursor(gbm_bo* buffer)
{
    if (current_crtc)
    {
        if (auto result = drmModeSetCursor(
                drm_fd,
                current_crtc->crtc_id,
                gbm_bo_get_handle(buffer).u32,
                gbm_bo_get_width(buffer),
                gbm_bo_get_height(buffer)))
        {
            fatal_error("drmModeSetCursor failed (returned %d)", result);
        }

        has_cursor_ = true;
    }
}

void mgm::RealKMSOutput::move_cursor(geometry::Point destination)
{
    if (current_crtc)
    {
        if (auto result = drmModeMoveCursor(drm_fd, current_crtc->crtc_id,
                                            destination.x.as_uint32_t(),
                                            destination.y.as_uint32_t()))
        {
            fatal_error("drmModeMoveCursor failed (returned %d)", result);
        }
    }
}

void mgm::RealKMSOutput::clear_cursor()
{
    if (current_crtc)
    {
        drmModeSetCursor(drm_fd, current_crtc->crtc_id, 0, 0, 0);
        has_cursor_ = false;
    }
}

bool mgm::RealKMSOutput::has_cursor() const
{
    return has_cursor_;
}

bool mgm::RealKMSOutput::ensure_crtc()
{
    /* Nothing to do if we already have a crtc */
    if (current_crtc)
        return true;

    /* If the output is not connected there is nothing to do */
    if (connector->connection != DRM_MODE_CONNECTED)
        return false;

    DRMModeResources resources{drm_fd};

    /* Check to see if there is a crtc already connected */
    auto encoder = resources.encoder(connector->encoder_id);
    if (encoder)
        current_crtc = resources.crtc(encoder->crtc_id);

    /* If we don't have a current crtc, try to find one */
    if (!current_crtc)
    {
        auto available_encoders = connector_available_encoders(resources, connector.get());

        int crtc_index = 0;

        resources.for_each_crtc([&](DRMModeCrtcUPtr crtc)
        {
            if (!current_crtc && !crtc_is_used(resources, crtc->crtc_id))
            {
                for (auto& enc : available_encoders)
                {
                    if (encoder_supports_crtc_index(enc.get(), crtc_index))
                    {
                        current_crtc = std::move(crtc);
                        break;
                    }
                }
            }

            crtc_index++;
        });
    }

    return (current_crtc != nullptr);
}

void mgm::RealKMSOutput::restore_saved_crtc()
{
    if (!using_saved_crtc)
    {
        drmModeSetCrtc(drm_fd, saved_crtc.crtc_id, saved_crtc.buffer_id,
                       saved_crtc.x, saved_crtc.y,
                       &connector->connector_id, 1, &saved_crtc.mode);

        using_saved_crtc = true;
    }
}

void mgm::RealKMSOutput::set_power_mode(MirPowerMode mode)
{
    std::lock_guard<std::mutex> lg(power_mutex);

    if (power_mode != mode)
    {
        power_mode = mode;
        drmModeConnectorSetProperty(drm_fd, connector_id,
                                   dpms_enum_id, mode);
    }
}
