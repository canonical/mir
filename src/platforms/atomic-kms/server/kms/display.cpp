/*
 * Copyright © Canonical Ltd.
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
 */

#include "display.h"
#include "cursor.h"
#include "display_sink.h"

#include "kms_display_configuration.h"
#include "kms_output.h"

#include "mir/console_services.h"
#include "mir/graphics/display_configuration_policy.h"
#include "mir/graphics/display_report.h"
#include "mir/graphics/event_handler_register.h"
#include "mir/graphics/overlapping_output_grouping.h"
#include "mir/graphics/platform.h"

#include <boost/throw_exception.hpp>
#include <boost/exception/get_error_info.hpp>

#include <boost/exception/errinfo_errno.hpp>
#include <gbm.h>
#include <memory>
#include <system_error>
#include <xf86drm.h>
#include "mir/log.h"
#include "kms-utils/drm_mode_resources.h"
#include "kms-utils/kms_connector.h"

#include <drm_fourcc.h>
#include <drm.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <stdexcept>

namespace mga = mir::graphics::atomic;
namespace mg = mir::graphics;

namespace
{
double calculate_vrefresh_hz(drmModeModeInfo const& mode)
{
    if (mode.htotal == 0 || mode.vtotal == 0)
        return 0.0;

    /* mode.clock is in KHz */
    double hz = (mode.clock * 100000LL /
                 ((long)mode.htotal * (long)mode.vtotal)
                ) / 100.0;

    return hz;
}

char const* describe_connection_status(drmModeConnector const& connection)
{
    switch (connection.connection)
    {
    case DRM_MODE_CONNECTED:
        return "connected";
    case DRM_MODE_DISCONNECTED:
        return "disconnected";
    case DRM_MODE_UNKNOWNCONNECTION:
        return "UNKNOWN";
    default:
        return "<Unexpected connection value>";
    }
}

void log_drm_details(mir::Fd const& drm_fd)
{
    mir::log_info("DRM device details:");
    auto version = std::unique_ptr<drmVersion, decltype(&drmFreeVersion)>{
        drmGetVersion(drm_fd),
        &drmFreeVersion};

    auto device_name = std::unique_ptr<char, decltype(&free)>{
        drmGetDeviceNameFromFd(drm_fd),
        &free
    };

    mir::log_info(
        "%s: using driver %s [%s] (version: %i.%i.%i driver date: %s)",
        device_name.get(),
        version->name,
        version->desc,
        version->version_major,
        version->version_minor,
        version->version_patchlevel,
        version->date);

    try
    {
        mg::kms::DRMModeResources resources{drm_fd};
        for (auto const& connector : resources.connectors())
        {
            mir::log_info(
                "\tOutput: %s (%s)",
                mg::kms::connector_name(connector).c_str(),
                describe_connection_status(*connector));
            for (auto i = 0; i < connector->count_modes; ++i)
            {
                mir::log_info(
                    "\t\tMode: %i×%i@%.2f",
                    connector->modes[i].hdisplay,
                    connector->modes[i].vdisplay,
                    calculate_vrefresh_hz(connector->modes[i]));
            }
        }
    }
    catch (std::exception const& error)
    {
        mir::log_info(
            "\tKMS not supported (%s)",
            error.what());
    }
}

}

mga::Display::Display(
    mir::Fd drm_fd,
    std::shared_ptr<struct gbm_device> gbm,
    mga::BypassOption bypass_option,
    std::shared_ptr<DisplayConfigurationPolicy> const& initial_conf_policy,
    std::shared_ptr<DisplayReport> const& listener,
    std::shared_ptr<GbmQuirks> const& gbm_quirks)
    : drm_fd{std::move(drm_fd)},
      gbm{std::move(gbm)},
      listener(listener),
      monitor(mir::udev::Context()),
      output_container{
          std::make_shared<RealKMSOutputContainer>(
            this->drm_fd)},
      current_display_configuration{output_container},
      dirty_configuration{false},
      bypass_option(bypass_option),
      gbm_quirks{gbm_quirks}
{
    monitor.filter_by_subsystem_and_type("drm", "drm_minor");
    monitor.enable();

    log_drm_details(this->drm_fd);

    initial_conf_policy->apply_to(current_display_configuration);

    configure(current_display_configuration);
}

// please don't remove this empty destructor, it's here for the
// unique ptr!! if you accidentally remove it you will get a not
// so relevant linker error about some missing headers
mga::Display::~Display() = default;

void mga::Display::for_each_display_sync_group(
    std::function<void(graphics::DisplaySyncGroup&)> const& f)
{
    std::lock_guard lg{configuration_mutex};

    for (auto& db_ptr : display_sinks)
        f(*db_ptr);
}

