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

#include "drm_mode_resources.h"

#include <boost/throw_exception.hpp>
#include <system_error>

namespace mgk = mir::graphics::kms;
namespace mgkd = mgk::detail;

namespace
{
mgk::DRMModeResUPtr resources_for_drm_node(int drm_fd)
{
    errno = 0;
    mgk::DRMModeResUPtr resources{drmModeGetResources(drm_fd), &drmModeFreeResources};

    if (!resources)
    {
        if (errno == 0)
        {
            // drmModeGetResources either sets errno, or has failed in malloc()
            errno = ENOMEM;
        }
        BOOST_THROW_EXCEPTION((std::system_error{errno, std::system_category(), "Couldn't get DRM resources"}));
    }

    return resources;
}

mgk::DRMModePlaneResUPtr planes_for_drm_node(int drm_fd)
{
    errno = 0;
    mgk::DRMModePlaneResUPtr resources{drmModeGetPlaneResources(drm_fd), &drmModeFreePlaneResources};

    if (!resources)
    {
        if (errno == 0)
        {
            // drmModeGetPlaneResources either sets errno, or has failed in malloc()
            errno = ENOMEM;
        }
        BOOST_THROW_EXCEPTION((std::system_error{errno, std::system_category(), "Couldn't get DRM plane resources"}));
    }

    return resources;
}

}

mgk::PlaneResources::PlaneResources(int drm_fd)
    : drm_fd{drm_fd},
      resources{planes_for_drm_node(drm_fd)}
{
}

auto mgk::PlaneResources::planes() const -> detail::ObjectCollection<DRMModePlaneUPtr, &get_plane>
{
    return detail::ObjectCollection<DRMModePlaneUPtr, &get_plane>{
        drm_fd,
        resources->planes,
        resources->planes + resources->count_planes};
}

mgk::DRMModeResources::DRMModeResources(int drm_fd)
    : drm_fd{drm_fd},
      resources{resources_for_drm_node(drm_fd)}
{
}

void mgk::DRMModeResources::for_each_connector(std::function<void(DRMModeConnectorUPtr)> const& f) const
{
    for (int i = 0; i < resources->count_connectors; i++)
    {
        f(get_connector(drm_fd, resources->connectors[i]));
    }
}

void mgk::DRMModeResources::for_each_encoder(std::function<void(DRMModeEncoderUPtr)> const& f) const
{
    for (int i = 0; i < resources->count_encoders; i++)
    {
        f(get_encoder(drm_fd, resources->encoders[i]));
    }
}

void mgk::DRMModeResources::for_each_crtc(std::function<void(DRMModeCrtcUPtr)> const& f) const
{
    for (int i = 0; i < resources->count_crtcs; i++)
    {
        f(get_crtc(drm_fd, resources->crtcs[i]));
    }
}

size_t mgk::DRMModeResources::num_connectors() const
{
    return resources->count_connectors;
}

size_t mgk::DRMModeResources::num_encoders() const
{
    return resources->count_encoders;
}

size_t mgk::DRMModeResources::num_crtcs() const
{
    return resources->count_crtcs;
}

mgk::DRMModeConnectorUPtr mgk::DRMModeResources::connector(uint32_t id) const
{
    return get_connector(drm_fd, id);
}

mgk::DRMModeEncoderUPtr mgk::DRMModeResources::encoder(uint32_t id) const
{
    return get_encoder(drm_fd, id);
}

mgk::DRMModeCrtcUPtr mgk::DRMModeResources::crtc(uint32_t id) const
{
    return get_crtc(drm_fd, id);
}

mgk::DRMModeConnectorUPtr mgk::get_connector(int drm_fd, uint32_t id)
{
    errno = 0;
    DRMModeConnectorUPtr connector{drmModeGetConnector(drm_fd, id), &drmModeFreeConnector};

    if (!connector)
    {
        if (errno == 0)
        {
            // drmModeGetConnector either sets errno, or has failed in malloc()
            errno = ENOMEM;
        }
        BOOST_THROW_EXCEPTION((
            std::system_error{errno, std::system_category(), "Failed to get DRM connector"}));
    }
    return connector;
}

