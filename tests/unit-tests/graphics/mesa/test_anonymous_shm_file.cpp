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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "src/platform/graphics/mesa/anonymous_shm_file.h"
#include <gtest/gtest.h>

namespace mgm = mir::graphics::mesa;

// TODO: To be removed when kernel support for O_TMPFILE (>= 3.11) is more prevalent
#define CHECK_FLAG_AND_SKIP()                                  \
    {                                                          \
        auto value = getenv("MIR_SKIP_TESTS_USING_O_TMPFILE"); \
        if (value != NULL && !strncmp(value, "true", 4))       \
        {                                                      \
           return;                                             \
        }                                                      \
    }

TEST(AnonymousShmFile, is_created)
{
    CHECK_FLAG_AND_SKIP();

    size_t const file_size{100};

    mgm::AnonymousShmFile shm_file{file_size};

    EXPECT_GE(shm_file.fd(), 0);
}

TEST(AnonymousShmFile, has_correct_size)
{
    CHECK_FLAG_AND_SKIP();

    size_t const file_size{100};

    mgm::AnonymousShmFile shm_file{file_size};

    struct stat stat;
    fstat(shm_file.fd(), &stat);

    EXPECT_EQ(static_cast<off_t>(file_size), stat.st_size);
}

TEST(AnonymousShmFile, writing_to_base_ptr_writes_to_file)
{
    CHECK_FLAG_AND_SKIP();

    size_t const file_size{100};

    mgm::AnonymousShmFile shm_file{file_size};

    auto base_ptr = reinterpret_cast<uint8_t*>(shm_file.base_ptr());

    for (size_t i = 0; i < file_size; i++)
    {
        base_ptr[i] = i;
    }

    std::vector<unsigned char> buffer(file_size);

    EXPECT_EQ(static_cast<ssize_t>(file_size),
              read(shm_file.fd(), buffer.data(), file_size));

    for (size_t i = 0; i < file_size; i++)
    {
        EXPECT_EQ(base_ptr[i], buffer[i]) << "i=" << i;
    }
}