std::unique_ptr<mg::DisplayConfiguration> mga::Display::configuration() const
{
    std::lock_guard lg{configuration_mutex};

    if (dirty_configuration)
    {
        /* Give back a copy of the latest configuration information */
        current_display_configuration.update();
        dirty_configuration = false;
    }

    return std::make_unique<mga::RealKMSDisplayConfiguration>(current_display_configuration);
}

void mga::Display::configure(mg::DisplayConfiguration const& conf)
{
    if (!conf.valid())
    {
        BOOST_THROW_EXCEPTION(
            std::logic_error("Invalid or inconsistent display configuration"));
    }

    {
        std::lock_guard lock{configuration_mutex};
        configure_locked(dynamic_cast<RealKMSDisplayConfiguration const&>(conf), lock);
    }
}

void mga::Display::register_configuration_change_handler(
    EventHandlerRegister& handlers,
    DisplayConfigurationChangeHandler const& conf_change_handler)
{
    handlers.register_fd_handler(
        {monitor.fd()},
        this,
        make_module_ptr<std::function<void(int)>>(
            [conf_change_handler, this](int)
            {
                mir::log_debug("Handling UDEV events");
                monitor.process_events([conf_change_handler, this]
                                       (mir::udev::Monitor::EventType type, mir::udev::Device const& device)
                                       {
                                            mir::log_debug("Processing UDEV event for %s: %i", device.syspath(), type);
                                            dirty_configuration = true;
                                            conf_change_handler();
                                       });
            }));
}

void mga::Display::pause()
{
}

void mga::Display::resume()
{
    {
        std::lock_guard lg{configuration_mutex};

        /*
         * After resuming (e.g. because we switched back to the display server VT)
         * we need to reset the CRTCs. For active displays we schedule a CRTC reset
         * on the next swap. For connected but unused outputs we clear the CRTC.
         */
        for (auto& db_ptr : display_sinks)
            db_ptr->schedule_set_crtc();

        clear_connected_unused_outputs();
    }
}

auto mga::Display::create_hardware_cursor() -> std::shared_ptr<graphics::Cursor>
{
    std::shared_ptr<mga::Cursor> locked_cursor = cursor.lock();
    if (!locked_cursor)
    {
        class AtomicKmsCurrentConfiguration : public CurrentConfiguration
        {
        public:
            AtomicKmsCurrentConfiguration(Display& display) :
                display(display)
            {
            }

            void with_current_configuration_do(std::function<void(KMSDisplayConfiguration const&)> const& exec)
            {
                std::lock_guard lg{display.configuration_mutex};
                exec(display.current_display_configuration);
            }

        private:
            Display& display;
        };

        try
        {
            locked_cursor = std::make_shared<mga::Cursor>(
                *output_container, std::make_shared<AtomicKmsCurrentConfiguration>(*this));
        }
        catch (std::runtime_error const&)
        {
            // That's OK, we don't need a hardware cursor. Returning null
            // is allowed and will trigger a fallback to software.
        }
        cursor = locked_cursor;
    }

    return locked_cursor;
}

void mga::Display::clear_connected_unused_outputs()
{
    current_display_configuration.for_each_output([&](DisplayConfigurationOutput const& conf_output)
    {
        /*
         * An output may be unused either because it's explicitly not used
         * (DisplayConfigurationOutput::used) or because its power mode is
         * not mir_power_mode_on.
         */
        if (conf_output.connected &&
            (!conf_output.used || (conf_output.power_mode != mir_power_mode_on)))
        {
            auto kms_output = current_display_configuration.get_output_for(conf_output.id);

            kms_output->clear_crtc();
            kms_output->set_power_mode(conf_output.power_mode);
            kms_output->set_gamma(conf_output.gamma);
        }
    });
}

bool mga::Display::apply_if_configuration_preserves_display_buffers(
    mg::DisplayConfiguration const& conf)
{
    bool result = false;
    auto const& new_kms_conf = dynamic_cast<RealKMSDisplayConfiguration const&>(conf);

    {
        std::lock_guard lock{configuration_mutex};
        if (compatible(current_display_configuration, new_kms_conf))
        {
            configure_locked(new_kms_conf, lock);
            result = true;
        }
    }

    return result;
}

