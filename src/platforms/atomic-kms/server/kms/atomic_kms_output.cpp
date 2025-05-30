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

#include "atomic_kms_output.h"
#include "kms-utils/drm_mode_resources.h"
#include "kms_framebuffer.h"
#include "mir/graphics/display_configuration.h"
#include "mir/graphics/drm_formats.h"
#include "mir/graphics/gamma_curves.h"
#include "mir_toolkit/common.h"
#include "kms-utils/kms_connector.h"
#include "mir/fatal.h"
#include "mir/log.h"
#include <drm_fourcc.h>
#include <drm_mode.h>
#include <span>
#include <string.h> // strcmp

#include <boost/throw_exception.hpp>
#include <system_error>
#include <xf86drm.h>
#include <xf86drmMode.h>

namespace mg = mir::graphics;
namespace mga = mg::atomic;
namespace mgk = mg::kms;
namespace geom = mir::geometry;

namespace
{
bool kms_modes_are_equal(drmModeModeInfo const* info1, drmModeModeInfo const* info2)
{
    return (info1 && info2) &&
        std::tie(info1->clock, info1->hdisplay, info1->hsync_start, info1->hsync_end, info1->htotal, info1->hskew, info1->vdisplay, info1->vsync_start, info1->vsync_end, info1->vtotal) ==
        std::tie(info2->clock, info2->hdisplay, info2->hsync_start, info2->hsync_end, info2->htotal, info2->hskew, info2->vdisplay, info2->vsync_start, info2->vsync_end, info2->vtotal);
}

uint32_t create_blob_returning_handle(mir::Fd const& drm_fd, void const* data, size_t len)
{
    uint32_t handle;
    if (auto err = drmModeCreatePropertyBlob(drm_fd, data, len, &handle))
    {
        BOOST_THROW_EXCEPTION((
            std::system_error{
                -err,
                std::system_category(),
                "Failed to create DRM property blob"}));
    }
    return handle;
}

class PropertyBlobData
{
public:
    PropertyBlobData(mir::Fd const& drm_fd, uint32_t handle)
        : ptr{drmModeGetPropertyBlob(drm_fd, handle)}
    {
        if (!ptr)
        {
            // drmModeGetPropertyBlob sets errno on failure, except on allocation failure
            auto const err = errno ? errno : ENOMEM;
            BOOST_THROW_EXCEPTION((
                std::system_error{
                    err,
                    std::system_category(),
                    "Failed to read DRM property blob"}));
        }
    }

    ~PropertyBlobData()
    {
        drmModeFreePropertyBlob(ptr);
    }

    template<typename T>
    auto data() const -> std::span<T const>
    {
        /* This is a precondition check, so technically unnecessary.
         * That said, there are a bunch of moving parts here, so
         * let's be nice and check what little we can.
         */
        if (ptr->length % sizeof(T) != 0)
        {
            BOOST_THROW_EXCEPTION((
                std::runtime_error{
                    std::format("DRM property size {} is not a multiple of expected object size {}",
                        ptr->length,
                        sizeof(T))}));
        }

        /* We don't have to care about alignment, at least; libdrm will
         * have copied the data into a suitably-aligned allocation
         */
        return std::span{static_cast<T const*>(ptr->data), ptr->length / sizeof(T)};
    }

    auto raw() const -> drmModePropertyBlobRes const*
    {
        return ptr;
    }
private:
    drmModePropertyBlobPtr const ptr;
};

class AtomicUpdate
{
public:
    AtomicUpdate()
        : req{drmModeAtomicAlloc()}
    {
        if (!req)
        {
            BOOST_THROW_EXCEPTION((
                std::runtime_error{"Failed to allocate Atomic DRM update request"}));
        }
    }

    ~AtomicUpdate()
    {
        drmModeAtomicFree(req);
    }

    operator drmModeAtomicReqPtr() const
    {
        return req;
    }

