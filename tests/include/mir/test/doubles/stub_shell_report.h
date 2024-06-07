/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_TEST_DOUBLES_STUB_SHELL_REPORT_H
#define MIR_TEST_DOUBLES_STUB_SHELL_REPORT_H

#include <mir/shell/shell_report.h>

namespace mir
{
namespace test
{
namespace doubles
{
class StubShellReport : public mir::shell::ShellReport
{
public:
    void opened_session(mir::scene::Session const&) override {}
    void closing_session(mir::scene::Session const&) override {}
    void created_surface(mir::scene::Session const&, mir::scene::Surface const&) override {}
    void update_surface(
    mir::scene::Session const&, mir::scene::Surface const&,
    mir::shell::SurfaceSpecification const&) override {}
    void update_surface(
    mir::scene::Session const&, mir::scene::Surface const&,
    MirWindowAttrib, int) override {}
    void destroying_surface(mir::scene::Session const&, mir::scene::Surface const&) override {}
    void started_prompt_session(mir::scene::PromptSession const&, mir::scene::Session const&) override {}
    void added_prompt_provider(mir::scene::PromptSession const&, mir::scene::Session const&) override {}
    void stopping_prompt_session(mir::scene::PromptSession const&) override {}
    void adding_display(mir::geometry::Rectangle const&) override {}
    void removing_display(mir::geometry::Rectangle const&) override {}
    void input_focus_set_to(mir::scene::Session const*, mir::scene::Surface const*) override {}
    void surfaces_raised(mir::shell::SurfaceSet const&) override {}
};
}
}
}

#endif //MIR_TEST_DOUBLES_STUB_SHELL_REPORT_H
