/*
 * Copyright © 2016 Canonical Ltd.
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
 *
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "kms-utils/drm_mode_resources.h"

#include "mir/geometry/size.h"
#include "mir/test/doubles/mock_drm.h"

#include <unordered_set>
#include <array>
#include <vector>
#include <tuple>

#include <boost/throw_exception.hpp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mtd = mir::test::doubles;
namespace mgk = mir::graphics::kms;

namespace
{
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

class FakeDRMObjectsWithProperties
{
public:
    FakeDRMObjectsWithProperties(
        std::vector<DRMProperty> const& properties)
    {
        for (auto const& prop : properties)
        {
            bool inserted;
            std::tie(std::ignore, inserted) = this->properties.insert({prop.id, prop.name});
            if (!inserted)
            {
                BOOST_THROW_EXCEPTION(std::logic_error{"Found a property with duplicate id"});
            }
        }
    }

    uint32_t add_object(DRMObjectWithProperties const& object)
    {
        do
        {
            next_id++;
        }
        while (properties.count(next_id) > 0);

        for (auto id : object.ids)
        {
            if (properties.count(id) == 0)
            {
                BOOST_THROW_EXCEPTION(std::logic_error{"Object has property ID not found in property set"});
            }
        }

        objects[next_id] = object;
        return next_id;
    }

    void setup_mock_drm(mtd::MockDRM& mock)
    {
        using namespace testing;
        ON_CALL(mock, drmModeObjectGetProperties(_, _, _))
            .WillByDefault(Invoke(this, &FakeDRMObjectsWithProperties::get_object_properties));
        ON_CALL(mock, drmModeFreeObjectProperties(_))
            .WillByDefault(Invoke(this, &FakeDRMObjectsWithProperties::free_object_properties));
        ON_CALL(mock, drmModeGetProperty(_, _))
            .WillByDefault(Invoke(this, &FakeDRMObjectsWithProperties::get_property));
        ON_CALL(mock, drmModeFreeProperty(_))
            .WillByDefault(Invoke(this, &FakeDRMObjectsWithProperties::free_property));
    }

private:
    drmModeObjectPropertiesPtr get_object_properties(
        int /*fd*/,
        uint32_t id,
        uint32_t type)
    {
        auto props =
            std::unique_ptr<drmModeObjectProperties, void (*)(drmModeObjectProperties*)>(new drmModeObjectProperties, &::drmModeFreeObjectProperties);

        auto const& object = objects.at(id);
        if (object.object_type != type)
        {
            BOOST_THROW_EXCEPTION(std::runtime_error{"Attempt to look up DRM object with incorrect type"});
        }

        props->count_props = object.ids.size();
        props->props = const_cast<uint32_t*>(object.ids.data());
        props->prop_values = const_cast<uint64_t*>(object.values.data());

        allocated_obj_props.insert(props.get());
        return props.release();
    }

    void free_object_properties(drmModeObjectPropertiesPtr props)
    {
        if (allocated_obj_props.count(props) == 0)
        {
            BOOST_THROW_EXCEPTION(std::runtime_error{"Freeing invalid drmModeObjectPropertiesPtr"});
        }
        delete props;
    }

    drmModePropertyPtr get_property(
        int /*fd*/,
        uint32_t id)
    {
        auto const& property = properties.at(id);

        auto prop =
            std::unique_ptr<drmModePropertyRes, void (*)(drmModePropertyPtr)>(new drmModePropertyRes, &::drmModeFreeProperty);

        memset(prop.get(), 0, sizeof(*prop));

        prop->prop_id = id;
        strncpy(prop->name, property.c_str(), sizeof(prop->name) - 1);

        allocated_props.insert(prop.get());
        return prop.release();
    }

    void free_property(
        drmModePropertyPtr prop)
    {
        if (allocated_props.count(prop) == 0)
        {
            BOOST_THROW_EXCEPTION(std::runtime_error{"Freeing invalid drmModePropertyPtr"});
        }
        delete prop;
    }

    std::unordered_map<uint32_t, DRMObjectWithProperties> objects;
    std::unordered_map<uint32_t, std::string> properties;
    std::unordered_set<drmModeObjectPropertiesPtr> allocated_obj_props;
    std::unordered_set<drmModePropertyPtr> allocated_props;
    uint32_t next_id{1};
};
}

TEST(DRMModeResources, can_access_plane_properties)
{
    using namespace testing;
    std::vector<DRMProperty> properties{
        DRMProperty{1, "FOO"},
        DRMProperty{2, "type"},
        DRMProperty{99, "CRTC_ID"}
    };

    std::vector<DRMObjectWithProperties> plane_object{
        {
            DRM_MODE_OBJECT_PLANE,
            {
                properties[0].id,
                properties[1].id,
                properties[2].id
            },
            {
                0xF00,
                DRM_PLANE_TYPE_CURSOR,
                29
            }
        }
    };

    NiceMock<mtd::MockDRM> mock_drm;

    FakeDRMObjectsWithProperties fake_objects{properties};
    fake_objects.setup_mock_drm(mock_drm);

    auto const plane_id = fake_objects.add_object(plane_object[0]);

    mgk::ObjectProperties plane_props{0, plane_id, DRM_MODE_OBJECT_PLANE};

    EXPECT_THAT(plane_props["type"],
                Eq(static_cast<unsigned>(DRM_PLANE_TYPE_CURSOR)));
    EXPECT_THAT(plane_props.id_for("CRTC_ID"), Eq(99u));
}
