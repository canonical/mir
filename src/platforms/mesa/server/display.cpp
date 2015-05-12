/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "display.h"
#include "cursor.h"
#include "platform.h"
#include "display_buffer.h"
#include "kms_display_configuration.h"
#include "kms_output.h"
#include "kms_page_flipper.h"
#include "virtual_terminal.h"
#include "mir/graphics/overlapping_output_grouping.h"
#include "mir/graphics/event_handler_register.h"

#include "mir/graphics/display_report.h"
#include "mir/graphics/gl_context.h"
#include "mir/graphics/display_configuration_policy.h"
#include "mir/geometry/rectangle.h"

#include <boost/throw_exception.hpp>
#include <boost/exception/get_error_info.hpp>
#include <boost/exception/errinfo_errno.hpp>

#include <stdexcept>
#include <algorithm>

namespace mgm = mir::graphics::mesa;
namespace mg = mir::graphics;
namespace geom = mir::geometry;

namespace
{

int errno_from_exception(std::exception const& e)
{
    auto errno_ptr = boost::get_error_info<boost::errinfo_errno>(e);
    return (errno_ptr != nullptr) ? *errno_ptr : -1;
}

class GBMGLContext : public mg::GLContext
{
public:
    GBMGLContext(mgm::helpers::GBMHelper const& gbm,
                 mg::GLConfig const& gl_config,
                 EGLContext shared_context)
        : egl{gl_config}
    {
        egl.setup(gbm, shared_context);
    }

    void make_current() const override
    {
        egl.make_current();
    }

    void release_current() const override
    {
        egl.release_current();
    }

private:
    mgm::helpers::EGLHelper egl;
};

}

mgm::Display::Display(std::shared_ptr<Platform> const& platform,
                      std::shared_ptr<DisplayConfigurationPolicy> const& initial_conf_policy,
                      std::shared_ptr<GLConfig> const& gl_config,
                      std::shared_ptr<DisplayReport> const& listener)
    : platform(platform),
      listener(listener),
      monitor(mir::udev::Context()),
      shared_egl{*gl_config},
      output_container{platform->drm->fd,
                       std::make_shared<KMSPageFlipper>(platform->drm->fd, listener)},
      current_display_configuration{platform->drm->fd},
      gl_config{gl_config}
{
    platform->vt->set_graphics_mode();

    shared_egl.setup(platform->gbm);

    monitor.filter_by_subsystem_and_type("drm", "drm_minor");
    monitor.enable();

    initial_conf_policy->apply_to(current_display_configuration);

    configure(current_display_configuration);

    shared_egl.make_current();
}

// please don't remove this empty destructor, it's here for the
// unique ptr!! if you accidentally remove it you will get a not
// so relevant linker error about some missing headers
mgm::Display::~Display()
{
}

void mgm::Display::for_each_display_sync_group(
    std::function<void(graphics::DisplaySyncGroup&)> const& f)
{
    std::lock_guard<std::mutex> lg{configuration_mutex};

    for (auto& db_ptr : display_buffers)
        f(*db_ptr);
}

std::unique_ptr<mg::DisplayConfiguration> mgm::Display::configuration() const
{
    std::lock_guard<std::mutex> lg{configuration_mutex};

    /* Give back a copy of the latest configuration information */
    current_display_configuration.update();
    return std::unique_ptr<mg::DisplayConfiguration>(
        new mgm::RealKMSDisplayConfiguration(current_display_configuration)
        );
}

void mgm::Display::configure(mg::DisplayConfiguration const& conf)
{
    if (!conf.valid())
    {
        BOOST_THROW_EXCEPTION(
            std::logic_error("Invalid or inconsistent display configuration"));
    }

    {
        std::lock_guard<std::mutex> lg{configuration_mutex};

        auto const& kms_conf = dynamic_cast<RealKMSDisplayConfiguration const&>(conf);
        // Treat the current_display_configuration as incompatible with itself,
        // before it's fully constructed, to force proper initialization.
        bool const comp{(&conf != &current_display_configuration) &&
                        compatible(kms_conf, current_display_configuration)};
        std::vector<std::unique_ptr<DisplayBuffer>> display_buffers_new;

        if (!comp)
        {
            /*
             * Notice for a little while here we will have duplicate
             * DisplayBuffers attached to each output, and the display_buffers_new
             * will take over the outputs before the old display_buffers are
             * destroyed. So to avoid page flipping confusion in-between, make
             * sure we wait for all pending page flips to finish before the
             * display_buffers_new are created and take control of the outputs.
             */
            for (auto& db : display_buffers)
                db->wait_for_page_flip();

            /* Reset the state of all outputs */
            kms_conf.for_each_output([&](DisplayConfigurationOutput const& conf_output)
            {
                uint32_t const connector_id = current_display_configuration.get_kms_connector_id(conf_output.id);
                auto kms_output = output_container.get_kms_output_for(connector_id);
                kms_output->clear_cursor();
                kms_output->reset();
            });
        }

        /* Set up used outputs */
        OverlappingOutputGrouping grouping{conf};
        auto group_idx = 0;

        grouping.for_each_group([&](OverlappingOutputGroup const& group)
        {
            auto bounding_rect = group.bounding_rectangle();
            std::vector<std::shared_ptr<KMSOutput>> kms_outputs;
            MirOrientation orientation = mir_orientation_normal;

            group.for_each_output([&](DisplayConfigurationOutput const& conf_output)
            {
                uint32_t const connector_id = kms_conf.get_kms_connector_id(conf_output.id);
                auto kms_output = output_container.get_kms_output_for(connector_id);

                auto const mode_index = kms_conf.get_kms_mode_index(conf_output.id,
                                                                    conf_output.current_mode_index);
                kms_output->configure(conf_output.top_left - bounding_rect.top_left, mode_index);
                if (!comp)
                {
                    kms_output->set_power_mode(conf_output.power_mode);
                    kms_outputs.push_back(kms_output);
                }

                /*
                 * Presently OverlappingOutputGroup guarantees all grouped
                 * outputs have the same orientation.
                 */
                orientation = conf_output.orientation;
            });

            if (comp)
            {
                display_buffers[group_idx++]->set_orientation(orientation, bounding_rect);
            }
            else
            {
                uint32_t width = bounding_rect.size.width.as_uint32_t();
                uint32_t height = bounding_rect.size.height.as_uint32_t();
                if (orientation == mir_orientation_left ||
                    orientation == mir_orientation_right)
                {
                    std::swap(width, height);
                }

                auto surface = platform->gbm.create_scanout_surface(width, height);

                std::unique_ptr<DisplayBuffer> db{
                    new DisplayBuffer{platform, listener,
                                      kms_outputs,
                                      std::move(surface),
                                      bounding_rect,
                                      orientation,
                                      *gl_config,
                                      shared_egl.context()}};

                display_buffers_new.push_back(std::move(db));
            }
        });

        if (!comp)
            display_buffers = std::move(display_buffers_new);

        /* Store applied configuration */
        current_display_configuration = kms_conf;

        if (!comp)
            /* Clear connected but unused outputs */
            clear_connected_unused_outputs();
    }

    if (auto c = cursor.lock()) c->resume();
}

