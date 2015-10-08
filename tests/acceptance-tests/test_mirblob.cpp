/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir_toolkit/mir_blob.h"
#include "mir_toolkit/mir_client_library.h"

#include "mir_test_framework/connected_client_with_a_surface.h"
#include "mir/test/display_config_matchers.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstring>

using MirBlobAPI = mir_test_framework::ConnectedClientWithASurface;
using mir::test::DisplayConfigMatches;

TEST_F(MirBlobAPI, can_serialize_display_configuration)
{
    std::vector<uint8_t> buffer;

    auto const save_display_config = mir_connection_create_display_config(connection);

    {
        auto const save_blob = mir_blob_from_display_configuration(save_display_config);

        buffer.resize(mir_blob_size(save_blob));
        memcpy(buffer.data(), mir_blob_data(save_blob), buffer.size());
        mir_blob_release(save_blob);
    }

    MirDisplayConfiguration* restore_display_config;
    {
        auto const restore_blob = mir_blob_onto_buffer(buffer.data(), buffer.size());
        restore_display_config = mir_blob_to_display_configuration(restore_blob);
        mir_blob_release(restore_blob);
    }

    EXPECT_THAT(save_display_config, DisplayConfigMatches(restore_display_config));

    mir_display_config_destroy(restore_display_config);
    mir_display_config_destroy(save_display_config);
}