    void add_property(mgk::ObjectProperties const& properties, char const* prop_name, uint64_t value)
    {
        drmModeAtomicAddProperty(req, properties.parent_id(), properties.id_for(prop_name), value);
    }
private:
    drmModeAtomicReqPtr const req;
};
}

class mga::AtomicKMSOutput::PropertyBlob
{
public:
    PropertyBlob(mir::Fd drm_fd, void const* data, size_t size)
        : handle_{create_blob_returning_handle(drm_fd, data, size)},
          drm_fd{std::move(drm_fd)}
    {
    }

    ~PropertyBlob()
    {
        if (auto err = drmModeDestroyPropertyBlob(drm_fd, handle_))
        {
            mir::log_warning(
                "Failed to free DRM property blob: %s (%i)",
                strerror(-err),
                -err);
        }
    }

    uint32_t handle() const
    {
        return handle_;
    }
private:
    uint32_t const handle_;
    mir::Fd const drm_fd;
};

/* Note:
 * (At least) Clang includes -Wmissing-designated-field-initializers to -Wextra
 * This (questionably?) warns when using designated field initialisers but
 * not explicitly initialising every field member.
 * (The standard says anything not specified is ~default-initialised)
 *
 * That's why the Configuration initialiser contains so many default values here
 */
mga::AtomicKMSOutput::AtomicKMSOutput(
    mir::Fd drm_master,
    kms::DRMModeConnectorUPtr connector)
    : drm_fd_{drm_master},
      configuration{
          Configuration {
          .connector = std::move(connector),
          .mode_index = 0,
          .fb_offset = {},
          .current_crtc = nullptr,
          .current_plane = nullptr,
          .mode = nullptr,
          .crtc_props = nullptr,
          .plane_props = nullptr,
          .connector_props = nullptr
      }},
      saved_crtc(),
      using_saved_crtc{true}
{
    reset();

    kms::DRMModeResources resources{drm_fd_};

    if (auto conf = configuration.lock(); conf->connector->encoder_id)
    {
        auto encoder = resources.encoder(conf->connector->encoder_id);
        if (encoder->crtc_id)
        {
            saved_crtc = *resources.crtc(encoder->crtc_id);
        }
    }
}

mga::AtomicKMSOutput::~AtomicKMSOutput()
{
    restore_saved_crtc();
}

uint32_t mga::AtomicKMSOutput::id() const
{
    return configuration.lock()->connector->connector_id;
}

void mga::AtomicKMSOutput::reset()
{
    kms::DRMModeResources resources{drm_fd_};

    /* Update the connector to ensure we have the latest information */
    auto conf = configuration.lock();
    try
    {
        conf->connector = resources.connector(conf->connector->connector_id);
        conf->connector_props = std::make_unique<mgk::ObjectProperties>(drm_fd_, conf->connector);

        if (conf->current_crtc && (conf->connector->connection != DRM_MODE_CONNECTED))
        {
            AtomicUpdate update;
            update.add_property(*conf->connector_props, "CRTC_ID", 0);
            update.add_property(*conf->crtc_props, "ACTIVE", 0);
            update.add_property(*conf->crtc_props, "MODE_ID", 0);
            update.add_property(*conf->plane_props, "FB_ID", 0);
            update.add_property(*conf->plane_props, "CRTC_ID", 0);

            if (auto err = drmModeAtomicCommit(drm_fd(), update, DRM_MODE_ATOMIC_ALLOW_MODESET, nullptr))
            {
                mir::log_warning("Failed release resources for disconnected output: %s (%i)", strerror(-err), -err);
            }

            conf->crtc_props = nullptr;
            conf->plane_props = nullptr;
        }
    }
    catch (std::exception const& e)
    {
        fatal_error(e.what());
    }

    /* Discard previously current crtc */
    conf->current_crtc = nullptr;
}

geom::Size mga::AtomicKMSOutput::size() const
{
    auto conf = configuration.lock();
    // Disconnected hardware has no modes: invent a size
    if (conf->connector->connection == DRM_MODE_DISCONNECTED)
        return {0, 0};

    drmModeModeInfo const& mode(conf->connector->modes[conf->mode_index]);
    return {mode.hdisplay, mode.vdisplay};
}