mgk::DRMModeEncoderUPtr mgk::get_encoder(int drm_fd, uint32_t id)
{
    errno = 0;
    DRMModeEncoderUPtr encoder{drmModeGetEncoder(drm_fd, id), &drmModeFreeEncoder};

    if (!encoder)
    {
        if (errno == 0)
        {
            // drmModeGetEncoder either sets errno, or has failed in malloc()
            errno = ENOMEM;
        }
        BOOST_THROW_EXCEPTION((
            std::system_error{errno, std::system_category(), "Failed to get DRM encoder"}));
    }
    return encoder;
}

mgk::DRMModeCrtcUPtr mgk::get_crtc(int drm_fd, uint32_t id)
{
    errno = 0;
    DRMModeCrtcUPtr crtc{drmModeGetCrtc(drm_fd, id), &drmModeFreeCrtc};

    if (!crtc)
    {
        if (errno == 0)
        {
            // drmModeGetCrtc either sets errno, or has failed in malloc()
            errno = ENOMEM;
        }
        BOOST_THROW_EXCEPTION((
            std::system_error{errno, std::system_category(), "Failed to get DRM crtc"}));
    }
    return crtc;
}

mgk::DRMModePlaneUPtr mgk::get_plane(int drm_fd, uint32_t id)
{
    errno = 0;
    DRMModePlaneUPtr plane{drmModeGetPlane(drm_fd, id), &drmModeFreePlane};

    if (!plane)
    {
        if (errno == 0)
        {
            // drmModeGetPlane either sets errno, or has failed in malloc()
            errno = ENOMEM;
        }
        BOOST_THROW_EXCEPTION((std::system_error{errno, std::system_category(), "Failed to get DRM plane"}));
    }
    return plane;
}

namespace
{
mgk::DRMModePropertyUPtr get_property(int drm_fd, uint32_t id)
{
    errno = 0;
    mgk::DRMModePropertyUPtr prop{drmModeGetProperty(drm_fd, id), &drmModeFreeProperty};

    if (!prop)
    {
        if (errno == 0)
        {
            // drmModeGetProperty either sets errno, or has failed in malloc()
            errno = ENOMEM;
        }
        BOOST_THROW_EXCEPTION((std::system_error{errno, std::system_category(), "Failed to get DRM plane"}));
    }
    return prop;
}

mgk::DRMModeObjectPropsUPtr get_object_properties(
    int drm_fd,
    uint32_t object_id,
    uint32_t object_type)
{
    errno = 0;
    mgk::DRMModeObjectPropsUPtr props{
        drmModeObjectGetProperties(drm_fd, object_id, object_type), &drmModeFreeObjectProperties};

    if (!props)
    {
        if (errno == 0)
        {
            // drmModeGetObjectProperties either sets errno, or has failed in malloc()
            errno = ENOMEM;
        }
        BOOST_THROW_EXCEPTION((std::system_error{errno, std::system_category(), "Failed to get DRM object properties"}));
    }
    return props;
}

std::unordered_map<std::string, mgk::ObjectProperties::Prop> extract_properties(
    int drm_fd,
    mgk::DRMModeObjectPropsUPtr const& properties)
{
    std::unordered_map<std::string, mgk::ObjectProperties::Prop> property_map;
    for (auto i = 0u; i < properties->count_props; ++i)
    {
        auto prop = get_property(drm_fd, properties->props[i]);
        property_map[prop->name] = {
            prop->prop_id,
            properties->prop_values[i]
        };
    }
    return property_map;
}
}

mgk::ObjectProperties::ObjectProperties(
    int drm_fd,
    uint32_t object_id,
    uint32_t object_type)
    : properties_table{extract_properties(drm_fd, get_object_properties(drm_fd, object_id, object_type))}
{
}

mgk::ObjectProperties::ObjectProperties(
    int drm_fd,
    DRMModePlaneUPtr const& plane)
    : ObjectProperties(drm_fd, plane->plane_id, DRM_MODE_OBJECT_PLANE)
{
}

uint64_t mgk::ObjectProperties::operator[](char const* name) const
{
    return properties_table.at(name).value;
}

