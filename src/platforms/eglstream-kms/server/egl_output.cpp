/*
 * Copyright © 2016 Canonical Ltd.
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
 *
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include <epoxy/egl.h>

#include "egl_output.h"
#include "mir/graphics/egl_error.h"
#include "kms-utils/kms_connector.h"

#include <cstring>
#include <drm.h>
#include <sys/ioctl.h>
#include <vector>
#include <boost/throw_exception.hpp>
#include <unordered_map>
#include <tuple>
#include <sys/mman.h>

namespace mg = mir::graphics;
namespace mge = mg::eglstream;
namespace mgek = mge::kms;
namespace mgk = mg::kms;
namespace geom = mir::geometry;

namespace
{
namespace
{
class DumbFb
{
public:
    DumbFb(int drm_fd, uint32_t width, uint32_t height)
        : drm_fd{drm_fd}
    {
        struct drm_mode_create_dumb params = {};

        params.bpp = 32;
        params.height = height;
        params.width = width;

        if (ioctl(drm_fd, DRM_IOCTL_MODE_CREATE_DUMB, &params) != 0)
        {
            BOOST_THROW_EXCEPTION((std::system_error{errno, std::system_category(), "Failed to create dumb buffer"}));
        }

        gem_handle = params.handle;
        pitch_ = params.pitch;

        auto ret = drmModeAddFB(drm_fd, width, height, 24, 32, params.pitch, params.handle, &fb_id);
        if (ret)
        {
            BOOST_THROW_EXCEPTION((std::system_error{-ret, std::system_category(), "Failed to attach dumb buffer to FB"}));
        }

        struct drm_mode_map_dumb map_request = {};

        map_request.handle = gem_handle;

        ret = drmIoctl(drm_fd, DRM_IOCTL_MODE_MAP_DUMB, &map_request);
        if (ret)
        {
            BOOST_THROW_EXCEPTION((std::system_error{-ret, std::system_category(), "Failed to map dumb buffer"}));
        }

        auto map = mmap(0, params.size, PROT_READ | PROT_WRITE, MAP_SHARED, drm_fd, map_request.offset);
        if (map == MAP_FAILED)
        {
            BOOST_THROW_EXCEPTION((std::system_error{errno, std::system_category(), "Failed to mmap() buffer"}));
        }

        ::memset(map, 0, pitch_ * height);
    }
    ~DumbFb() noexcept(false)
    {
        struct drm_mode_destroy_dumb params = { gem_handle };

        if (ioctl(drm_fd, DRM_IOCTL_MODE_DESTROY_DUMB, &params) != 0)
        {
            if (!std::uncaught_exception())
            {
                BOOST_THROW_EXCEPTION((std::system_error{errno, std::system_category(), "Failed to destroy dumb buffer"}));
            }
        }
    }

    uint32_t handle() const
    {
        return gem_handle;
    }

    uint32_t pitch() const
    {
        return pitch_;
    }

    uint32_t id() const
    {
        return fb_id;
    }

private:
    int const drm_fd;
    uint32_t fb_id;
    uint32_t gem_handle;
    uint32_t pitch_;
};

}

uint32_t kms_id_from_port(EGLDisplay dpy, EGLOutputPortEXT port)
{
    EGLAttrib kms_connector_id;
    if (eglQueryOutputPortAttribEXT(dpy, port, EGL_DRM_CONNECTOR_EXT, &kms_connector_id) != EGL_TRUE)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to get EGLOutputPort → KMS connector ID mapping"));
    }
    return static_cast<uint32_t>(kms_connector_id);
}

void refresh_connector(int drm_fd, mgk::DRMModeConnectorUPtr& connector)
{
    connector = mgk::get_connector(drm_fd, connector->connector_id);
}
}

mgek::EGLOutput::EGLOutput(
    int drm_fd,
    EGLDisplay dpy,
    EGLOutputPortEXT connector)
    : drm_fd{drm_fd},
      display{dpy},
      port{connector},
      connector{mgk::get_connector(drm_fd, kms_id_from_port(dpy, port))}
{

}

mgek::EGLOutput::~EGLOutput()
{
    try
    {
        restore_saved_crtc();
    }
    catch(std::exception const& e)
    {
        log(logging::Severity::error,
            MIR_LOG_COMPONENT_FALLBACK,
            std::make_exception_ptr(e),
            "Failed to restore saved crtc");
    }
}

void mgek::EGLOutput::reset()
{
    /* Update the connector to ensure we have the latest information */
    refresh_connector(drm_fd, connector);

    // TODO: What if we can't locate the DPMS property?
    // TODO: Replace with mgk::ObjectProperties
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
}

