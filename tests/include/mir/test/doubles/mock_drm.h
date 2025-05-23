/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_TEST_DOUBLES_MOCK_DRM_H_
#define MIR_TEST_DOUBLES_MOCK_DRM_H_

#include "mir_test_framework/mmap_wrapper.h"
#include "mir_test_framework/open_wrapper.h"
#include "mir/geometry/forward.h"

#include <gmock/gmock.h>

#include <xf86drm.h>
#include <xf86drmMode.h>
#include <unordered_map>
#include <unordered_set>

namespace mir
{
namespace test
{
namespace doubles
{

class FakeDRMResources
{
public:
    FakeDRMResources();
    ~FakeDRMResources();

    int fd() const;
    int write_fd() const;
    drmModeRes* resources_ptr();
    drmModePlaneRes* plane_resources_ptr();
    drmModeObjectProperties* get_object_properties(uint32_t id, uint32_t type);
    drmModePropertyRes* get_property(uint32_t prop_id);
    drmModePropertyBlobRes* get_property_blob(uint32_t prop_id);

    uint32_t add_property(const char *name);
    void add_crtc(uint32_t id, drmModeModeInfo mode,
                  std::vector<uint32_t> prop_ids = {},
                  std::vector<uint64_t> prop_values = {});
    void add_encoder(uint32_t encoder_id, uint32_t crtc_id, uint32_t possible_crtcs_mask,
                     std::vector<uint32_t> prop_ids = {},
                     std::vector<uint64_t> prop_values = {});
    void add_connector(uint32_t connector_id, uint32_t type, drmModeConnection connection,
                       uint32_t encoder_id, std::vector<drmModeModeInfo>& modes,
                       std::vector<uint32_t>& possible_encoder_ids,
                       geometry::Size const& physical_size,
                       drmModeSubPixel subpixel_arrangement = DRM_MODE_SUBPIXEL_UNKNOWN,
                       std::vector<uint32_t> prop_ids = {},
                       std::vector<uint64_t> prop_values = {});
    void add_plane(std::vector<uint32_t>& formats, uint32_t plane_id, uint32_t crtc_id,
                   uint32_t fb_id, uint32_t crtc_x, uint32_t crtc_y, uint32_t x,
                   uint32_t y, uint32_t possible_crtcs_mask, uint32_t gamma_size,
                   std::vector<uint32_t> prop_ids = {}, std::vector<uint64_t> prop_values = {});

    void prepare();
    void reset();

    drmModeCrtc* find_crtc(uint32_t id);
    drmModeEncoder* find_encoder(uint32_t id);
    drmModeConnector* find_connector(uint32_t id);
    drmModePlane* find_plane(uint32_t id);

    enum ModePreference {NormalMode, PreferredMode};
    static drmModeModeInfo create_mode(uint16_t hdisplay, uint16_t vdisplay,
                                       uint32_t clock, uint16_t htotal, uint16_t vtotal,
                                       ModePreference preferred);

    bool drm_setversion_called{false};
private:
    struct DRMObjectWithProperties
    {
        uint32_t object_type;
        std::vector<uint32_t> ids;
        std::vector<uint64_t> values;
    };

    struct DRMProperty
    {
        uint32_t id;
        char name[DRM_PROP_NAME_LEN];
    };

    int pipe_fds[2];

    drmModeRes resources;
    drmModePlaneRes plane_resources;
    drmModePlane plane;
    std::vector<drmModeCrtc> crtcs;
    std::vector<drmModeEncoder> encoders;
    std::vector<drmModeConnector> connectors;
    std::vector<drmModePlane> planes;
    std::unordered_map<uint32_t, DRMObjectWithProperties> objects;
    std::unordered_map<uint32_t, std::string> properties;
    std::unordered_set<drmModeObjectPropertiesPtr> allocated_obj_props;
    std::unordered_set<drmModePropertyPtr> allocated_props;
    std::unordered_set<drmModePropertyBlobPtr> allocated_prop_blobs;
    uint32_t next_prop_id{1};

    std::vector<uint32_t> crtc_ids;
    std::vector<uint32_t> encoder_ids;
    std::vector<uint32_t> connector_ids;
    std::vector<uint32_t> plane_ids;

    std::vector<drmModeModeInfo> modes;
    std::vector<drmModeModeInfo> modes_empty;
    std::vector<uint32_t> connector_encoder_ids;
    std::vector<uint32_t> plane_formats;
};

class MockDRM
{
public:
    MockDRM();
    ~MockDRM() noexcept;

