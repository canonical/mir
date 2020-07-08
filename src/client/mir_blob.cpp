/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir_blob.h"
#include "display_configuration.h"

#include "mir_toolkit/mir_blob.h"
#include "mir_protobuf.pb.h"

#include "mir/uncaught.h"

namespace mp = mir::protobuf;

namespace
{
struct MirManagedBlob : MirBlob
{

    size_t size() const override { return size_; }
    void const* data() const override { return  data_; }
    google::protobuf::uint8* data() { return data_; }

    size_t const size_;
    google::protobuf::uint8*  const data_;

    MirManagedBlob(size_t const size) : size_{size}, data_{new google::protobuf::uint8[size]} {}
    ~MirManagedBlob() { delete[] data_; }
};

struct MirUnmanagedBlob : MirBlob
{
    size_t size() const override { return size_; }
    void const* data() const override { return  data_; }

    size_t const size_;
    void const* const data_;

    MirUnmanagedBlob(size_t const size, void const* data) : size_{size}, data_{data} {}
};
}  // namespace

MirBlob* mir_blob_from_display_config(MirDisplayConfig* configuration)
try
{
    auto blob = std::make_unique<MirManagedBlob>(static_cast<size_t>(configuration->ByteSize()));

    configuration->SerializeWithCachedSizesToArray(blob->data());

    return blob.release();
}
catch (std::exception const& x)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(x);
    return nullptr;
}

MirBlob* mir_blob_onto_buffer(void const* buffer, size_t buffer_size)
try
{
    return new MirUnmanagedBlob{buffer_size, buffer};
}
catch (std::exception const& x)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(x);
    return nullptr;
}

MirDisplayConfig* mir_blob_to_display_config(MirBlob* blob)
try
{
    auto config = new MirDisplayConfig;

    config->ParseFromArray(mir_blob_data(blob), mir_blob_size(blob));

    return config;
}
catch (std::exception const& x)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(x);
    abort();
}

size_t mir_blob_size(MirBlob* blob)
{
    return blob->size();
}

void const* mir_blob_data(MirBlob* blob)
{
    return blob->data();
}

void mir_blob_release(MirBlob* blob)
{
    delete blob;
}