geom::Size mgek::EGLOutput::size() const
{
    drmModeModeInfo const& mode = connector->modes[mode_index];
    return {mode.hdisplay, mode.vdisplay};
}

int mgek::EGLOutput::max_refresh_rate() const
{
    drmModeModeInfo const& current_mode = connector->modes[mode_index];
    return current_mode.vrefresh;
}

void mgek::EGLOutput::configure(size_t kms_mode_index)
{
    mode_index = kms_mode_index;
    auto const width = connector->modes[kms_mode_index].hdisplay;
    auto const height = connector->modes[kms_mode_index].vdisplay;

    std::unique_ptr<drmModeAtomicReq, void(*)(drmModeAtomicReqPtr)>
        request{drmModeAtomicAlloc(), &drmModeAtomicFree};

    mgk::DRMModeCrtcUPtr crtc;
    mgk::DRMModePlaneUPtr plane;

    std::tie(crtc, plane) = mgk::find_crtc_with_primary_plane(drm_fd, connector);
    auto const crtc_id = crtc->crtc_id;

    uint32_t mode_id{0};
    auto ret = drmModeCreatePropertyBlob(
        drm_fd,
        &connector->modes[kms_mode_index],
        sizeof(connector->modes[kms_mode_index]),
        &mode_id);

    if (ret != 0)
    {
        BOOST_THROW_EXCEPTION(
            std::system_error(-ret, std::system_category(), "Failed to create DRM Mode property blob"));
    }

    DumbFb dummy{drm_fd, width, height};

    mgk::ObjectProperties crtc_props{drm_fd, crtc_id, DRM_MODE_OBJECT_CRTC};

    /* Activate the CRTC and set the mode */
    drmModeAtomicAddProperty(request.get(), crtc_id, crtc_props.id_for("MODE_ID"), mode_id);
    drmModeAtomicAddProperty(request.get(), crtc_id, crtc_props.id_for("ACTIVE"), 1);

    /* Set CRTC for the output */
    auto const connector_id = connector->connector_id;
    mgk::ObjectProperties connector_props{drm_fd, connector_id, DRM_MODE_OBJECT_CONNECTOR};
    drmModeAtomicAddProperty(request.get(), connector_id, connector_props.id_for("CRTC_ID"), crtc_id);

    /* Set up the output plane... */
    auto const plane_id = plane->plane_id;
    mgk::ObjectProperties plane_props{drm_fd, plane_id, DRM_MODE_OBJECT_PLANE};

    /* Source viewport. Coordinates are 16.16 fixed point format */
    drmModeAtomicAddProperty(request.get(), plane_id, plane_props.id_for("SRC_X"), 0);
    drmModeAtomicAddProperty(request.get(), plane_id, plane_props.id_for("SRC_Y"), 0);
    drmModeAtomicAddProperty(request.get(), plane_id, plane_props.id_for("SRC_W"), width << 16);
    drmModeAtomicAddProperty(request.get(), plane_id, plane_props.id_for("SRC_H"), height << 16);

    /* Destination viewport. Coordinates are *not* 16.16 */
    drmModeAtomicAddProperty(request.get(), plane_id, plane_props.id_for("CRTC_X"), 0);
    drmModeAtomicAddProperty(request.get(), plane_id, plane_props.id_for("CRTC_Y"), 0);
    drmModeAtomicAddProperty(request.get(), plane_id, plane_props.id_for("CRTC_W"), width);
    drmModeAtomicAddProperty(request.get(), plane_id, plane_props.id_for("CRTC_H"), height);

    /* Set a surface for the plane, and connect to the CRTC */
    drmModeAtomicAddProperty(request.get(), plane_id, plane_props.id_for("FB_ID"), dummy.id());
    drmModeAtomicAddProperty(request.get(), plane_id, plane_props.id_for("CRTC_ID"), crtc_id);

    /* We don't monitor the DRM events (yet), so have no userdata */
    ret = drmModeAtomicCommit(drm_fd, request.get(), DRM_MODE_ATOMIC_ALLOW_MODESET, nullptr);

    if (ret != 0)
    {
        BOOST_THROW_EXCEPTION(
            std::system_error(-ret, std::system_category(), "Failed to commit atomic KMS configuration change"));
    }

    EGLAttrib const crtc_filter[] = {
        EGL_DRM_CRTC_EXT, static_cast<EGLAttrib>(crtc_id),
        EGL_NONE};
    int found_layers{0};
    if (eglGetOutputLayersEXT(display, crtc_filter, &layer, 1, &found_layers) != EGL_TRUE)
    {
        BOOST_THROW_EXCEPTION((
            mg::egl_error(
                std::string{"Failed to find EGLOutputEXT corresponding to DRM CRTC "} +
                std::to_string(crtc_id))));
    }
    if (found_layers != 1)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error{
            std::string{"Failed to find EGLOutputEXT corresponding to DRM CRTC "} +
            std::to_string(crtc_id)});
    }

    using_saved_crtc = false;
}

