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

#include "real_kms_output.h"
#include "page_flipper.h"

#include <boost/throw_exception.hpp>
#include <boost/exception/info.hpp>

#include <stdexcept>
#include <vector>

namespace mgg = mir::graphics::gbm;
namespace geom = mir::geometry;

namespace
{

bool encoder_is_used(mgg::DRMModeResources const& resources, uint32_t encoder_id)
{
    bool encoder_used{false};

    resources.for_each_connector([&](mgg::DRMModeConnectorUPtr connector)
    {
        if (connector->encoder_id == encoder_id)
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

bool crtc_is_used(mgg::DRMModeResources const& resources, uint32_t crtc_id)
{
    bool crtc_used{false};

    resources.for_each_connector([&](mgg::DRMModeConnectorUPtr connector)
    {
        auto encoder = resources.encoder(connector->encoder_id);
        if (encoder)
        {
            if (encoder->crtc_id == crtc_id)
                crtc_used = true;
        }
    });

    return crtc_used;
}

std::vector<mgg::DRMModeEncoderUPtr>
connector_available_encoders(mgg::DRMModeResources const& resources,
                             drmModeConnector const* connector)
{
    std::vector<mgg::DRMModeEncoderUPtr> encoders;

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

}

mgg::RealKMSOutput::RealKMSOutput(int drm_fd, uint32_t connector_id,
                                  std::shared_ptr<PageFlipper> const& page_flipper)
    : drm_fd{drm_fd}, connector_id{connector_id}, page_flipper{page_flipper},
      connector(), mode_index{0}, current_crtc(), saved_crtc(),
      using_saved_crtc{true}, has_cursor_{false}
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

mgg::RealKMSOutput::~RealKMSOutput()
{
    restore_saved_crtc();
}

void mgg::RealKMSOutput::reset()
{
    DRMModeResources resources{drm_fd};

    /* Update the connector to ensure we have the latest information */
    connector = resources.connector(connector_id);

    if (!connector)
        BOOST_THROW_EXCEPTION(std::runtime_error("No DRM connector found\n"));

    /* Discard previously current crtc */
    current_crtc = nullptr;
}

geom::Size mgg::RealKMSOutput::size() const
{
    drmModeModeInfo const& mode(connector->modes[mode_index]);
    return {mode.hdisplay, mode.vdisplay};
}

void mgg::RealKMSOutput::configure(geom::Displacement offset, size_t kms_mode_index)
{
    fb_offset = offset;
    mode_index = kms_mode_index;
}

bool mgg::RealKMSOutput::set_crtc(uint32_t fb_id)
{
    if (!ensure_crtc())
        BOOST_THROW_EXCEPTION(std::runtime_error("Output has no associated crtc"));

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

bool mgg::RealKMSOutput::schedule_page_flip(uint32_t fb_id)
{
    if (!current_crtc)
        BOOST_THROW_EXCEPTION(std::runtime_error("Output has no associated crtc"));

    return page_flipper->schedule_flip(current_crtc->crtc_id, fb_id);
}

void mgg::RealKMSOutput::wait_for_page_flip()
{
    if (!current_crtc)
        BOOST_THROW_EXCEPTION(std::runtime_error("Output has no associated crtc"));

    page_flipper->wait_for_flip(current_crtc->crtc_id);
}

void mgg::RealKMSOutput::set_cursor(gbm_bo* buffer)
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
            BOOST_THROW_EXCEPTION(
                ::boost::enable_error_info(std::runtime_error("drmModeSetCursor() failed"))
                    << (boost::error_info<KMSOutput, decltype(result)>(result)));
        }

        has_cursor_ = true;
    }
}

void mgg::RealKMSOutput::move_cursor(geometry::Point destination)
{
    if (current_crtc)
    {
        if (auto result = drmModeMoveCursor(drm_fd, current_crtc->crtc_id,
                                            destination.x.as_uint32_t(),
                                            destination.y.as_uint32_t()))
        {
            BOOST_THROW_EXCEPTION(
                ::boost::enable_error_info(std::runtime_error("drmModeMoveCursor() failed"))
                    << (boost::error_info<KMSOutput, decltype(result)>(result)));
        }
    }
}

void mgg::RealKMSOutput::clear_cursor()
{
    if (current_crtc)
    {
        drmModeSetCursor(drm_fd, current_crtc->crtc_id, 0, 0, 0);
        has_cursor_ = false;
    }
}

bool mgg::RealKMSOutput::has_cursor() const
{
    return has_cursor_;
}

bool mgg::RealKMSOutput::ensure_crtc()
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

void mgg::RealKMSOutput::restore_saved_crtc()
{
    if (!using_saved_crtc)
    {
        drmModeSetCrtc(drm_fd, saved_crtc.crtc_id, saved_crtc.buffer_id,
                       saved_crtc.x, saved_crtc.y,
                       &connector->connector_id, 1, &saved_crtc.mode);

        using_saved_crtc = true;
    }
}