uint32_t mgk::ObjectProperties::id_for(char const* property_name) const
{
    return properties_table.at(property_name).id;
}

bool mgk::ObjectProperties::has_property(char const* property_name) const
{
    return properties_table.count(property_name) > 0;
}

auto mgk::ObjectProperties::begin() const -> std::unordered_map<std::string, Prop>::const_iterator
{
    return properties_table.begin();
}

auto mgk::ObjectProperties::end() const -> std::unordered_map<std::string, Prop>::const_iterator
{
    return properties_table.end();
}

auto mgk::DRMModeResources::connectors() const -> detail::ObjectCollection<DRMModeConnectorUPtr, &get_connector>
{
    return detail::ObjectCollection<DRMModeConnectorUPtr, &get_connector>{drm_fd, resources->connectors, resources->connectors + resources->count_connectors};
}

auto mgk::DRMModeResources::encoders() const -> detail::ObjectCollection<DRMModeEncoderUPtr, &get_encoder>
{
    return detail::ObjectCollection<DRMModeEncoderUPtr, &get_encoder>{drm_fd, resources->encoders, resources->encoders + resources->count_encoders};
}

auto mgk::DRMModeResources::crtcs() const -> detail::ObjectCollection<DRMModeCrtcUPtr, &get_crtc>
{
    return detail::ObjectCollection<DRMModeCrtcUPtr, &get_crtc>{drm_fd, resources->crtcs, resources->crtcs + resources->count_crtcs};
}

template<typename DRMUPtr, DRMUPtr(*object_constructor)(int drm_fd, uint32_t id)>
mgkd::ObjectCollection<DRMUPtr, object_constructor>::ObjectCollection(int drm_fd, uint32_t* begin, uint32_t* end)
    : drm_fd{drm_fd},
      begin_{begin},
      end_{end}
{
}

template<typename DRMUPtr, DRMUPtr(*object_constructor)(int drm_fd, uint32_t id)>
auto mgkd::ObjectCollection<DRMUPtr, object_constructor>::begin() -> iterator
{
    return iterator(drm_fd, begin_);
}

template auto mgkd::ObjectCollection<mgk::DRMModeConnectorUPtr, &mgk::get_connector>::begin() -> iterator;
template auto mgkd::ObjectCollection<mgk::DRMModeEncoderUPtr, &mgk::get_encoder>::begin() -> iterator;
template auto mgkd::ObjectCollection<mgk::DRMModeCrtcUPtr, &mgk::get_crtc>::begin() -> iterator;
template auto mgkd::ObjectCollection<mgk::DRMModePlaneUPtr, &mgk::get_plane>::begin() -> iterator;

template<typename DRMUPtr, DRMUPtr(*object_constructor)(int drm_fd, uint32_t id)>
auto mgkd::ObjectCollection<DRMUPtr, object_constructor>::end() -> iterator
{
    return iterator(drm_fd, end_);
}

template auto mgkd::ObjectCollection<mgk::DRMModeConnectorUPtr, &mgk::get_connector>::end() -> iterator;
template auto mgkd::ObjectCollection<mgk::DRMModeEncoderUPtr, &mgk::get_encoder>::end() -> iterator;
template auto mgkd::ObjectCollection<mgk::DRMModeCrtcUPtr, &mgk::get_crtc>::end() -> iterator;
template auto mgkd::ObjectCollection<mgk::DRMModePlaneUPtr, &mgk::get_plane>::end() -> iterator;

template<typename DRMUPtr, DRMUPtr(*object_constructor)(int drm_fd, uint32_t id)>
mgkd::ObjectCollection<DRMUPtr, object_constructor>::iterator::iterator(iterator const& from)
    : drm_fd{from.drm_fd},
      id_ptr{from.id_ptr}
{
}
template mgkd::ObjectCollection<mgk::DRMModeConnectorUPtr, &mgk::get_connector>::iterator::iterator(iterator const&);
template mgkd::ObjectCollection<mgk::DRMModeEncoderUPtr, &mgk::get_encoder>::iterator::iterator(iterator const&);
template mgkd::ObjectCollection<mgk::DRMModeCrtcUPtr, &mgk::get_crtc>::iterator::iterator(iterator const&);
template mgkd::ObjectCollection<mgk::DRMModePlaneUPtr, &mgk::get_plane>::iterator::iterator(iterator const&);


