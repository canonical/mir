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

#include "mir/test/doubles/fd_matcher.h"
#include "src/server/frontend/protobuf_buffer_packer.h"
#include "src/server/frontend/resource_cache.h"

#include "mir_protobuf.pb.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <stdio.h>

namespace mf=mir::frontend;
namespace mfd=mir::frontend::detail;
namespace mp=mir::protobuf;
namespace geom=mir::geometry;
namespace mtd=mir::test::doubles;

TEST(ProtobufBufferPacker, packing)
{
    geom::Stride dummy_stride(4);

    mp::Buffer response;
    mfd::ProtobufBufferPacker packer(&response);

    int num_fd = 33, num_int = 44;
    std::vector<mir::Fd> raw_fds;
    for(auto i=0; i < num_fd; i++)
    {
        raw_fds.push_back(mir::Fd(fileno(tmpfile())));
        packer.pack_fd(mir::Fd(mir::IntOwnedFd{raw_fds.back()}));
    }
    for(auto i=0; i < num_int; i++)
        packer.pack_data(i);

    packer.pack_stride(dummy_stride);
    packer.pack_flags(123);
    packer.pack_size(geom::Size{456, 789});

    EXPECT_EQ(num_fd, response.fd_size());
    EXPECT_EQ(num_int, response.data_size());
    auto i = 0u;
    for (auto raw_fd : raw_fds)
        EXPECT_EQ(raw_fd, response.fd(i++));
    for (int i = 0; i < response.data_size(); ++i)
        EXPECT_EQ(i, response.data(i));
    EXPECT_EQ(dummy_stride.as_uint32_t(), static_cast<unsigned int>(response.stride()));
    EXPECT_EQ(123U, response.flags());
    EXPECT_EQ(456, response.width());
    EXPECT_EQ(789, response.height());
}

TEST(ProtobufBufferPacker, data_and_fds_are_the_same_as_packed)
{
    using namespace testing;

    mp::Buffer response;
    unsigned int const num_fds{3};
    unsigned int const num_data{9};
    for(auto i = 0u; i < num_fds; i++)
        response.add_fd(mir::Fd{fileno(tmpfile())});
    for(auto i = 0u; i < num_data; i++)
        response.add_data(i*3);

    mfd::ProtobufBufferPacker packer(&response);

    mir::Fd additional_fd{fileno(tmpfile())};
    packer.pack_fd(additional_fd);

    auto fds = packer.fds();
    EXPECT_THAT(fds.size(), Eq(num_fds + 1));

    auto data = packer.data();
    EXPECT_THAT(data, ElementsAreArray(response.data().data(), num_data));
}

TEST(ProtobufBufferPacker, message_takes_ownership_of_fds)
{
    using namespace testing;

    mp::Buffer response;
    unsigned int const num_fds{3};
    for(auto i = 0u; i < num_fds; i++)
        response.add_fd(fileno(tmpfile()));

    mir::Fd additional_fd{fileno(tmpfile())};

    {
        mfd::ProtobufBufferPacker packer(&response);
        packer.pack_fd(additional_fd);
    }

    EXPECT_THAT(response.fd().size(), Eq(num_fds+1));
    auto i = 0u;
    for(; i < num_fds; i++)
        EXPECT_THAT(response.fd().Get(i), Not(mtd::RawFdIsValid()));
    EXPECT_THAT(response.fd().Get(i), mtd::RawFdIsValid());
}