int mga::AtomicKMSOutput::max_refresh_rate() const
{
    auto conf = configuration.lock();
    if (conf->connector->connection == DRM_MODE_DISCONNECTED)
        return 1;

    drmModeModeInfo const& current_mode = conf->connector->modes[conf->mode_index];
    return current_mode.vrefresh;
}

void mga::AtomicKMSOutput::configure(geom::Displacement offset, size_t kms_mode_index)
{
    auto conf = configuration.lock();
    conf->fb_offset = offset;
    conf->mode_index = kms_mode_index;
    conf->mode = std::make_unique<PropertyBlob>(
        drm_fd_,
        &conf->connector->modes[conf->mode_index],
        sizeof(conf->connector->modes[conf->mode_index]));
    ensure_crtc(*conf);
}

bool mga::AtomicKMSOutput::set_crtc(FBHandle const& fb)
{
    auto conf = configuration.lock();
    if (!ensure_crtc(*conf))
    {
        mir::log_error("Output %s has no associated CRTC to set a framebuffer on",
                       mgk::connector_name(conf->connector).c_str());
        return false;
    }

    /* We use the *requested* mode rather than the current_crtc
     * because we might have been asked to perform a modeset
     */
    auto const width = conf->connector->modes[conf->mode_index].hdisplay;
    auto const height = conf->connector->modes[conf->mode_index].vdisplay;

    AtomicUpdate update;
    update.add_property(*conf->crtc_props, "MODE_ID", conf->mode->handle());
    update.add_property(*conf->connector_props, "CRTC_ID", conf->current_crtc->crtc_id);
    update.add_property(*conf->crtc_props, "ACTIVE", 1);

    /* Source viewport. Coordinates are 16.16 fixed point format */
    update.add_property(*conf->plane_props, "SRC_X", conf->fb_offset.dx.as_uint32_t() << 16);
    update.add_property(*conf->plane_props, "SRC_Y", conf->fb_offset.dy.as_uint32_t() << 16);
    update.add_property(*conf->plane_props, "SRC_W", width << 16);
    update.add_property(*conf->plane_props, "SRC_H", height << 16);

    /* Destination viewport. Coordinates are *not* 16.16 */
    update.add_property(*conf->plane_props, "CRTC_X", 0);
    update.add_property(*conf->plane_props, "CRTC_Y", 0);
    update.add_property(*conf->plane_props, "CRTC_W", width);
    update.add_property(*conf->plane_props, "CRTC_H", height);

    /* Set a surface for the plane */
    update.add_property(*conf->plane_props, "CRTC_ID", conf->current_crtc->crtc_id);
    update.add_property(*conf->plane_props, "FB_ID", fb);

    auto ret = drmModeAtomicCommit(drm_fd_, update, DRM_MODE_ATOMIC_ALLOW_MODESET, nullptr);
    if (ret)
    {
        mir::log_error("Failed to set CRTC: %s (%i)", strerror(-ret), -ret);
        conf->current_crtc = nullptr;
        return false;
    }

    // We might have performed a modeset; update our view of the hardware state accordingly!
    conf->current_crtc = mgk::get_crtc(drm_fd_, conf->current_crtc->crtc_id);

    using_saved_crtc = false;
    return true;
}

bool mga::AtomicKMSOutput::has_crtc_mismatch()
{
    auto conf = configuration.lock();
    if (!ensure_crtc(*conf))
    {
        mir::log_error("Output %s has no associated CRTC to get ", mgk::connector_name(conf->connector).c_str());
        return true;
    }

    return !kms_modes_are_equal(&conf->current_crtc->mode, &conf->connector->modes[conf->mode_index]);
}