template<typename DRMUPtr, DRMUPtr(*object_constructor)(int drm_fd, uint32_t id)>
auto mgkd::ObjectCollection<DRMUPtr, object_constructor>::iterator::operator=(iterator const& rhs) -> iterator&
{
    drm_fd = rhs.drm_fd;
    id_ptr = rhs.id_ptr;
    current.reset();
    return *this;
}
template auto mgkd::ObjectCollection<mgk::DRMModeConnectorUPtr, &mgk::get_connector>::iterator::operator=(iterator const&) -> iterator&;
template auto mgkd::ObjectCollection<mgk::DRMModeEncoderUPtr, &mgk::get_encoder>::iterator::operator=(iterator const&) -> iterator&;
template auto mgkd::ObjectCollection<mgk::DRMModeCrtcUPtr, &mgk::get_crtc>::iterator::operator=(iterator const&) -> iterator&;
template auto mgkd::ObjectCollection<mgk::DRMModePlaneUPtr, &mgk::get_plane>::iterator::operator=(iterator const&) -> iterator&;

template<typename DRMUPtr, DRMUPtr(*object_constructor)(int drm_fd, uint32_t id)>
mgkd::ObjectCollection<DRMUPtr, object_constructor>::iterator::iterator(
    int drm_fd,
    uint32_t* id_ptr)
    : drm_fd{drm_fd},
      id_ptr{id_ptr}
{
}
template mgkd::ObjectCollection<mgk::DRMModeConnectorUPtr, &mgk::get_connector>::iterator::iterator(int, uint32_t*);
template mgkd::ObjectCollection<mgk::DRMModeEncoderUPtr, &mgk::get_encoder>::iterator::iterator(int, uint32_t*);
template mgkd::ObjectCollection<mgk::DRMModeCrtcUPtr, &mgk::get_crtc>::iterator::iterator(int, uint32_t*);
template mgkd::ObjectCollection<mgk::DRMModePlaneUPtr, &mgk::get_plane>::iterator::iterator(int, uint32_t*);

template<typename DRMUPtr, DRMUPtr(*object_constructor)(int drm_fd, uint32_t id)>
auto mgkd::ObjectCollection<DRMUPtr, object_constructor>::iterator::operator++() -> iterator&
{
    ++id_ptr;
    current.reset();
    return *this;
}
template auto mgkd::ObjectCollection<mgk::DRMModeConnectorUPtr, &mgk::get_connector>::iterator::operator++() -> iterator&;
template auto mgkd::ObjectCollection<mgk::DRMModeEncoderUPtr, &mgk::get_encoder>::iterator::operator++() -> iterator&;
template auto mgkd::ObjectCollection<mgk::DRMModeCrtcUPtr, &mgk::get_crtc>::iterator::operator++() -> iterator&;
template auto mgkd::ObjectCollection<mgk::DRMModePlaneUPtr, &mgk::get_plane>::iterator::operator++() -> iterator&;

template<typename DRMUPtr, DRMUPtr(*object_constructor)(int drm_fd, uint32_t id)>
auto mgkd::ObjectCollection<DRMUPtr, object_constructor>::iterator::operator++(int) -> iterator
{
    iterator copy(drm_fd, id_ptr);
    ++id_ptr;
    current.reset();
    return copy;
}
template auto mgkd::ObjectCollection<mgk::DRMModeConnectorUPtr, &mgk::get_connector>::iterator::operator++(int) -> iterator;
template auto mgkd::ObjectCollection<mgk::DRMModeEncoderUPtr, &mgk::get_encoder>::iterator::operator++(int) -> iterator;
template auto mgkd::ObjectCollection<mgk::DRMModeCrtcUPtr, &mgk::get_crtc>::iterator::operator++(int) -> iterator;
template auto mgkd::ObjectCollection<mgk::DRMModePlaneUPtr, &mgk::get_plane>::iterator::operator++(int) -> iterator;