EGLOutputLayerEXT mgek::EGLOutput::output_layer() const
{
    return layer;
}

uint32_t mgek::EGLOutput::crtc_id() const
{
    EGLAttrib crtc_id;
    if (eglQueryOutputLayerAttribEXT(display, layer, EGL_DRM_CRTC_EXT, &crtc_id)  != EGL_TRUE)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to query DRM CRTC ID from EGLOutputLayer"));
    } else if (crtc_id == 0)
    {
        BOOST_THROW_EXCEPTION((std::runtime_error{"EGLOutputLayer is associated with invalid CRTC?!"}));
    }
    return static_cast<uint32_t>(crtc_id);
}

void mgek::EGLOutput::clear_crtc()
{
    using namespace std::string_literals;
    mgk::DRMModeCrtcUPtr crtc;
    try
    {
        crtc = mgk::find_crtc_for_connector(drm_fd, connector);
    }
    catch (std::runtime_error const&)
    {
        /*
         * In order to actually clear the output, we need to have a crtc
         * connected to the output/connector so that we can disconnect
         * it. However, not being able to get a crtc is OK, since it means
         * that the output cannot be displaying anything anyway.
         */
        return;
    }
    auto const crtc_id = crtc->crtc_id;

    auto result = drmModeSetCrtc(drm_fd, crtc_id,
                                 0, 0, 0, nullptr, 0, nullptr);
    if (result)
    {
        BOOST_THROW_EXCEPTION((
            std::system_error{
                -result,
                std::system_category(),
                "Couldn't clear output "s + mgk::connector_name(connector)}));
    }
}

void mgek::EGLOutput::restore_saved_crtc()
{
    if (!using_saved_crtc)
    {
        auto result = drmModeSetCrtc(
            drm_fd,
            saved_crtc.crtc_id,
            saved_crtc.buffer_id,
            saved_crtc.x, saved_crtc.y,
            &connector->connector_id,
            1,
            &saved_crtc.mode);

        if (result != 0)
        {
            BOOST_THROW_EXCEPTION((std::system_error{-result, std::system_category(), "Failed to set CRTC"}));
        }

        using_saved_crtc = true;
    }
}

void mgek::EGLOutput::set_power_mode(MirPowerMode mode)
{
    auto ret = drmModeConnectorSetProperty(
        drm_fd,
        connector->connector_id,
        dpms_enum_id,
        mode);
    if (ret)
    {
        BOOST_THROW_EXCEPTION((std::system_error{-ret, std::system_category(), "Failed to set output power mode"}));
    }
}