void mga::AtomicKMSOutput::clear_crtc()
{
    auto conf = configuration.lock();
    try
    {
        ensure_crtc(*conf);
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

    AtomicUpdate update;
    update.add_property(*conf->connector_props, "CRTC_ID", 0);
    update.add_property(*conf->crtc_props, "ACTIVE", 0);
    update.add_property(*conf->crtc_props, "MODE_ID", 0);
    update.add_property(*conf->plane_props, "FB_ID", 0);
    update.add_property(*conf->plane_props, "CRTC_ID", 0);

    auto result = drmModeAtomicCommit(drm_fd_, update, DRM_MODE_ATOMIC_ALLOW_MODESET, nullptr);
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
                mgk::connector_name(conf->connector).c_str(),
                strerror(-result),
                -result);
        }
        else
        {
            fatal_error("Couldn't clear output %s (drmModeSetCrtc = %d)",
                        mgk::connector_name(conf->connector).c_str(), result);
        }
    }

    conf->current_crtc = nullptr;
}

bool mga::AtomicKMSOutput::page_flip(FBHandle const& fb)
{
    auto conf = configuration.lock();
    if (!ensure_crtc(*conf))
    {
        mir::log_error("Output %s has no associated CRTC to set a framebuffer on",
                       mgk::connector_name(conf->connector).c_str());
        return false;
    }

    auto const crtc_width = conf->current_crtc->width;
    auto const crtc_height = conf->current_crtc->height;

    auto const fb_width = fb.size().width.as_uint32_t();
    auto const fb_height = fb.size().height.as_uint32_t();

    if ((crtc_width != fb_width) || (crtc_height != fb_height))
    {
        /* If the submitted FB isn't the same size as the output, we need
         * a modeset, which can't be done as a pageflip.
         *
         * The calling code should detect a failure to pagefilp, and fall back
         * on `set_crtc`.
         */
        return false;
    }

    AtomicUpdate update;
    update.add_property(*conf->crtc_props, "MODE_ID", conf->mode->handle());
    update.add_property(*conf->connector_props, "CRTC_ID", conf->current_crtc->crtc_id);

    /* Source viewport. Coordinates are 16.16 fixed point format */
    update.add_property(*conf->plane_props, "SRC_X", conf->fb_offset.dx.as_uint32_t() << 16);
    update.add_property(*conf->plane_props, "SRC_Y", conf->fb_offset.dy.as_uint32_t() << 16);
    update.add_property(*conf->plane_props, "SRC_W", fb_width << 16);
    update.add_property(*conf->plane_props, "SRC_H", fb_height << 16);

    /* Destination viewport. Coordinates are *not* 16.16 */
    update.add_property(*conf->plane_props, "CRTC_X", 0);
    update.add_property(*conf->plane_props, "CRTC_Y", 0);
    update.add_property(*conf->plane_props, "CRTC_W", crtc_width);
    update.add_property(*conf->plane_props, "CRTC_H", crtc_height);

    /* Set a surface for the plane */
    update.add_property(*conf->plane_props, "CRTC_ID", conf->current_crtc->crtc_id);
    update.add_property(*conf->plane_props, "FB_ID", fb);

    auto ret = drmModeAtomicCommit(
        drm_fd_,
        update,
        0,
        nullptr);
    if (ret)
    {
        mir::log_error("Failed to schedule page flip: %s (%i)", strerror(-ret), -ret);
        conf->current_crtc = nullptr;
        return false;
    }

    using_saved_crtc = false;
    return true;
}

void mga::AtomicKMSOutput::set_cursor_image(gbm_bo* buffer)
{
    if (auto conf = configuration.lock(); conf->current_crtc)
    {
        if (auto result = drmModeSetCursor(
            drm_fd_,
            conf->current_crtc->crtc_id,
            gbm_bo_get_handle(buffer).u32,
            gbm_bo_get_width(buffer),
            gbm_bo_get_height(buffer)))
        {
            mir::log_warning("set_cursor: drmModeSetCursor failed (%s)", strerror(-result));
        }
        cursor_image_set = true;
    }
}