void mgm::Display::register_configuration_change_handler(
    EventHandlerRegister& handlers,
    DisplayConfigurationChangeHandler const& conf_change_handler)
{
    handlers.register_fd_handler(
        {monitor.fd()},
        this,
        [conf_change_handler, this](int)
        {
            monitor.process_events([conf_change_handler]
                                   (mir::udev::Monitor::EventType, mir::udev::Device const&)
                                   {
                                        conf_change_handler();
                                   });
        });
}

void mgm::Display::register_pause_resume_handlers(
    EventHandlerRegister& handlers,
    DisplayPauseHandler const& pause_handler,
    DisplayResumeHandler const& resume_handler)
{
    platform->vt->register_switch_handlers(handlers, pause_handler, resume_handler);
}

void mgm::Display::pause()
{
    try
    {
        if (auto c = cursor.lock()) c->suspend();
        platform->drm->drop_master();
    }
    catch(std::runtime_error const& e)
    {
        listener->report_drm_master_failure(errno_from_exception(e));
        throw;
    }
}

void mgm::Display::resume()
{
    try
    {
        platform->drm->set_master();
    }
    catch(std::runtime_error const& e)
    {
        listener->report_drm_master_failure(errno_from_exception(e));
        throw;
    }

    {
        std::lock_guard<std::mutex> lg{configuration_mutex};

        /*
         * After resuming (e.g. because we switched back to the display server VT)
         * we need to reset the CRTCs. For active displays we schedule a CRTC reset
         * on the next swap. For connected but unused outputs we clear the CRTC.
         */
        for (auto& db_ptr : display_buffers)
            db_ptr->schedule_set_crtc();

        clear_connected_unused_outputs();
    }

    if (auto c = cursor.lock()) c->resume();
}

auto mgm::Display::create_hardware_cursor(std::shared_ptr<mg::CursorImage> const& initial_image) -> std::shared_ptr<graphics::Cursor>
{
    // There is only one hardware cursor. We do not keep a strong reference to it in the display though,
    // if no other component of Mir is interested (i.e. the input stack does not keep a reference to send
    // position updates) we must be configured not to use a cursor and thusly let it deallocate.
    std::shared_ptr<mgm::Cursor> locked_cursor = cursor.lock();
    if (!locked_cursor)
    {
        class KMSCurrentConfiguration : public CurrentConfiguration
        {
        public:
            KMSCurrentConfiguration(Display& display)
                : display(display)
            {
            }

            void with_current_configuration_do(
                std::function<void(KMSDisplayConfiguration const&)> const& exec)
            {
                std::lock_guard<std::mutex> lg{display.configuration_mutex};
                exec(display.current_display_configuration);
            }

        private:
            Display& display;
        };

        cursor = locked_cursor = std::make_shared<Cursor>(platform->gbm.device, output_container,
            std::make_shared<KMSCurrentConfiguration>(*this),
            initial_image);
    }

    return locked_cursor;
}

std::unique_ptr<mg::GLContext> mgm::Display::create_gl_context()
{
    return std::unique_ptr<GBMGLContext>{
        new GBMGLContext{
            platform->gbm,
            *gl_config,
            shared_egl.context()}};
}

void mgm::Display::clear_connected_unused_outputs()
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
            uint32_t const connector_id = current_display_configuration.get_kms_connector_id(conf_output.id);
            auto kms_output = output_container.get_kms_output_for(connector_id);

            kms_output->clear_crtc();
            kms_output->set_power_mode(conf_output.power_mode);
        }
    });
}
