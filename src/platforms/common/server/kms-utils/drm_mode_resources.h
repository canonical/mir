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

#ifndef MIR_GRAPHICS_COMMON_KMS_UTILS_DRM_MODE_RESOURCES_H_
#define MIR_GRAPHICS_COMMON_KMS_UTILS_DRM_MODE_RESOURCES_H_

#include <xf86drm.h>
#include <xf86drmMode.h>

#include <memory>
#include <functional>

namespace mir
{
namespace graphics
{
namespace kms
{

typedef std::unique_ptr<drmModeCrtc,std::function<void(drmModeCrtc*)>> DRMModeCrtcUPtr;
typedef std::unique_ptr<drmModeEncoder,std::function<void(drmModeEncoder*)>> DRMModeEncoderUPtr;
typedef std::unique_ptr<drmModeConnector,std::function<void(drmModeConnector*)>> DRMModeConnectorUPtr;
typedef std::unique_ptr<drmModeRes,std::function<void(drmModeRes*)>> DRMModeResUPtr;

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

    class Connectors
    {
    public:
        class iterator :
            public std::iterator<std::input_iterator_tag, DRMModeConnectorUPtr>
        {
        public:
            iterator(iterator const& from);
            iterator& operator=(iterator const& rhs);

            iterator& operator++();
            iterator operator++(int);

            bool operator==(iterator const& rhs) const;
            bool operator!=(iterator const& rhs) const;

            DRMModeConnectorUPtr& operator*() const;
            DRMModeConnectorUPtr* operator->() const;

        private:
            friend class Connectors;
            iterator(int drm_fd, uint32_t* id_ptr);

            int drm_fd;
            uint32_t* id_ptr;
            DRMModeConnectorUPtr mutable current;
        };

        iterator begin();
        iterator end();
    private:
        friend class DRMModeResources;
        explicit Connectors(int drm_fd, uint32_t* begin, uint32_t* end);

        int const drm_fd;
        uint32_t* const begin_;
        uint32_t* const end_;
    };
    Connectors connectors() const;

    class Encoders
    {
    public:
        class iterator :
            public std::iterator<std::input_iterator_tag, DRMModeEncoderUPtr>
        {
        public:
            iterator(iterator const& from);
            iterator& operator=(iterator const& rhs);

            iterator& operator++();
            iterator operator++(int);

            bool operator==(iterator const& rhs) const;
            bool operator!=(iterator const& rhs) const;

            DRMModeEncoderUPtr& operator*() const;
            DRMModeEncoderUPtr* operator->() const;

        private:
            friend class Encoders;
            iterator(int drm_fd, uint32_t* id_ptr);

            int drm_fd;
            uint32_t* id_ptr;
            DRMModeEncoderUPtr mutable current;
        };

        iterator begin();
        iterator end();
    private:
        friend class DRMModeResources;
        explicit Encoders(int drm_fd, uint32_t* begin, uint32_t* end);
        int const drm_fd;
        uint32_t* const begin_;
        uint32_t* const end_;
    };
    Encoders encoders() const;

    class CRTCs
    {
    public:
        class iterator :
            public std::iterator<std::input_iterator_tag, DRMModeCrtcUPtr>
        {
        public:
            iterator(iterator const& from);
            iterator& operator=(iterator const& rhs);

            iterator& operator++();
            iterator operator++(int);

            bool operator==(iterator const& rhs) const;
            bool operator!=(iterator const& rhs) const;

            DRMModeCrtcUPtr& operator*() const;
            DRMModeCrtcUPtr* operator->() const;

        private:
            friend class CRTCs;
            iterator(int drm_fd, uint32_t* id_ptr);

            int drm_fd;
            uint32_t* id_ptr;
            DRMModeCrtcUPtr mutable current;
        };

        iterator begin();
        iterator end();
    private:
        friend class DRMModeResources;
        explicit CRTCs(int drm_fd, uint32_t* begin, uint32_t* end);
        int const drm_fd;
        uint32_t* const begin_;
        uint32_t* const end_;
    };
    CRTCs crtcs() const;

private:
    int const drm_fd;
    DRMModeResUPtr const resources;
};


DRMModeConnectorUPtr get_connector(int drm_fd, uint32_t id);
DRMModeEncoderUPtr get_encoder(int drm_fd, uint32_t id);
DRMModeCrtcUPtr get_crtc(int drm_fd, uint32_t id);


}
}
}

#endif /* MIR_GRAPHICS_COMMON_KMS_UTILS_DRM_MODE_RESOURCES_H_ */