void mga::AtomicKMSOutput::move_cursor(geometry::Point destination)
{
    if (auto conf = configuration.lock(); conf->current_crtc)
    {
        if (auto result =
                drmModeMoveCursor(drm_fd_, conf->current_crtc->crtc_id, destination.x.as_int(), destination.y.as_int()))
        {
            mir::log_warning("move_cursor: drmModeMoveCursor failed (%s)", strerror(-result));
        }
    }
}

bool mga::AtomicKMSOutput::clear_cursor()
{
    int result = 0;
    if (auto conf = configuration.lock(); conf->current_crtc)
    {
        result = drmModeSetCursor(drm_fd_, conf->current_crtc->crtc_id, 0, 0, 0);

        if (result)
            mir::log_warning("clear_cursor: drmModeSetCursor failed (%s)", strerror(-result));
    }

    cursor_image_set = false;

    return !result;
}

bool mga::AtomicKMSOutput::ensure_crtc(Configuration& to_update)
{
    /* Nothing to do if we already have a crtc */
    if (to_update.current_crtc)
        return true;

    /* If the output is not connected there is nothing to do */
    if (to_update.connector->connection != DRM_MODE_CONNECTED)
        return false;

    // Update the connector as we may unexpectedly fail in find_crtc_and_index_for_connector()
    // https://github.com/canonical/mir/issues/2661
    to_update.connector = kms::get_connector(drm_fd_, to_update.connector->connector_id);

    if (to_update.connector->connection != DRM_MODE_CONNECTED)
    {
        return false;
    }

    std::tie(to_update.current_crtc, to_update.current_plane) = mgk::find_crtc_with_primary_plane(drm_fd_, to_update.connector);
    if (!to_update.current_crtc || !to_update.current_plane)
    {
        return false;
    }

    to_update.crtc_props = std::make_unique<mgk::ObjectProperties>(drm_fd_, to_update.current_crtc);
    to_update.plane_props = std::make_unique<mgk::ObjectProperties>(drm_fd_, to_update.current_plane);

    return true;
}

void mga::AtomicKMSOutput::restore_saved_crtc()
{
    if (auto conf = configuration.lock(); !using_saved_crtc)
    {
        drmModeSetCrtc(drm_fd_, saved_crtc.crtc_id, saved_crtc.buffer_id,
                       saved_crtc.x, saved_crtc.y,
                       &conf->connector->connector_id, 1, &saved_crtc.mode);

        using_saved_crtc = true;
    }
}

void mga::AtomicKMSOutput::set_power_mode(MirPowerMode mode)
{
    bool should_be_active = mode == mir_power_mode_on;

    if (auto conf = configuration.lock(); conf->current_crtc)
    {
        AtomicUpdate update;
        update.add_property(*conf->crtc_props, "ACTIVE", should_be_active);
        if (auto err = drmModeAtomicCommit(drm_fd_, update, DRM_MODE_ATOMIC_ALLOW_MODESET, nullptr))
        {
            mir::log_warning("Failed to set DPMS %s (%s [%i])", should_be_active ? "active" : "off", strerror(-err), -err);
        }
    }
    else if (should_be_active)
    {
        mir::log_error("Attempted to set mir_power_mode_on for unconfigured output");
    }
    // An output which doesn't have a CRTC configured is already off, so that's not an error.
}