    MOCK_METHOD(int, open, (char const* path, int flags));
    MOCK_METHOD(int, drmOpen, (const char *name, const char *busid));
    MOCK_METHOD(int, drmIoctl, (int fd, unsigned long request, void *arg));

    MOCK_METHOD(drmModeResPtr, drmModeGetResources, (int fd));
    MOCK_METHOD(drmModeConnectorPtr, drmModeGetConnector, (int fd, uint32_t connectorId));
    MOCK_METHOD(drmModeEncoderPtr, drmModeGetEncoder, (int fd, uint32_t encoder_id));
    MOCK_METHOD(drmModePlaneResPtr, drmModeGetPlaneResources, (int fd));
    MOCK_METHOD(drmModePlanePtr, drmModeGetPlane, (int fd, uint32_t plane_id));
    MOCK_METHOD(drmModeObjectPropertiesPtr, drmModeObjectGetProperties, (int fd, uint32_t id, uint32_t type));
    MOCK_METHOD(drmModeCrtcPtr, drmModeGetCrtc, (int fd, uint32_t crtcId));
    MOCK_METHOD(int, drmModeSetCrtc, (int fd, uint32_t crtcId, uint32_t bufferId,
                                     uint32_t x, uint32_t y, uint32_t *connectors,
                                     int count, drmModeModeInfoPtr mode));

    MOCK_METHOD(void, drmModeFreeResources, (drmModeResPtr ptr));
    MOCK_METHOD(void, drmModeFreeConnector, (drmModeConnectorPtr ptr));
    MOCK_METHOD(void, drmModeFreeEncoder, (drmModeEncoderPtr ptr));
    MOCK_METHOD(void, drmModeFreeCrtc, (drmModeCrtcPtr ptr));
    MOCK_METHOD(void, drmModeFreePlaneResources, (drmModePlaneResPtr ptr));
    MOCK_METHOD(void, drmModeFreePlane, (drmModePlanePtr ptr));
    MOCK_METHOD(void, drmModeFreeObjectProperties, (drmModeObjectPropertiesPtr));

    MOCK_METHOD(int, drmModeAddFB, (int fd, uint32_t width, uint32_t height,
                                   uint8_t depth, uint8_t bpp, uint32_t pitch,
                                   uint32_t bo_handle, uint32_t *buf_id));
    MOCK_METHOD(int, drmModeAddFB2, (int fd, uint32_t width, uint32_t height,
                                    uint32_t pixel_format, uint32_t const bo_handles[4],
                                    uint32_t const pitches[4], uint32_t const offsets[4],
                                    uint32_t *buf_id, uint32_t flags));
    MOCK_METHOD(int, drmModeAddFB2WithModifiers, (int fd, uint32_t width, uint32_t height,
        uint32_t fourcc, uint32_t const handles[4], uint32_t const pitches[4],
        uint32_t const offsets[4], uint64_t const modifiers[4], uint32_t *buf_id, uint32_t flags));
    MOCK_METHOD(int, drmModeRmFB, (int fd, uint32_t bufferId));

    MOCK_METHOD(int, drmModePageFlip,
                (int fd, uint32_t crtc_id, uint32_t fb_id, uint32_t flags, void *user_data));
    MOCK_METHOD(int, drmHandleEvent, (int fd, drmEventContextPtr evctx));

    MOCK_METHOD(int, drmGetCap, (int fd, uint64_t capability, uint64_t *value));
    MOCK_METHOD(int, drmSetClientCap, (int fd, uint64_t capability, uint64_t value));
    MOCK_METHOD(drmModePropertyPtr, drmModeGetProperty, (int fd, uint32_t propertyId));
    MOCK_METHOD(drmModePropertyBlobPtr, drmModeGetPropertyBlob, (int fd, uint32_t blobId));
    MOCK_METHOD(void, drmModeFreeProperty, (drmModePropertyPtr));
    MOCK_METHOD(void, drmModeFreePropertyBlob, (drmModePropertyBlobPtr));
    MOCK_METHOD(int, drmModeConnectorSetProperty, (int fd, uint32_t connector_id, uint32_t property_id, uint64_t value));

    MOCK_METHOD(int, drmGetMagic, (int fd, drm_magic_t *magic));
    MOCK_METHOD(int, drmAuthMagic, (int fd, drm_magic_t magic));

    MOCK_METHOD(int, drmPrimeHandleToFD, (int fd, uint32_t handle, uint32_t flags, int *prime_fd));
    MOCK_METHOD(int, drmPrimeFDToHandle, (int fd, int prime_fd, uint32_t *handle));

    MOCK_METHOD(int, drmIsMaster, (int fd));

