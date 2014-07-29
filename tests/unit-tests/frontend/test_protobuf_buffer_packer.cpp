/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "src/server/frontend/protobuf_buffer_packer.h"
#include "src/server/frontend/resource_cache.h"

#include "mir_protobuf.pb.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mf=mir::frontend;
namespace mfd=mir::frontend::detail;
namespace mp=mir::protobuf;
namespace geom=mir::geometry;

namespace
{

struct MockResourceCache : public mf::MessageResourceCache
{
    MOCK_METHOD2(save_resource, void(google::protobuf::Message*, std::shared_ptr<void> const&));
    MOCK_METHOD1(free_resource, void(google::protobuf::Message*));
};

struct ProtobufBufferPacker : public testing::Test
{
    ProtobufBufferPacker() :
        mock_resource_cache(std::make_shared<testing::NiceMock<MockResourceCache>>())
    {
    }
    std::shared_ptr<MockResourceCache> mock_resource_cache;
};
}

TEST_F(ProtobufBufferPacker, packing)
{
    geom::Stride dummy_stride(4);

    mp::Buffer response;
    mfd::ProtobufBufferPacker packer(&response, mock_resource_cache);

    int num_fd = 33, num_int = 44;
    for(auto i=0; i < num_fd; i++)
        packer.pack_fd(i);
    for(auto i=0; i < num_int; i++)
        packer.pack_data(i);

    packer.pack_stride(dummy_stride);
    packer.pack_flags(123);
    packer.pack_size(geom::Size{456, 789});

    EXPECT_EQ(num_fd, response.fd_size());
    EXPECT_EQ(num_int, response.data_size());
    for (int i = 0; i < response.fd_size(); ++i)
        EXPECT_EQ(i, response.fd(i));
    for (int i = 0; i < response.data_size(); ++i)
        EXPECT_EQ(i, response.data(i));
    EXPECT_EQ(dummy_stride.as_uint32_t(), static_cast<unsigned int>(response.stride()));
    EXPECT_EQ(123U, response.flags());
    EXPECT_EQ(456, response.width());
    EXPECT_EQ(789, response.height());
}

TEST_F(ProtobufBufferPacker, fd_packing_saves_using_the_resource_cache)
{
    geom::Stride dummy_stride(4);

    mp::Buffer response;
    mfd::ProtobufBufferPacker packer(&response, mock_resource_cache);

    int num_fd = 33, num_int = 44;
    for(auto i=0; i < num_fd; i++)
        packer.pack_fd(i);
    for(auto i=0; i < num_int; i++)
        packer.pack_data(i);

    packer.pack_stride(dummy_stride);
    packer.pack_flags(123);
    packer.pack_size(geom::Size{456, 789});

    EXPECT_EQ(num_fd, response.fd_size());
    EXPECT_EQ(num_int, response.data_size());
    for (int i = 0; i < response.fd_size(); ++i)
        EXPECT_EQ(i, response.fd(i));
    for (int i = 0; i < response.data_size(); ++i)
        EXPECT_EQ(i, response.data(i));
    EXPECT_EQ(dummy_stride.as_uint32_t(), static_cast<unsigned int>(response.stride()));
    EXPECT_EQ(123U, response.flags());
    EXPECT_EQ(456, response.width());
    EXPECT_EQ(789, response.height());
}
