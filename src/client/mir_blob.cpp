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
}


MirBlob* mir_blob_from_display_configuration(MirDisplayConfiguration* configuration)
try
{
    mp::DisplayConfiguration protobuf_config;

    for (auto const* card = configuration->cards; card != configuration->cards+configuration->num_cards; ++card)
    {
        auto protobuf_card = protobuf_config.add_display_card();
        protobuf_card->set_card_id(card->card_id);
        protobuf_card->set_max_simultaneous_outputs(card->max_simultaneous_outputs);
    }

    for (auto const* output = configuration->outputs;
         output != configuration->outputs+configuration->num_outputs;
         ++output)
    {
        auto protobuf_output = protobuf_config.add_display_output();

        protobuf_output->set_output_id(output->output_id);
        protobuf_output->set_card_id(output->card_id);
        protobuf_output->set_type(output->type);

        for (auto const* format = output->output_formats;
             format != output->output_formats+output->num_output_formats;
             ++format)
        {
            protobuf_output->add_pixel_format(*format);
        }

        for (auto const* mode = output->modes;
             mode != output->modes+output->num_modes;
             ++mode)
        {
            auto protobuf_output_mode = protobuf_output->add_mode();
            protobuf_output_mode->set_horizontal_resolution(mode->horizontal_resolution);
            protobuf_output_mode->set_vertical_resolution(mode->vertical_resolution);
            protobuf_output_mode->set_refresh_rate(mode->refresh_rate);
        }

        protobuf_output->set_preferred_mode(output->preferred_mode);

        protobuf_output->set_physical_width_mm(output->physical_width_mm);
        protobuf_output->set_physical_height_mm(output->physical_height_mm);

        protobuf_output->set_connected(output->connected);
        protobuf_output->set_used(output->used);
        protobuf_output->set_position_x(output->position_x);
        protobuf_output->set_position_y(output->position_y);
        protobuf_output->set_current_mode(output->current_mode);
        protobuf_output->set_current_format(output->current_format);
        protobuf_output->set_power_mode(output->power_mode);
        protobuf_output->set_orientation(output->orientation);
    }

    auto blob = std::make_unique<MirManagedBlob>(static_cast<size_t>(protobuf_config.ByteSize()));

    protobuf_config.SerializeWithCachedSizesToArray(blob->data());

    return blob.release();
}
catch (std::exception const& x)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(x);
    return nullptr;
}

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

MirDisplayConfiguration* mir_blob_to_display_configuration(MirBlob* blob)
try
{
    mp::DisplayConfiguration protobuf_config;

    protobuf_config.ParseFromArray(mir_blob_data(blob), mir_blob_size(blob));

    auto new_config = new MirDisplayConfiguration;

    new_config->num_cards = protobuf_config.display_card_size();
    new_config->cards = new MirDisplayCard[new_config->num_cards];

    for (auto i = 0u; i != new_config->num_cards; ++i)
    {
        auto const& protobuf_card = protobuf_config.display_card(i);
        auto& card = new_config->cards[i];
        card.card_id = protobuf_card.card_id();
        card.max_simultaneous_outputs = protobuf_card.max_simultaneous_outputs();
    }

    new_config->num_outputs = protobuf_config.display_output_size();
    new_config->outputs = new MirDisplayOutput[new_config->num_outputs];

    for (auto i = 0u; i != new_config->num_outputs; ++i)
    {
        auto const& protobuf_output = protobuf_config.display_output(i);
        auto& output = new_config->outputs[i];

        output.output_id = protobuf_output.output_id();
        output.card_id = protobuf_output.card_id();
        output.type = static_cast<MirDisplayOutputType>(protobuf_output.type());

        output.num_output_formats = protobuf_output.pixel_format_size();
        output.output_formats = new MirPixelFormat[output.num_output_formats];

        output.num_modes = protobuf_output.mode_size();
        output.modes = new MirDisplayMode[output.num_modes];

        for (auto i = 0u; i != output.num_output_formats; ++i)
        {
            output.output_formats[i] = static_cast<MirPixelFormat>(protobuf_output.pixel_format(i));
        }

        for (auto i = 0u; i != output.num_modes; ++i)
        {
            auto const& protobuf_mode = protobuf_output.mode(i);
            auto& mode = output.modes[i];

            mode.horizontal_resolution = protobuf_mode.horizontal_resolution();
            mode.vertical_resolution = protobuf_mode.vertical_resolution();
            mode.refresh_rate = protobuf_mode.refresh_rate();
        }

        output.preferred_mode = protobuf_output.preferred_mode();

        output.physical_width_mm = protobuf_output.physical_width_mm();
        output.physical_height_mm = protobuf_output.physical_height_mm();

        output.connected = protobuf_output.connected();
        output.used = protobuf_output.used();
        output.position_x = protobuf_output.position_x();
        output.position_y = protobuf_output.position_y();
        output.current_mode = protobuf_output.current_mode();
        output.current_format = static_cast<MirPixelFormat>(protobuf_output.current_format());
        output.orientation = static_cast<MirOrientation>(protobuf_output.orientation());
        output.power_mode = static_cast<MirPowerMode>(protobuf_output.power_mode());
    }

    return new_config;
}
catch (std::exception const& x)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(x);
    abort();
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
