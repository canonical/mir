#include <mir_toolkit/mir_display_configuration.h>

void mir_display_config_release(MirDisplayConfig* /*config*/)
{

}
int mir_display_config_get_num_outputs(MirDisplayConfig const* /*config*/)
{
    return 1;
}

MirOutput const* mir_display_config_get_output(MirDisplayConfig const* /*config*/, int /*index*/)
{
    return nullptr;
}

MirOutput* mir_display_config_get_mutable_output(MirDisplayConfig* /*config*/, int /*index*/)
{
    return nullptr;
}

bool mir_output_is_enabled(MirOutput const* /*output*/)
{
    return false;
}

void mir_output_enable(MirOutput* /*output*/)
{

}

void mir_output_disable(MirOutput* /*output*/)
{

}

int mir_display_config_get_max_simultaneous_outputs(MirDisplayConfig const* /*conf*/)
{
    return 1;
}

int mir_output_get_id(MirOutput const* /*output*/)
{
    return 1;
}

MirDisplayOutputType mir_output_get_type(MirOutput const* /*output*/)
{
    return mir_display_output_type_displayport;
}

int mir_output_get_preferred_mode(MirOutput const* /*output*/)
{
    return 0;
}

int mir_output_physical_width_mm(MirOutput const* /*output*/)
{
    return 0;
}

int mir_output_get_num_modes(MirOutput const* /*output*/)
{
    return 0;
}

int mir_output_get_current_mode(MirOutput const* /*output*/)
{
    return 0;
}

void mir_output_set_current_mode(MirOutput* /*output*/, int /*index*/)
{

}

MirDisplayMode* mir_output_get_mode(MirOutput const* /*output*/, int /*index*/)
{
    return nullptr;
}

int mir_output_get_num_output_formats(MirOutput const* /*output*/)
{
    return 0;
}

MirPixelFormat mir_output_get_format(MirOutput const* /*output*/, int /*index*/)
{
    return mir_pixel_format_xbgr_8888;
}

MirPixelFormat mir_output_get_current_format(MirOutput const* /*output*/)
{
    return mir_pixel_format_xbgr_8888;
}

void mir_output_set_format(MirOutput* /*output*/, int /*index*/)
{

}

int mir_output_get_position_x(MirOutput const* /*output*/)
{
    return 0;
}

int mir_output_get_position_y(MirOutput const* /*output*/)
{
    return 0;
}

bool mir_output_is_connected(MirOutput const* /*output*/)
{
    return false;
}

int mir_output_physical_height_mm(MirOutput const* /*output*/)
{
    return 0;
}

MirPowerMode mir_output_get_power_mode(MirOutput const* /*output*/)
{
    return mir_power_mode_standby;
}

void mir_output_set_power_mode(MirOutput* /*output*/, MirPowerMode /*mode*/)
{

}

MirOrientation mir_output_get_orientation(MirOutput const* /*output*/)
{
    return mir_orientation_left;
}

void mir_output_set_orientation(MirOutput* /*output*/, MirOrientation /*orientation*/)
{

}