void mga::AtomicKMSOutput::set_gamma(mg::GammaCurves const& gamma)
{
    if (!gamma.red.size())
    {
        mir::log_warning("Ignoring attempt to set zero length gamma");
        return;
    }

    auto conf = configuration.lock();
    if (!ensure_crtc(*conf))
    {
        mir::log_warning("Output %s has no associated CRTC to set gamma on",
                         mgk::connector_name(conf->connector).c_str());
        return;
    }

    if (gamma.red.size() != gamma.green.size() ||
        gamma.green.size() != gamma.blue.size())
    {
        BOOST_THROW_EXCEPTION(
            std::invalid_argument("set_gamma: mismatch gamma LUT sizes"));
    }

    auto drm_lut = std::make_unique<struct drm_color_lut[]>(gamma.red.size());
    for (size_t i = 0; i < gamma.red.size(); ++i)
    {
        drm_lut[i].red = gamma.red[i];
        drm_lut[i].green = gamma.green[i];
        drm_lut[i].blue = gamma.blue[i];
    }
    PropertyBlob lut{drm_fd_, drm_lut.get(), sizeof(struct drm_color_lut) * gamma.red.size()};
    AtomicUpdate update;

    update.add_property(*conf->crtc_props, "GAMMA_LUT", lut.handle());
    drmModeAtomicCommit(drm_fd(), update, DRM_MODE_ATOMIC_ALLOW_MODESET, nullptr);
}

void mga::AtomicKMSOutput::refresh_hardware_state()
{
    auto conf = configuration.lock();
    conf->connector = kms::get_connector(drm_fd_, conf->connector->connector_id);

    if (conf->current_crtc && (conf->connector->connection != DRM_MODE_CONNECTED))
    {
        AtomicUpdate update;
        update.add_property(*conf->connector_props, "CRTC_ID", 0);
        update.add_property(*conf->crtc_props, "ACTIVE", 0);
        update.add_property(*conf->crtc_props, "MODE_ID", 0);
        update.add_property(*conf->plane_props, "FB_ID", 0);
        update.add_property(*conf->plane_props, "CRTC_ID", 0);

        drmModeAtomicCommit(drm_fd(), update, DRM_MODE_ATOMIC_ALLOW_MODESET, nullptr);

        conf->connector_props = nullptr;
        conf->crtc_props = nullptr;
        conf->plane_props = nullptr;
    }
    conf->current_crtc = nullptr;

    if (conf->connector->encoder_id)
    {
        auto encoder = kms::get_encoder(drm_fd_, conf->connector->encoder_id);

        if (encoder->crtc_id)
        {
            conf->current_crtc = kms::get_crtc(drm_fd_, encoder->crtc_id);
        }
    }
}

namespace
{
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

std::vector<uint8_t> edid_for_connector(mir::Fd const& drm_fd, uint32_t connector_id)
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

        // A property value of 0 means invalid.
        if (connector_props["EDID"] == 0)
        {
            /*
             * Log a debug message only. This will trigger for broken monitors which
             * don't provide an EDID, which is not as unusual as you might think...
             */
            mir::log_debug("No EDID data available on connector %u", connector_id);
            return edid;
        }

        try
        {
            PropertyBlobData edid_blob{drm_fd, static_cast<uint32_t>(connector_props["EDID"])};
            edid.reserve(edid_blob.data<uint8_t>().size_bytes());
            edid.assign(edid_blob.data<uint8_t>().begin(), edid_blob.data<uint8_t>().end());

            edid.shrink_to_fit();
        }
        catch (std::system_error const& err)
        {
            // Failing to read the EDID property is weird, but shouldn't be fatal
            mir::log_warning(
                "Failed to get EDID property blob for connector %u: %i (%s)",
                connector_id,
                err.code().value(),
                err.what());

            return edid;
        }
    }

    return edid;
}

