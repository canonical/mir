/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
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

#ifndef MIR_GRAPHICS_COMMON_KMS_UTILS_DRM_MODE_RESOURCES_H_
#define MIR_GRAPHICS_COMMON_KMS_UTILS_DRM_MODE_RESOURCES_H_

#include <xf86drm.h>
#include <xf86drmMode.h>

#include <memory>
#include <functional>
#include <string>
#include <unordered_map>

namespace mir
{
namespace graphics
{
namespace kms
{

typedef std::unique_ptr<drmModeCrtc,std::function<void(drmModeCrtc*)>> DRMModeCrtcUPtr;
typedef std::unique_ptr<drmModeEncoder,std::function<void(drmModeEncoder*)>> DRMModeEncoderUPtr;
typedef std::unique_ptr<drmModeConnector,std::function<void(drmModeConnector*)>> DRMModeConnectorUPtr;
typedef std::unique_ptr<drmModeRes,void(*)(drmModeRes*)> DRMModeResUPtr;
typedef std::unique_ptr<drmModePlaneRes,void(*)(drmModePlaneRes*)> DRMModePlaneResUPtr;
typedef std::unique_ptr<drmModePlane,std::function<void(drmModePlane*)>> DRMModePlaneUPtr;
typedef std::unique_ptr<drmModeObjectProperties,void(*)(drmModeObjectProperties*)> DRMModeObjectPropsUPtr;
typedef std::unique_ptr<drmModePropertyRes,void(*)(drmModePropertyPtr)> DRMModePropertyUPtr;

DRMModeConnectorUPtr get_connector(int drm_fd, uint32_t id);
DRMModeEncoderUPtr get_encoder(int drm_fd, uint32_t id);
DRMModeCrtcUPtr get_crtc(int drm_fd, uint32_t id);
DRMModePlaneUPtr get_plane(int drm_fd, uint32_t id);

class DRMModeResources;
class PlaneResources;

namespace detail
{
template<typename DRMUPtr, DRMUPtr(*)(int, uint32_t)>
class ObjectCollection
{
public:
    class iterator :
        public std::iterator<std::input_iterator_tag, DRMUPtr>
    {
    public:
        iterator(iterator const& from);
        iterator& operator=(iterator const& rhs);

        iterator& operator++();
        iterator operator++(int);

        bool operator==(iterator const& rhs) const;
        bool operator!=(iterator const& rhs) const;

        DRMUPtr& operator*() const;
        DRMUPtr* operator->() const;

    private:
        friend class ObjectCollection;
        iterator(int drm_fd, uint32_t* id_ptr);

        int drm_fd;
        uint32_t* id_ptr;
        DRMUPtr mutable current;
    };

    iterator begin();
    iterator end();
private:
    friend class mir::graphics::kms::DRMModeResources;
    friend class mir::graphics::kms::PlaneResources;
    ObjectCollection(int drm_fd, uint32_t* begin, uint32_t* end);

    int const drm_fd;
    uint32_t* const begin_;
    uint32_t* const end_;
};
}

class ObjectProperties
{
public:
    struct Prop
    {
        uint32_t id;
        uint64_t value;
    };

    ObjectProperties(int drm_fd, uint32_t object_id, uint32_t object_type);
    ObjectProperties(int drm_fd, DRMModePlaneUPtr const& plane);
    ObjectProperties(int drm_fd, DRMModeCrtcUPtr const& crtc);
    ObjectProperties(int drm_fd, DRMModeConnectorUPtr const& connector);

    uint64_t operator[](char const* name) const;
    uint32_t id_for(char const* property_name) const;
    bool has_property(char const* property_name) const;

    std::unordered_map<std::string, Prop>::const_iterator begin() const;
    std::unordered_map<std::string, Prop>::const_iterator end() const;

private:
    std::unordered_map<std::string, Prop> const properties_table;
};

class PlaneResources
{
public:
    explicit PlaneResources(int drm_fd);

    detail::ObjectCollection<DRMModePlaneUPtr, &get_plane> planes() const;
private:
    int const drm_fd;
    DRMModePlaneResUPtr const resources;
};

class DRMModeResources
{
public:
    explicit DRMModeResources(int drm_fd);

    void for_each_connector(std::function<void(DRMModeConnectorUPtr)> const& f) const;

    void for_each_encoder(std::function<void(DRMModeEncoderUPtr)> const& f) const;

    void for_each_crtc(std::function<void(DRMModeCrtcUPtr)> const& f) const;

    size_t num_connectors() const;

    size_t num_encoders() const;

    size_t num_crtcs() const;

    DRMModeConnectorUPtr connector(uint32_t id) const;
    DRMModeEncoderUPtr encoder(uint32_t id) const;
    DRMModeCrtcUPtr crtc(uint32_t id) const;

    detail::ObjectCollection<DRMModeConnectorUPtr, &get_connector> connectors() const;
    detail::ObjectCollection<DRMModeEncoderUPtr, &get_encoder> encoders() const;
    detail::ObjectCollection<DRMModeCrtcUPtr, &get_crtc> crtcs() const;

private:
    int const drm_fd;
    DRMModeResUPtr const resources;
};

}
}
}

#endif /* MIR_GRAPHICS_COMMON_KMS_UTILS_DRM_MODE_RESOURCES_H_ */