template<typename DRMUPtr, DRMUPtr(*object_constructor)(int drm_fd, uint32_t id)>
auto mgkd::ObjectCollection<DRMUPtr, object_constructor>::iterator::operator*() const -> DRMUPtr&
{
    if (!current)
    {
        current = (*object_constructor)(drm_fd, *id_ptr);
    }
    return current;
}
template auto mgkd::ObjectCollection<mgk::DRMModeConnectorUPtr, &mgk::get_connector>::iterator::operator*() const -> DRMModeConnectorUPtr&;
template auto mgkd::ObjectCollection<mgk::DRMModeEncoderUPtr, &mgk::get_encoder>::iterator::operator*() const -> DRMModeEncoderUPtr&;
template auto mgkd::ObjectCollection<mgk::DRMModeCrtcUPtr, &mgk::get_crtc>::iterator::operator*() const -> DRMModeCrtcUPtr&;
template auto mgkd::ObjectCollection<mgk::DRMModePlaneUPtr, &mgk::get_plane>::iterator::operator*() const -> DRMModePlaneUPtr&;

template<typename DRMUPtr, DRMUPtr(*object_constructor)(int drm_fd, uint32_t id)>
auto mgkd::ObjectCollection<DRMUPtr, object_constructor>::iterator::operator->() const -> DRMUPtr*
{
    if (!current)
    {
        current = (*object_constructor)(drm_fd, *id_ptr);
    }
    return &current;
}
template auto mgkd::ObjectCollection<mgk::DRMModeConnectorUPtr, &mgk::get_connector>::iterator::operator->() const -> DRMModeConnectorUPtr*;
template auto mgkd::ObjectCollection<mgk::DRMModeEncoderUPtr, &mgk::get_encoder>::iterator::operator->() const -> DRMModeEncoderUPtr*;
template auto mgkd::ObjectCollection<mgk::DRMModeCrtcUPtr, &mgk::get_crtc>::iterator::operator->() const -> DRMModeCrtcUPtr*;
template auto mgkd::ObjectCollection<mgk::DRMModePlaneUPtr, &mgk::get_plane>::iterator::operator->() const -> DRMModePlaneUPtr*;

template<typename DRMUPtr, DRMUPtr(*object_constructor)(int drm_fd, uint32_t id)>
bool mgkd::ObjectCollection<DRMUPtr, object_constructor>::iterator::operator==(iterator const& rhs) const
{
    return rhs.id_ptr == id_ptr;
}
template bool mgkd::ObjectCollection<mgk::DRMModeConnectorUPtr, &mgk::get_connector>::iterator::operator==(iterator const&) const;
template bool mgkd::ObjectCollection<mgk::DRMModeEncoderUPtr, &mgk::get_encoder>::iterator::operator==(iterator const&) const;
template bool mgkd::ObjectCollection<mgk::DRMModeCrtcUPtr, &mgk::get_crtc>::iterator::operator==(iterator const&) const;
template bool mgkd::ObjectCollection<mgk::DRMModePlaneUPtr, &mgk::get_plane>::iterator::operator==(iterator const&) const;

template<typename DRMUPtr, DRMUPtr(*object_constructor)(int drm_fd, uint32_t id)>
bool mgkd::ObjectCollection<DRMUPtr, object_constructor>::iterator::operator!=(iterator const& rhs) const
{
    return !(*this == rhs);
}
template bool mgkd::ObjectCollection<mgk::DRMModeConnectorUPtr, &mgk::get_connector>::iterator::operator!=(iterator const&) const;
template bool mgkd::ObjectCollection<mgk::DRMModeEncoderUPtr, &mgk::get_encoder>::iterator::operator!=(iterator const&) const;
template bool mgkd::ObjectCollection<mgk::DRMModeCrtcUPtr, &mgk::get_crtc>::iterator::operator!=(iterator const&) const;
template bool mgkd::ObjectCollection<mgk::DRMModePlaneUPtr, &mgk::get_plane>::iterator::operator!=(iterator const&) const;