auto formats_for_output(mir::Fd const& drm_fd, mgk::DRMModeConnectorUPtr const& connector) -> std::vector<mg::DRMFormat>
{
    auto [_, plane] = mgk::find_crtc_with_primary_plane(drm_fd, connector);

    mgk::ObjectProperties plane_props{drm_fd, plane->plane_id, DRM_MODE_OBJECT_PLANE};

    if (!plane_props.has_property("IN_FORMATS"))
    {
        return {mg::DRMFormat{DRM_FORMAT_ARGB8888}, mg::DRMFormat{DRM_FORMAT_XRGB8888} };
    }

    PropertyBlobData format_blob{drm_fd, static_cast<uint32_t>(plane_props["IN_FORMATS"])};
    drmModeFormatModifierIterator iter{};

    std::vector<mg::DRMFormat> supported_formats;
    while (drmModeFormatModifierBlobIterNext(format_blob.raw(), &iter))
    {
        /* This will iterate over {format, modifier} pairs, with all the modifiers for a single
         * format in a block. For example:
         * {fmt1, mod1}
         * {fmt1, mod2}
         * {fmt1, mod3}
         * {fmt2, mod2}
         * {fmt2, mod4}
         * ...
         *
         * We only care about the format, so we only add when we see a new format
         */
        if (supported_formats.empty() || supported_formats.back() != iter.fmt)
        {
            supported_formats.emplace_back(iter.fmt);
        }
    }
    return supported_formats;
}
}

void mga::AtomicKMSOutput::update_from_hardware_state(
    DisplayConfigurationOutput& output) const
{
    auto conf = configuration.lock();

    auto const& connector = conf->connector;
    auto const& current_crtc = conf->current_crtc;
    auto const& crtc_props = conf->crtc_props;

    DisplayConfigurationOutputType const type{
        kms_connector_type_to_output_type(connector->connector_type)};
    geom::Size physical_size{connector->mmWidth, connector->mmHeight};
    bool connected{connector->connection == DRM_MODE_CONNECTED};
    uint32_t const invalid_mode_index = std::numeric_limits<uint32_t>::max();
    uint32_t current_mode_index{invalid_mode_index};
    uint32_t preferred_mode_index{invalid_mode_index};
    std::vector<DisplayConfigurationMode> modes;
    std::vector<MirPixelFormat> formats;

    auto supported_formats = formats_for_output(drm_fd_, connector);
    formats.reserve(supported_formats.size());
    for (auto const& format : supported_formats)
    {
        if (auto mir_format = format.as_mir_format())
        {
            formats.push_back(*mir_format);
        }
    }

    std::vector<uint8_t> edid;
    if (connected)
    {
        /* Only ask for the EDID on connected outputs. There's obviously no monitor EDID
         * when there is no monitor connected!
         */
        edid = edid_for_connector(drm_fd_, connector->connector_id);
    }

    auto const current_mode_info =
        [&]() -> drmModeModeInfo const*
        {
            if (current_crtc)
            {
                return &current_crtc->mode;
            }
            return nullptr;
        }();

    GammaCurves gamma;
    if (connected && current_crtc && crtc_props->has_property("GAMMA_LUT") && crtc_props->has_property("GAMMA_LUT_SIZE"))
    {
        try
        {
            PropertyBlobData gamma_lut{drm_fd_, static_cast<uint32_t>((*crtc_props)["GAMMA_LUT"])};
            auto const gamma_size = gamma_lut.data<struct drm_color_lut>().size();
            gamma.red.reserve(gamma_size);
            gamma.green.reserve(gamma_size);
            gamma.blue.reserve(gamma_size);
            for (auto const& entry : gamma_lut.data<struct drm_color_lut>())
            {
                gamma.red.push_back(entry.red);
                gamma.green.push_back(entry.green);
                gamma.blue.push_back(entry.blue);
            }
        }
        catch (...)
        {
            log(logging::Severity::warning, MIR_LOG_COMPONENT, std::current_exception(), "Failed to get gamma curves");
        }
    }

    /* Add all the available modes and find the current and preferred one */
    for (int m = 0; m < connector->count_modes; m++) {
        drmModeModeInfo const& mode_info = connector->modes[m];

        geom::Size size{mode_info.hdisplay, mode_info.vdisplay};

        double vrefresh_hz = calculate_vrefresh_hz(mode_info);

        modes.push_back({size, vrefresh_hz});

        if (kms_modes_are_equal(&mode_info, current_mode_info))
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

int mga::AtomicKMSOutput::drm_fd() const
{
    return drm_fd_;
}

bool mga::AtomicKMSOutput::has_cursor_image() const
{
    return cursor_image_set;
}