    MOCK_METHOD(int, drmSetMaster, (int fd));
    MOCK_METHOD(int, drmDropMaster, (int fd));

    MOCK_METHOD(int, drmModeSetCursor, (int fd, uint32_t crtcId, uint32_t bo_handle, uint32_t width, uint32_t height));
    MOCK_METHOD(int, drmModeMoveCursor, (int fd, uint32_t crtcId, int x, int y));

    MOCK_METHOD(int, drmSetInterfaceVersion, (int fd, drmSetVersion* sv));
    MOCK_METHOD(char*, drmGetBusid, (int fd));
    MOCK_METHOD(void, drmFreeBusid, (const char*));
    MOCK_METHOD(char*, drmGetDeviceNameFromFd, (int fd));
    MOCK_METHOD(char*, drmGetPrimaryDeviceNameFromFd, (int fd));

    MOCK_METHOD(int, drmModeCrtcGetGamma, (int fd, uint32_t crtc_id, uint32_t size,
                                          uint16_t* red, uint16_t* green, uint16_t* blue));
    MOCK_METHOD(int, drmModeCrtcSetGamma, (int fd, uint32_t crtc_id, uint32_t size,
                                          uint16_t const* red, uint16_t const* green, uint16_t const* blue));

    MOCK_METHOD(drmVersionPtr, drmGetVersion, (int));
    MOCK_METHOD(void, drmFreeVersion, (drmVersionPtr));

    MOCK_METHOD(int, drmCheckModesettingSupported, (char const*));

    MOCK_METHOD(void*, mmap, (void* addr, size_t length, int prot, int flags, int fd, off_t offset));
    MOCK_METHOD(int, munmap, (void* addr, size_t length));

    uint32_t add_property(char const* device, char const* name);
    void add_crtc(
        char const* device,
        uint32_t id,
        drmModeModeInfo mode,
        std::vector<uint32_t> prop_ids = {},
	std::vector<uint64_t> prop_values = {});
    void add_encoder(
        char const* device,
        uint32_t encoder_id,
        uint32_t crtc_id,
        uint32_t possible_crtcs_mask,
        std::vector<uint32_t> prop_ids = {},
	std::vector<uint64_t> prop_values = {});
    void add_connector(
        char const* device,
        uint32_t connector_id,
        uint32_t type,
        drmModeConnection connection,
        uint32_t encoder_id,
        std::vector<drmModeModeInfo>& modes,
        std::vector<uint32_t>& possible_encoder_ids,
        geometry::Size const& physical_size,
        drmModeSubPixel subpixel_arrangement = DRM_MODE_SUBPIXEL_UNKNOWN,
        std::vector<uint32_t> prop_ids = {},
	std::vector<uint64_t> prop_values = {});
    void add_plane(
        char const* device,
	std::vector<uint32_t>& formats,
	uint32_t plane_id,
	uint32_t crtc_id,
        uint32_t fb_id,
	uint32_t crtc_x,
	uint32_t crtc_y,
	uint32_t x,
        uint32_t y,
	uint32_t possible_crtcs_mask,
	uint32_t gamma_size,
        std::vector<uint32_t> prop_ids = {},
	std::vector<uint64_t> prop_values = {});

    void prepare(char const* device);
    void reset(char const* device);

    void generate_event_on(char const* device);
    void consume_event_on(char const* device);

    class IsFdOfDeviceMatcher;
    friend class IsFdOfDeviceMatcher;

private:
    std::unordered_map<std::string, FakeDRMResources> fake_drms;
    std::unordered_map<int, FakeDRMResources&> fd_to_drm;

    class TransparentUPtrComparator
    {
    public:
        auto operator()(std::unique_ptr<char[]> const& a, void* b) const -> bool
        {
            return a.get() < b;
        }
        auto operator()(void*a, std::unique_ptr<char[]> const& b) const -> bool
        {
            return a < b.get();
        }
        auto operator()(std::unique_ptr<char[]> const& a, std::unique_ptr<char[]> const& b) const -> bool
        {
            return a.get() < b.get();
        }
        struct is_transparent;
    };

    std::map<std::unique_ptr<char[]>, size_t, TransparentUPtrComparator> mmapings;
    mir_test_framework::OpenHandlerHandle const open_interposer;
    mir_test_framework::MmapHandlerHandle const mmap_interposer;
    mir_test_framework::MunmapHandlerHandle const munmap_interposer;
};

testing::Matcher<int> IsFdOfDevice(char const* device);
}
}
}

#endif /* MIR_TEST_DOUBLES_DRM_MOCK_H_ */
