/*
 * Copyright © 2013 Canonical Ltd.
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

#include "real_kms_output.h"
#include "mir/graphics/display_configuration.h"
#include "page_flipper.h"
#include "kms-utils/kms_connector.h"
#include "mir/fatal.h"
#include "mir/log.h"
#include <string.h> // strcmp

#include <boost/throw_exception.hpp>
#include <system_error>
#include <xf86drm.h>

namespace mg = mir::graphics;
namespace mgg = mg::gbm;
namespace mgk = mg::kms;
namespace geom = mir::geometry;

class mgg::FBHandle
{
public:
    FBHandle(int drm_fd, uint32_t fb_id)
        : drm_fd{drm_fd},
          fb_id{fb_id}
    {
    }

    ~FBHandle()
    {
        // TODO: Some sort of logging on failure?
        drmModeRmFB(drm_fd, fb_id);
    }

    auto get_drm_fb_id() const -> uint32_t
    {
        return fb_id;
    }
private:
    int const drm_fd;
    uint32_t const fb_id;
};


mgg::RealKMSOutput::RealKMSOutput(
    int drm_fd,
    kms::DRMModeConnectorUPtr&& connector,
    std::shared_ptr<PageFlipper> const& page_flipper)
    : drm_fd_{drm_fd},
      page_flipper{page_flipper},
      connector{std::move(connector)},
      mode_index{0},
      current_crtc(),
      saved_crtc(),
      using_saved_crtc{true},
      has_cursor_{false},
      power_mode(mir_power_mode_on)
{
    reset();

    kms::DRMModeResources resources{drm_fd_};

    if (this->connector->encoder_id)
    {
        auto encoder = resources.encoder(this->connector->encoder_id);
        if (encoder->crtc_id)
        {
            saved_crtc = *resources.crtc(encoder->crtc_id);
        }
    }
}

mgg::RealKMSOutput::~RealKMSOutput()
{
    restore_saved_crtc();
}

uint32_t mgg::RealKMSOutput::id() const
{
    return connector->connector_id;
}

void mgg::RealKMSOutput::reset()
{
    kms::DRMModeResources resources{drm_fd_};

    /* Update the connector to ensure we have the latest information */
    try
    {
        connector = resources.connector(connector->connector_id);
    }
    catch (std::exception const& e)
    {
        fatal_error(e.what());
    }

    // TODO: What if we can't locate the DPMS property?
    for (int i = 0; i < connector->count_props; i++)
    {
        auto prop = drmModeGetProperty(drm_fd_, connector->props[i]);
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

geom::Size mgg::RealKMSOutput::size() const
{
    drmModeModeInfo const& mode(connector->modes[mode_index]);
    return {mode.hdisplay, mode.vdisplay};
}

int mgg::RealKMSOutput::max_refresh_rate() const
{
    if (connector->connection == DRM_MODE_DISCONNECTED)
        return 1;

    drmModeModeInfo const& current_mode = connector->modes[mode_index];
    return current_mode.vrefresh;
}

void mgg::RealKMSOutput::configure(geom::Displacement offset, size_t kms_mode_index)
{
    fb_offset = offset;
    mode_index = kms_mode_index;
}

bool mgg::RealKMSOutput::set_crtc(FBHandle const& fb)
{
    if (!ensure_crtc())
    {
        mir::log_error("Output %s has no associated CRTC to set a framebuffer on",
                       mgk::connector_name(connector).c_str());
        return false;
    }

    auto ret = drmModeSetCrtc(drm_fd_, current_crtc->crtc_id,
                              fb.get_drm_fb_id(), fb_offset.dx.as_int(), fb_offset.dy.as_int(),
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

void mgg::RealKMSOutput::clear_crtc()
{
    try
    {
        ensure_crtc();
    }
    catch (...)
    {
        /*
         * In order to actually clear the output, we need to have a crtc
         * connected to the output/connector so that we can disconnect
         * it. However, not being able to get a crtc is OK, since it means
         * that the output cannot be displaying anything anyway.
         */
        return;
    }

    auto result = drmModeSetCrtc(drm_fd_, current_crtc->crtc_id,
                                 0, 0, 0, nullptr, 0, nullptr);
    if (result)
    {
        if (result == -EACCES || result == -EPERM)
        {
            /* We don't have modesetting rights.
             *
             * This can happen during session switching if (eg) logind has already
             * revoked device access before notifying us.
             *
             * Whatever we're switching to can handle the CRTCs; this should not be fatal.
             */
            mir::log_info("Couldn't clear output %s (drmModeSetCrtc: %s (%i))",
                mgk::connector_name(connector).c_str(),
                strerror(-result),
                -result);
        }
        else
        {
            fatal_error("Couldn't clear output %s (drmModeSetCrtc = %d)",
                        mgk::connector_name(connector).c_str(), result);
        }
    }

    current_crtc = nullptr;
}

bool mgg::RealKMSOutput::schedule_page_flip(FBHandle const& fb)
{
    std::unique_lock lg(power_mutex);
    if (power_mode != mir_power_mode_on)
        return true;
    if (!current_crtc)
    {
        mir::log_error("Output %s has no associated CRTC to schedule page flips on",
                       mgk::connector_name(connector).c_str());
        return false;
    }
    return page_flipper->schedule_flip(
        current_crtc->crtc_id,
        fb.get_drm_fb_id(),
        connector->connector_id);
}

void mgg::RealKMSOutput::wait_for_page_flip()
{
    std::unique_lock lg(power_mutex);
    if (power_mode != mir_power_mode_on)
        return;
    if (!current_crtc)
    {
        fatal_error("Output %s has no associated CRTC to wait on",
                   mgk::connector_name(connector).c_str());
    }

    last_frame_.store(page_flipper->wait_for_flip(current_crtc->crtc_id));
}

mg::Frame mgg::RealKMSOutput::last_frame() const
{
    return last_frame_.load();
}

bool mgg::RealKMSOutput::set_cursor(gbm_bo* buffer)
{
    int result = 0;
    if (current_crtc)
    {
        has_cursor_ = true;
        result = drmModeSetCursor(
                drm_fd_,
                current_crtc->crtc_id,
                gbm_bo_get_handle(buffer).u32,
                gbm_bo_get_width(buffer),
                gbm_bo_get_height(buffer));
        if (result)
        {
            has_cursor_ = false;
            mir::log_warning("set_cursor: drmModeSetCursor failed (%s)",
                             strerror(-result));
        }
    }
    return !result;
}

void mgg::RealKMSOutput::move_cursor(geometry::Point destination)
{
    if (current_crtc)
    {
        if (auto result = drmModeMoveCursor(drm_fd_, current_crtc->crtc_id,
                                            destination.x.as_int(),
                                            destination.y.as_int()))
        {
            mir::log_warning("move_cursor: drmModeMoveCursor failed (%s)",
                             strerror(-result));
        }
    }
}

bool mgg::RealKMSOutput::clear_cursor()
{
    int result = 0;
    if (current_crtc)
    {
        result = drmModeSetCursor(drm_fd_, current_crtc->crtc_id, 0, 0, 0);

        if (result)
            mir::log_warning("clear_cursor: drmModeSetCursor failed (%s)",
                             strerror(-result));
        has_cursor_ = false;
    }

    return !result;
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

    current_crtc = mgk::find_crtc_for_connector(drm_fd_, connector);


    return (current_crtc != nullptr);
}

void mgg::RealKMSOutput::restore_saved_crtc()
{
    if (!using_saved_crtc)
    {
        drmModeSetCrtc(drm_fd_, saved_crtc.crtc_id, saved_crtc.buffer_id,
                       saved_crtc.x, saved_crtc.y,
                       &connector->connector_id, 1, &saved_crtc.mode);

        using_saved_crtc = true;
    }
}

void mgg::RealKMSOutput::set_power_mode(MirPowerMode mode)
{
    std::lock_guard lg(power_mutex);

    if (power_mode != mode)
    {
        power_mode = mode;
        drmModeConnectorSetProperty(
            drm_fd_,
            connector->connector_id,
            dpms_enum_id,
            mode);
    }
}

void mgg::RealKMSOutput::set_gamma(mg::GammaCurves const& gamma)
{
    if (!ensure_crtc())
    {
        mir::log_warning("Output %s has no associated CRTC to set gamma on",
                         mgk::connector_name(connector).c_str());
        return;
    }

    if (gamma.red.size() != gamma.green.size() ||
        gamma.green.size() != gamma.blue.size())
    {
        BOOST_THROW_EXCEPTION(
            std::invalid_argument("set_gamma: mismatch gamma LUT sizes"));
    }

    int ret = drmModeCrtcSetGamma(
        drm_fd_,
        current_crtc->crtc_id,
        gamma.red.size(),
        const_cast<uint16_t*>(gamma.red.data()),
        const_cast<uint16_t*>(gamma.green.data()),
        const_cast<uint16_t*>(gamma.blue.data()));

    int err = -ret;
    if (err)
        mir::log_warning("drmModeCrtcSetGamma failed: %s", strerror(err));

    // TODO: return bool in future? Then do what with it?
}

void mgg::RealKMSOutput::refresh_hardware_state()
{
    connector = kms::get_connector(drm_fd_, connector->connector_id);
    current_crtc = nullptr;

    if (connector->encoder_id)
    {
        auto encoder = kms::get_encoder(drm_fd_, connector->encoder_id);

        if (encoder->crtc_id)
        {
            current_crtc = kms::get_crtc(drm_fd_, encoder->crtc_id);
        }
    }
}

namespace
{

bool kms_modes_are_equal(drmModeModeInfo const& info1, drmModeModeInfo const& info2)
{
    return (info1.clock == info2.clock &&
            info1.hdisplay == info2.hdisplay &&
            info1.hsync_start == info2.hsync_start &&
            info1.hsync_end == info2.hsync_end &&
            info1.htotal == info2.htotal &&
            info1.hskew == info2.hskew &&
            info1.vdisplay == info2.vdisplay &&
            info1.vsync_start == info2.vsync_start &&
            info1.vsync_end == info2.vsync_end &&
            info1.vtotal == info2.vtotal);
}

double calculate_vrefresh_hz(drmModeModeInfo const& mode)
{
    if (mode.htotal == 0 || mode.vtotal == 0)
        return 0.0;

    /* mode.clock is in KHz */
    double hz = (mode.clock * 100000LL /
                 ((long)mode.htotal * (long)mode.vtotal)
                ) / 100.0;

    // Actually we don't need floating point at all for this...
    // TODO: Consider converting our structs to fixed-point ints
    return hz;
}

mg::DisplayConfigurationOutputType
kms_connector_type_to_output_type(uint32_t connector_type)
{
    return static_cast<mg::DisplayConfigurationOutputType>(connector_type);
}

MirSubpixelArrangement kms_subpixel_to_mir_subpixel(uint32_t subpixel)
{
    switch (subpixel)
    {
        case DRM_MODE_SUBPIXEL_UNKNOWN:
            return mir_subpixel_arrangement_unknown;
        case DRM_MODE_SUBPIXEL_HORIZONTAL_RGB:
            return mir_subpixel_arrangement_horizontal_rgb;
        case DRM_MODE_SUBPIXEL_HORIZONTAL_BGR:
            return mir_subpixel_arrangement_horizontal_bgr;
        case DRM_MODE_SUBPIXEL_VERTICAL_RGB:
            return mir_subpixel_arrangement_vertical_rgb;
        case DRM_MODE_SUBPIXEL_VERTICAL_BGR:
            return mir_subpixel_arrangement_vertical_bgr;
        case DRM_MODE_SUBPIXEL_NONE:
            return mir_subpixel_arrangement_none;
        default:
            return mir_subpixel_arrangement_unknown;
    }
}

std::vector<uint8_t> edid_for_connector(int drm_fd, uint32_t connector_id)
{
    std::vector<uint8_t> edid;

    mgk::ObjectProperties connector_props{
        drm_fd, connector_id, DRM_MODE_OBJECT_CONNECTOR};

    if (connector_props.has_property("EDID"))
    {
        /*
         * We don't technically need the property information here, but query it
         * anyway so we can detect if our assumptions about DRM behaviour
         * become invalid.
         */
        auto property = mgk::DRMModePropertyUPtr{
            drmModeGetProperty(drm_fd, connector_props.id_for("EDID")),
            &drmModeFreeProperty};

        if (!property)
        {
            mir::log_warning(
                "Failed to get EDID property for connector %u: %i (%s)",
                connector_id,
                errno,
                ::strerror(errno));
            return edid;
        }

        if (!drm_property_type_is(property.get(), DRM_MODE_PROP_BLOB))
        {
            mir::log_warning(
                "EDID property on connector %u has unexpected type %u",
                connector_id,
                property->flags);
            return edid;
        }

        // A property ID of 0 means invalid.
        if (connector_props["EDID"] == 0)
        {
            /*
             * Log a debug message only. This will trigger for broken monitors which
             * don't provide an EDID, which is not as unusual as you might think...
             */
            mir::log_debug("No EDID data available on connector %u", connector_id);
            return edid;
        }

        auto blob = drmModeGetPropertyBlob(drm_fd, connector_props["EDID"]);

        if (!blob)
        {
            mir::log_warning(
                "Failed to get EDID property blob for connector %u: %i (%s)",
                connector_id,
                errno,
                ::strerror(errno));

            return edid;
        }

        edid.reserve(blob->length);
        edid.insert(edid.begin(),
                    reinterpret_cast<uint8_t*>(blob->data),
                    reinterpret_cast<uint8_t*>(blob->data) + blob->length);

        drmModeFreePropertyBlob(blob);

        edid.shrink_to_fit();
    }

    return edid;
}
}

void mgg::RealKMSOutput::update_from_hardware_state(
    DisplayConfigurationOutput& output) const
{
    DisplayConfigurationOutputType const type{
        kms_connector_type_to_output_type(connector->connector_type)};
    geom::Size physical_size{connector->mmWidth, connector->mmHeight};
    bool connected{connector->connection == DRM_MODE_CONNECTED};
    uint32_t const invalid_mode_index = std::numeric_limits<uint32_t>::max();
    uint32_t current_mode_index{invalid_mode_index};
    uint32_t preferred_mode_index{invalid_mode_index};
    std::vector<DisplayConfigurationMode> modes;
    std::vector<MirPixelFormat> formats{mir_pixel_format_argb_8888,
                                        mir_pixel_format_xrgb_8888};

    std::vector<uint8_t> edid;
    if (connected) {
        /* Only ask for the EDID on connected outputs. There's obviously no monitor EDID
         * when there is no monitor connected!
         */
        edid = edid_for_connector(drm_fd_, connector->connector_id);
    }

    drmModeModeInfo current_mode_info = drmModeModeInfo();
    GammaCurves gamma;

    /* Get information about the current mode */
    if (current_crtc) {
        current_mode_info = current_crtc->mode;

        if (current_crtc->gamma_size > 0)
            gamma = mg::LinearGammaLUTs(current_crtc->gamma_size);
    }

    /* Add all the available modes and find the current and preferred one */
    for (int m = 0; m < connector->count_modes; m++) {
        drmModeModeInfo &mode_info = connector->modes[m];

        geom::Size size{mode_info.hdisplay, mode_info.vdisplay};

        double vrefresh_hz = calculate_vrefresh_hz(mode_info);

        modes.push_back({size, vrefresh_hz});

        if (kms_modes_are_equal(mode_info, current_mode_info))
            current_mode_index = m;

        if ((mode_info.type & DRM_MODE_TYPE_PREFERRED) == DRM_MODE_TYPE_PREFERRED)
            preferred_mode_index = m;
    }

    /* Fallback for VMWare which fails to specify a matching current mode (bug:1661295) */
    if (current_mode_index == invalid_mode_index) {
        for (int m = 0; m != connector->count_modes; ++m) {
            drmModeModeInfo &mode_info = connector->modes[m];

            if (strcmp(mode_info.name, "preferred") == 0)
                current_mode_index = m;
        }
    }

    // There's no need to warn about failing to find a current display mode on a disconnected display.
    if (connected && (current_mode_index == invalid_mode_index)) {
        mir::log_warning(
            "Unable to determine the current display mode.");
    }

    output.type = type;
    output.modes = modes;
    output.preferred_mode_index = preferred_mode_index;
    output.physical_size_mm = physical_size;
    output.connected = connected;
    output.current_format = mir_pixel_format_xrgb_8888;
    output.current_mode_index = current_mode_index;
    output.subpixel_arrangement = kms_subpixel_to_mir_subpixel(connector->subpixel);
    output.gamma = gamma;
    output.edid = edid;
}

namespace
{
void bo_user_data_destroy(gbm_bo* /*bo*/, void *data)
{
    auto bufobj = static_cast<std::shared_ptr<mgg::FBHandle const>*>(data);
    delete bufobj;
}
}

auto mgg::RealKMSOutput::FBRegistry::lookup_or_create(int const drm_fd, gbm_bo* bo)
    -> std::shared_ptr<FBHandle const>
{
    if (!bo)
        return nullptr;

    /*
     * Check if we have already set up this gbm_bo (the gbm-kms implementation is
     * free to reuse gbm_bos). If so, return the associated FBHandle.
     */
    auto bufobj = static_cast<std::shared_ptr<FBHandle const>*>(gbm_bo_get_user_data(bo));
    if (bufobj)
    {
        return *bufobj;
    }

    uint32_t fb_id{0};
    uint32_t handles[4] = {gbm_bo_get_handle(bo).u32, 0, 0, 0};
    uint32_t strides[4] = {gbm_bo_get_stride(bo), 0, 0, 0};
    uint32_t offsets[4] = {0, 0, 0, 0};

    auto format = gbm_bo_get_format(bo);
    /*
     * Mir might use the old GBM_BO_ enum formats, but KMS and the rest of
     * the world need fourcc formats, so convert...
     */
    if (format == GBM_BO_FORMAT_XRGB8888)
        format = GBM_FORMAT_XRGB8888;
    else if (format == GBM_BO_FORMAT_ARGB8888)
        format = GBM_FORMAT_ARGB8888;

    auto const width = gbm_bo_get_width(bo);
    auto const height = gbm_bo_get_height(bo);

    /* Create a KMS FB object with the gbm_bo attached to it. */
    auto ret = drmModeAddFB2(drm_fd, width, height, format,
                             handles, strides, offsets, &fb_id, 0);
    if (ret)
        return nullptr;

    /* Create a FBHandle and associate it with the gbm_bo */

    bufobj = new std::shared_ptr<FBHandle const>(new FBHandle{drm_fd, fb_id});
    gbm_bo_set_user_data(bo, bufobj, bo_user_data_destroy);

    return *bufobj;
}

auto mgg::RealKMSOutput::fb_for(gbm_bo* bo) const -> std::shared_ptr<FBHandle const>
{
    return framebuffers.lookup_or_create(drm_fd(), bo);
}

struct mgg::RealKMSOutput::FBRegistry::DMABufFB
{
    std::array<Fd, 4> const fds;
    std::array<uint32_t, 4> const bo_handles;
    std::weak_ptr<FBHandle const> handle;

    DMABufFB(
        std::array<Fd, 4> fds,
        std::array<uint32_t, 4> bo_handles,
        std::weak_ptr<FBHandle const> handle)
        : fds{std::move(fds)},
          bo_handles{bo_handles},
          handle{std::move(handle)}
    {
    }
};

auto mgg::RealKMSOutput::FBRegistry::lookup_or_create(
    const int drm_fd,
    const DMABufBuffer& image) -> std::shared_ptr<FBHandle const>
{
    // The DRM API expects a bunch of 4-element arrays, with unused elements set to 0
    std::array<uint32_t, 4> handles = {0, 0, 0, 0};
    std::array<uint32_t, 4> strides = {0, 0, 0, 0};
    std::array<uint32_t, 4> offsets = {0, 0, 0, 0};
    std::array<uint64_t, 4> modifiers = {0, 0, 0, 0};

    std::array<Fd, 4> dma_bufs;

    auto const& planes = image.planes();
    for (auto i = 0u; i < planes.size() ; ++i)
    {
        strides[i] = planes[i].stride;
        offsets[i] = planes[i].offset;
        dma_bufs[i] = planes[i].dma_buf;
    }

    // If we've got this FB already imported, we can just return that…
    auto const existing_fb = std::find_if(
        dmabuf_fbs.begin(),
        dmabuf_fbs.end(),
        [&dma_bufs](auto const& candidate)
        {
            for (auto i = 0u; i < dma_bufs.size(); ++i)
            {
                /* We can do a numerical compare here because the DMAbufFB structs keep a
                 * reference to their fds open, so any newly imported buffer will have
                 * a different integer fd handle; they can't accidentally alias.
                 *
                 * And the Wayland protocol only imports the FDs once, and then the client
                 * references the wl_buffer, so we can expect the FDs of a particular buffer
                 * to be numerically stable.
                 */
                if (static_cast<int>(dma_bufs[i]) != static_cast<int>(candidate->fds[i]))
                {
                    return false;
                }
            }
            return true;
        });

    uint32_t const* imported_handles;
    if (existing_fb != dmabuf_fbs.end())
    {
        if (auto const handle = (*existing_fb)->handle.lock())
        {
            /* We have already imported the DMA-bufs *and* have a current FB for them.
             * We can just return that.
             */
            return handle;
        }
        /* We've imported the DMA-bufs, but have already deleted the FB associated with
         * them. Grab the previously-imported handles, then re-create a FB.
         */
        imported_handles = (*existing_fb)->bo_handles.data();
    }
    else
    {
        // Otherwise, we need to import the DMA-bufs
        for (auto i = 0u; i < planes.size(); ++i)
        {
            if (auto const error = drmPrimeFDToHandle(drm_fd, dma_bufs[i], &handles[i]))
            {
                BOOST_THROW_EXCEPTION((
                                          std::system_error{
                                              error,
                                              std::system_category(),
                                              "Failed to acquire GEM handle for DMA-BUF"}));
            }
        }
        imported_handles = handles.data();
    }


    // If there's a modifier set, propagate it to all components.
    if (image.modifier().has_value())
    {
        for (auto i = 0u; i < planes.size(); ++i)
        {
            modifiers[i] = image.modifier().value();
        }
    }

    uint32_t drm_fb_id;
    auto const ret = drmModeAddFB2WithModifiers(
        drm_fd,
        image.size().width.as_uint32_t(),
        image.size().height.as_uint32_t(),
        image.drm_fourcc(),
        imported_handles,
        strides.data(),
        offsets.data(),
        image.modifier().has_value() ? modifiers.data() : nullptr,
        &drm_fb_id,
        image.modifier().has_value() ? DRM_MODE_FB_MODIFIERS : 0);

    if (ret)
    {
        mir::log_debug(
            "Failed to import dmabuf-based image as FB: %s",
            std::system_category().message(ret).c_str());
        return nullptr;
    }

    auto fb_handle = std::make_shared<FBHandle>(drm_fd, drm_fb_id);

    if (existing_fb != dmabuf_fbs.end())
    {
        // We already have the buffer data; we just need to re-create a handle
        (*existing_fb)->handle = fb_handle;
    }
    else
    {
        dmabuf_fbs.push_back(std::make_shared<DMABufFB>(std::move(dma_bufs), handles, fb_handle));
    }

    return fb_handle;
}

auto mgg::RealKMSOutput::fb_for(mg::DMABufBuffer const& image) const -> std::shared_ptr<FBHandle const>
{
    return framebuffers.lookup_or_create(drm_fd(), image);
}

bool mgg::RealKMSOutput::buffer_requires_migration(gbm_bo* bo) const
{
    /*
     * It's possible that some devices will not require migration -
     * Intel GPUs can obviously scanout from main memory, as can USB outputs such as
     * DisplayLink.
     *
     * For a first go, just say that *every* device scans out of GPU-private memory.
     *
     * To complicate matters, Mali's gbm-kms implementation does *not* return the same
     * integer fd from gbm_device_get_fd() as the drm fd that the device was created
     * from, and GBM may choose to internally open the DRM render node associated
     * with the DRM node passed to gbm_create_device.
     */

    errno = 0;
    auto const gbm_device_node = std::unique_ptr<char, decltype(&free)>{
        drmGetPrimaryDeviceNameFromFd(gbm_device_get_fd(gbm_bo_get_device(bo))),
        &free
    };
    if (!gbm_device_node)
    {
        BOOST_THROW_EXCEPTION((
            std::system_error{
                errno,
                std::system_category(),
                "Failed to query DRM device backing GBM buffer"}));
    }

    auto const drm_device_node = std::unique_ptr<char, decltype(&free)>{
        drmGetPrimaryDeviceNameFromFd(drm_fd_),
        &free
    };
    if (!drm_device_node)
    {
        BOOST_THROW_EXCEPTION((
            std::system_error{
                errno,
                std::system_category(),
                "Failed to query DRM device node of display device"}));
    }

    // These *should* match if we're on the same device
    return strcmp(gbm_device_node.get(), drm_device_node.get()) != 0;
}

int mgg::RealKMSOutput::drm_fd() const
{
    return drm_fd_;
}