void mga::Display::configure_locked(
    mga::RealKMSDisplayConfiguration const& kms_conf,
    std::lock_guard<std::mutex> const&)
{
    // Treat the current_display_configuration as incompatible with itself,
    // before it's fully constructed, to force proper initialization.
    bool const comp{
        (&kms_conf != &current_display_configuration) &&
        compatible(kms_conf, current_display_configuration)};
    std::vector<std::unique_ptr<DisplaySink>> display_buffers_new;

    if (!comp)
    {
        /* Reset the state of all outputs */
        kms_conf.for_each_output(
            [&](DisplayConfigurationOutput const& conf_output)
            {
                auto kms_output = current_display_configuration.get_output_for(conf_output.id);
                kms_output->clear_cursor();
                kms_output->reset();
            });
    }

    size_t output_idx{0};
    kms_conf.for_each_output(
        [&](DisplayConfigurationOutput const& out)
        {
            if (!out.connected || !out.used ||
                (out.power_mode != mir_power_mode_on) ||
                (out.current_mode_index >= out.modes.size()))
            {
                // We don't need to do anything for unconfigured outputs
                return;
            }

            auto kms_output = kms_conf.get_output_for(out.id);
            auto const mode_index = kms_conf.get_kms_mode_index(out.id, out.current_mode_index);

            kms_output->configure({0, 0}, mode_index);

            if (!comp)
            {
                kms_output->set_power_mode(out.power_mode);
                kms_output->set_gamma(out.gamma);
            }

            auto const transform = out.transformation();

            if (comp)
            {
                /* If we don't need a modeset we can just
                 * update our existing `DisplaySink`s.
                 *
                 * Note: this assumes that RealKMSDisplayConfiguration.for_each_output
                 * iterates over outputs in the same order each time.
                 */
                display_sinks[output_idx]->set_transformation(
                    transform,
                    out.extents());
                ++output_idx;
            }
            else
            {
                /* If we need a modeset we'll create a new set
                 * of `DisplaySink`s
                 */
                auto db = std::make_unique<DisplaySink>(
                    drm_fd,
                    gbm,
                    bypass_option,
                    listener,
                    std::move(kms_output),
                    out.extents(),
                    transform, 
                    gbm_quirks);

                display_buffers_new.push_back(std::move(db));
            }
        });

    if (!comp)
        display_sinks = std::move(display_buffers_new);

    /* Store applied configuration */
    current_display_configuration = kms_conf;

    if (!comp)
        /* Clear connected but unused outputs */
        clear_connected_unused_outputs();
}

namespace
{
auto gbm_create_device_checked(mir::Fd fd) -> std::shared_ptr<struct gbm_device>
{
    errno = 0;
    auto device = gbm_create_device(fd);
    if (!device)
    {
        BOOST_THROW_EXCEPTION((
            std::system_error{
                errno,
                std::system_category(),
                "Failed to create GBM device"}));
    }
    return {
        device,
        [fd](struct gbm_device* device)    // Capture shared ownership of fd to keep gdm_device functional
        {
            if (device)
            {
                gbm_device_destroy(device);
            }
        }
    };
}
}

mga::GBMDisplayProvider::GBMDisplayProvider(
    mir::Fd drm_fd)
    : fd{std::move(drm_fd)},
      gbm{gbm_create_device_checked(fd)}
{
}

auto mga::GBMDisplayProvider::on_this_sink(mg::DisplaySink& sink) const -> bool
{
    if (auto gbm_display_sink = dynamic_cast<mga::DisplaySink*>(&sink))
    {
        return gbm_display_sink->gbm_device() == gbm;
    }
    return false;
}

auto mga::GBMDisplayProvider::gbm_device() const -> std::shared_ptr<struct gbm_device>
{
    return gbm;
}

auto mga::GBMDisplayProvider::is_same_device(mir::udev::Device const& render_device) const -> bool
{
#ifndef MIR_DRM_HAS_GET_DEVICE_FROM_DEVID
    class CStrFree
    {
    public:
        void operator()(char* str)
        {
            if (str)
            {
                free(str);
            }
        }
    };

    std::unique_ptr<char[], CStrFree> primary_node{drmGetPrimaryDeviceNameFromFd(fd)};
    std::unique_ptr<char[], CStrFree> render_node{drmGetRenderDeviceNameFromFd(fd)};

    if (primary_node)
    {
        if (strcmp(primary_node.get(), render_device.devnode()) == 0)
        {
            return true;
        }
    }
    if (render_node)
    {
        if (strcmp(render_node.get(), render_device.devnode()) == 0)
        {
            return true;
        }
    }

    return false;
#else
    drmDevicePtr us{nullptr}, them{nullptr};

    drmGetDeviceFromDevId(render_device.devno(), 0, &them);
    drmGetDevice2(fd, 0, &us);

    bool result = drmDevicesEqual(us, them);

    drmDeviceFree(us);
    drmDeviceFree(them);

    return result;
#endif
}
