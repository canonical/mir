/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "override_watcher.h"

#include "live_config_test_helpers.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstring>
#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace fs = std::filesystem;
namespace mlc = miral::live_config;
using FileChange = mlc::OverrideWatcher::FileChange;
using OverrideDirEvent = mlc::OverrideWatcher::OverrideDirEvent;
using BatchSummary = mlc::OverrideWatcher::BatchSummary;


namespace
{

auto make_tmp(std::string const& label) -> fs::path
{
    auto tmpl = (fs::temp_directory_path() / (label + "-XXXXXX")).string();
    auto const raw = mkdtemp(tmpl.data());
    if (!raw)
        throw std::runtime_error{std::string{"mkdtemp failed: "} + strerror(errno)};
    return fs::path{raw};
}

auto make_watcher_common(fs::path config) -> std::shared_ptr<mlc::OverrideWatcher>
{
    return std::make_shared<mlc::OverrideWatcher>(std::move(config), [](auto const&) {}, ".conf");
}

struct TestOverrideWatcher : testing::Test
{
    XdgEnvGuard xdg_guard;
    fs::path tmp_dir;
    fs::path dummy_system_dir;
    fs::path base_config;
    fs::path override_dir;

    void SetUp() override
    {
        tmp_dir = make_tmp("test-override-watcher-home");
        dummy_system_dir = make_tmp("test-override-watcher-sys");

        base_config = tmp_dir / "test.conf";
        override_dir = tmp_dir / "test.conf.d";
        std::ofstream{base_config} << "base";

        setenv("XDG_CONFIG_HOME", tmp_dir.c_str(), 1);
        setenv("XDG_CONFIG_DIRS", dummy_system_dir.c_str(), 1);
    }

    void TearDown() override
    {
        fs::remove_all(tmp_dir);
        fs::remove_all(dummy_system_dir);
    }

    void write_override_file(std::string const& name, std::string const& content = "content")
    {
        fs::create_directories(override_dir);
        std::ofstream{override_dir / name} << content;
    }

    auto make_watcher() -> std::shared_ptr<mlc::OverrideWatcher>
    {
        return ::make_watcher_common(base_config);
    }
};

struct SummaryBuilder
{
    BatchSummary s;

    auto base_changed(fs::path p) -> SummaryBuilder&
    {
        s.base_config_changed = std::move(p);
        return *this;
    }

    auto base_removed(fs::path p) -> SummaryBuilder&
    {
        s.base_config_removed = std::move(p);
        return *this;
    }

    auto dir_event(fs::path root, OverrideDirEvent e) -> SummaryBuilder&
    {
        s.override_dir_events[std::move(root)] = e;
        return *this;
    }

    auto file_change(fs::path p, FileChange c) -> SummaryBuilder&
    {
        s.override_file_changes[p.parent_path()].push_back({std::move(p), c});
        return *this;
    }

    operator BatchSummary() { return std::move(s); }
};
}


TEST_F(TestOverrideWatcher, empty_batch_summary_produces_no_reload)
{
    auto watcher = make_watcher();
    EXPECT_FALSE(watcher->apply_events({}).has_value());
}

TEST_F(TestOverrideWatcher, base_config_change_marks_base_as_modified_and_known_overrides_as_unchanged)
{
    write_override_file("10-first.conf");
    auto watcher = make_watcher(); // known = {10-first.conf}

    auto result = watcher->apply_events(SummaryBuilder{}.base_changed(base_config));
    ASSERT_TRUE(result.has_value());
    auto const rec = record(*result);

    EXPECT_THAT(rec.modified, testing::ElementsAre(base_config));
    EXPECT_THAT(rec.unchanged, testing::ElementsAre(override_dir / "10-first.conf"));
    EXPECT_THAT(rec.fresh, testing::IsEmpty());
    EXPECT_THAT(rec.dropped, testing::IsEmpty());
}

TEST_F(TestOverrideWatcher, new_override_file_is_classified_as_fresh)
{
    write_override_file("10-first.conf");
    auto watcher = make_watcher(); // known = {10-first.conf}

    write_override_file("20-second.conf");
    auto result = watcher->apply_events(
        SummaryBuilder{}.file_change(override_dir / "20-second.conf", FileChange::written));
    ASSERT_TRUE(result.has_value());
    auto const rec = record(*result);

    EXPECT_THAT(rec.fresh, testing::ElementsAre(override_dir / "20-second.conf"));
    EXPECT_THAT(rec.unchanged, testing::ElementsAre(base_config, override_dir / "10-first.conf"));
    EXPECT_THAT(rec.modified, testing::IsEmpty());
    EXPECT_THAT(rec.dropped, testing::IsEmpty());
}

TEST_F(TestOverrideWatcher, fresh_file_becomes_modified_on_subsequent_write)
{
    write_override_file("10-first.conf");
    auto watcher = make_watcher(); // known = {10-first.conf}

    write_override_file("20-second.conf");
    watcher->apply_events(
        SummaryBuilder{}.file_change(override_dir / "20-second.conf", FileChange::written));

    write_override_file("20-second.conf", "updated content");
    auto result = watcher->apply_events(
        SummaryBuilder{}.file_change(override_dir / "20-second.conf", FileChange::written));
    ASSERT_TRUE(result.has_value());
    auto const rec = record(*result);

    EXPECT_THAT(rec.modified, testing::ElementsAre(override_dir / "20-second.conf"));
    EXPECT_THAT(rec.unchanged, testing::ElementsAre(base_config, override_dir / "10-first.conf"));
    EXPECT_THAT(rec.fresh, testing::IsEmpty());
    EXPECT_THAT(rec.dropped, testing::IsEmpty());
}

TEST_F(TestOverrideWatcher, existing_override_file_change_is_classified_as_modified)
{
    write_override_file("10-first.conf");
    write_override_file("20-second.conf");
    auto watcher = make_watcher(); // known = {10-first.conf, 20-second.conf}

    auto result = watcher->apply_events(
        SummaryBuilder{}.file_change(override_dir / "10-first.conf", FileChange::written));
    ASSERT_TRUE(result.has_value());
    auto const rec = record(*result);

    EXPECT_THAT(rec.modified, testing::ElementsAre(override_dir / "10-first.conf"));
    EXPECT_THAT(rec.unchanged, testing::ElementsAre(base_config, override_dir / "20-second.conf"));
    EXPECT_THAT(rec.fresh, testing::IsEmpty());
    EXPECT_THAT(rec.dropped, testing::IsEmpty());
}

TEST_F(TestOverrideWatcher, deleted_override_file_is_classified_as_dropped)
{
    write_override_file("10-first.conf");
    write_override_file("20-second.conf");
    auto watcher = make_watcher(); // known = {10-first.conf, 20-second.conf}

    fs::remove(override_dir / "10-first.conf");
    auto result = watcher->apply_events(
        SummaryBuilder{}.file_change(override_dir / "10-first.conf", FileChange::removed));
    ASSERT_TRUE(result.has_value());
    auto const rec = record(*result);

    EXPECT_THAT(rec.dropped, testing::ElementsAre(override_dir / "10-first.conf"));
    EXPECT_THAT(rec.unchanged, testing::ElementsAre(base_config, override_dir / "20-second.conf"));
    EXPECT_THAT(rec.fresh, testing::IsEmpty());
    EXPECT_THAT(rec.modified, testing::IsEmpty());
}

TEST_F(TestOverrideWatcher, simultaneously_written_new_files_are_both_classified_as_fresh)
{
    fs::create_directories(override_dir);
    auto watcher = make_watcher(); // known = {} (override dir empty)

    write_override_file("10-first.conf");
    write_override_file("20-second.conf");
    auto result = watcher->apply_events(
        SummaryBuilder{}.file_change(override_dir / "10-first.conf", FileChange::written).file_change(override_dir / "20-second.conf", FileChange::written));
    ASSERT_TRUE(result.has_value());
    auto const rec = record(*result);

    EXPECT_THAT(rec.fresh, testing::ElementsAre(override_dir / "10-first.conf", override_dir / "20-second.conf"));
    EXPECT_THAT(rec.unchanged, testing::ElementsAre(base_config));
    EXPECT_THAT(rec.modified, testing::IsEmpty());
    EXPECT_THAT(rec.dropped, testing::IsEmpty());
}

TEST_F(TestOverrideWatcher, override_dir_created_with_file_triggers_reload)
{
    auto watcher = make_watcher(); // no override dir, known = {}

    write_override_file("10-first.conf");
    auto result = watcher->apply_events(SummaryBuilder{}.dir_event(tmp_dir, OverrideDirEvent::created));
    ASSERT_TRUE(result.has_value());
    auto const rec = record(*result);

    EXPECT_THAT(rec.fresh, testing::ElementsAre(override_dir / "10-first.conf"));
    EXPECT_THAT(rec.unchanged, testing::ElementsAre(base_config));
    EXPECT_THAT(rec.modified, testing::IsEmpty());
    EXPECT_THAT(rec.dropped, testing::IsEmpty());
}

TEST_F(TestOverrideWatcher, empty_override_dir_created_produces_no_reload)
{
    auto watcher = make_watcher(); // no override dir

    fs::create_directories(override_dir);
    EXPECT_FALSE(watcher->apply_events(SummaryBuilder{}.dir_event(tmp_dir, OverrideDirEvent::created)).has_value());
}

TEST_F(TestOverrideWatcher, override_dir_removed_with_known_files_triggers_reload)
{
    write_override_file("10-first.conf");
    auto watcher = make_watcher(); // known = {10-first.conf}

    fs::remove_all(override_dir);
    auto result = watcher->apply_events(SummaryBuilder{}.dir_event(tmp_dir, OverrideDirEvent::removed));
    ASSERT_TRUE(result.has_value());
    auto const rec = record(*result);

    EXPECT_THAT(rec.dropped, testing::ElementsAre(override_dir / "10-first.conf"));
    EXPECT_THAT(rec.unchanged, testing::ElementsAre(base_config));
    EXPECT_THAT(rec.modified, testing::IsEmpty());
    EXPECT_THAT(rec.fresh, testing::IsEmpty());
}

TEST_F(TestOverrideWatcher, override_dir_recreated_with_different_files_drops_old_and_adds_new)
{
    write_override_file("10-first.conf");
    auto watcher = make_watcher(); // known = {10-first.conf}, dir is watched

    fs::remove_all(override_dir);
    write_override_file("20-second.conf"); // dir recreated with a different file
    auto result = watcher->apply_events(SummaryBuilder{}.dir_event(tmp_dir, OverrideDirEvent::created));
    ASSERT_TRUE(result.has_value());
    auto const rec = record(*result);

    EXPECT_THAT(rec.dropped, testing::ElementsAre(override_dir / "10-first.conf"));
    EXPECT_THAT(rec.fresh, testing::ElementsAre(override_dir / "20-second.conf"));
    EXPECT_THAT(rec.unchanged, testing::ElementsAre(base_config));
    EXPECT_THAT(rec.modified, testing::IsEmpty());
}

TEST_F(TestOverrideWatcher, override_dir_recreated_with_same_files_treats_them_as_fresh)
{
    write_override_file("10-first.conf");
    auto watcher = make_watcher();

    fs::remove_all(override_dir);
    write_override_file("10-first.conf", "updated content");
    auto result = watcher->apply_events(SummaryBuilder{}.dir_event(tmp_dir, OverrideDirEvent::created));
    ASSERT_TRUE(result.has_value());
    auto const rec = record(*result);

    EXPECT_THAT(rec.fresh, testing::ElementsAre(override_dir / "10-first.conf"));
    EXPECT_THAT(rec.unchanged, testing::ElementsAre(base_config));
    EXPECT_THAT(rec.dropped, testing::IsEmpty());
    EXPECT_THAT(rec.modified, testing::IsEmpty());
}

TEST_F(TestOverrideWatcher, base_and_override_dir_created_emits_modified_and_fresh)
{
    auto watcher = make_watcher(); // no override dir yet, known = {}

    write_override_file("10-first.conf");
    auto result = watcher->apply_events(SummaryBuilder{}.base_changed(base_config).dir_event(tmp_dir, OverrideDirEvent::created));
    ASSERT_TRUE(result.has_value());
    auto const rec = record(*result);

    EXPECT_THAT(rec.modified, testing::ElementsAre(base_config));
    EXPECT_THAT(rec.fresh, testing::ElementsAre(override_dir / "10-first.conf"));
    EXPECT_THAT(rec.unchanged, testing::IsEmpty());
    EXPECT_THAT(rec.dropped, testing::IsEmpty());
}

TEST_F(TestOverrideWatcher, base_and_override_dir_removed_emits_modified_and_dropped)
{
    write_override_file("10-first.conf");
    auto watcher = make_watcher(); // known = {10-first.conf}

    fs::remove_all(override_dir);
    auto result = watcher->apply_events(SummaryBuilder{}.base_changed(base_config).dir_event(tmp_dir, OverrideDirEvent::removed));
    ASSERT_TRUE(result.has_value());
    auto const rec = record(*result);

    EXPECT_THAT(rec.modified, testing::ElementsAre(base_config));
    EXPECT_THAT(rec.dropped, testing::ElementsAre(override_dir / "10-first.conf"));
    EXPECT_THAT(rec.unchanged, testing::IsEmpty());
    EXPECT_THAT(rec.fresh, testing::IsEmpty());
}

TEST_F(TestOverrideWatcher, base_and_new_override_file_emits_modified_and_fresh)
{
    fs::create_directories(override_dir);
    auto watcher = make_watcher(); // known = {}

    write_override_file("10-first.conf");
    auto result = watcher->apply_events(
        SummaryBuilder{}.base_changed(base_config).file_change(override_dir / "10-first.conf", FileChange::written));
    ASSERT_TRUE(result.has_value());
    auto const rec = record(*result);

    EXPECT_THAT(rec.modified, testing::ElementsAre(base_config));
    EXPECT_THAT(rec.fresh, testing::ElementsAre(override_dir / "10-first.conf"));
    EXPECT_THAT(rec.unchanged, testing::IsEmpty());
    EXPECT_THAT(rec.dropped, testing::IsEmpty());
}

TEST_F(TestOverrideWatcher, base_and_modified_override_emits_both_as_modified)
{
    write_override_file("10-first.conf");
    auto watcher = make_watcher(); // known = {10-first.conf}

    write_override_file("10-first.conf", "updated content");
    auto result = watcher->apply_events(
        SummaryBuilder{}.base_changed(base_config).file_change(override_dir / "10-first.conf", FileChange::written));
    ASSERT_TRUE(result.has_value());
    auto const rec = record(*result);

    EXPECT_THAT(rec.modified, testing::ElementsAre(base_config, override_dir / "10-first.conf"));
    EXPECT_THAT(rec.unchanged, testing::IsEmpty());
    EXPECT_THAT(rec.fresh, testing::IsEmpty());
    EXPECT_THAT(rec.dropped, testing::IsEmpty());
}

TEST_F(TestOverrideWatcher, base_and_deleted_override_emits_modified_and_dropped)
{
    write_override_file("10-first.conf");
    auto watcher = make_watcher(); // known = {10-first.conf}

    fs::remove(override_dir / "10-first.conf");
    auto result = watcher->apply_events(
        SummaryBuilder{}.base_changed(base_config).file_change(override_dir / "10-first.conf", FileChange::removed));
    ASSERT_TRUE(result.has_value());
    auto const rec = record(*result);

    EXPECT_THAT(rec.modified, testing::ElementsAre(base_config));
    EXPECT_THAT(rec.dropped, testing::ElementsAre(override_dir / "10-first.conf"));
    EXPECT_THAT(rec.unchanged, testing::IsEmpty());
    EXPECT_THAT(rec.fresh, testing::IsEmpty());
}

TEST_F(TestOverrideWatcher, base_config_deletion_with_no_fallback_produces_no_reload)
{
    auto watcher = make_watcher();

    fs::remove(base_config);
    EXPECT_FALSE(watcher->apply_events(SummaryBuilder{}.base_removed(base_config)).has_value());
}

TEST_F(TestOverrideWatcher, base_config_deletion_with_no_fallback_and_known_overrides_produces_no_reload)
{
    // Even if there were known override files, without a base config there is
    // nothing to reload into — consistent with the initial load behaviour.
    write_override_file("10-first.conf");
    auto watcher = make_watcher(); // known = {10-first.conf}

    fs::remove(base_config);
    EXPECT_FALSE(watcher->apply_events(SummaryBuilder{}.base_removed(base_config)).has_value());
}

namespace
{
struct TestMultiRootOverrideWatcher : testing::Test
{
    XdgEnvGuard xdg_guard;
    fs::path user_dir;
    fs::path system_dir;
    fs::path base_config;
    fs::path user_override_dir;
    fs::path system_override_dir;

    void SetUp() override
    {
        user_dir = make_tmp("test-multi-root-user");
        system_dir = make_tmp("test-multi-root-system");

        base_config = user_dir / "test.conf";
        user_override_dir = user_dir / "test.conf.d";
        system_override_dir = system_dir / "test.conf.d";

        std::ofstream{base_config} << "base";

        setenv("XDG_CONFIG_HOME", user_dir.c_str(), 1);
        setenv("XDG_CONFIG_DIRS", system_dir.c_str(), 1);
    }

    void TearDown() override
    {
        fs::remove_all(user_dir);
        fs::remove_all(system_dir);
    }

    void write_user_override(std::string const& name, std::string const& content = "user-content")
    {
        fs::create_directories(user_override_dir);
        std::ofstream{user_override_dir / name} << content;
    }

    void write_system_override(std::string const& name, std::string const& content = "system-content")
    {
        fs::create_directories(system_override_dir);
        std::ofstream{system_override_dir / name} << content;
    }

    auto make_watcher() -> std::shared_ptr<mlc::OverrideWatcher>
    {
        return ::make_watcher_common(fs::path{"test.conf"});
    }
};
}


TEST_F(TestMultiRootOverrideWatcher, base_change_includes_system_overrides)
{
    write_system_override("10-system.conf");
    auto watcher = make_watcher();

    // Trigger a base config change to get the full override list
    auto result = watcher->apply_events(SummaryBuilder{}.base_changed(base_config));
    ASSERT_TRUE(result.has_value());

    auto const paths = record_paths(*result);
    EXPECT_THAT(paths, testing::Contains(system_override_dir / "10-system.conf"));
}

TEST_F(TestMultiRootOverrideWatcher, base_change_includes_overrides_from_both_roots)
{
    write_system_override("10-system.conf");
    write_user_override("20-user.conf");
    auto watcher = make_watcher();

    auto result = watcher->apply_events(SummaryBuilder{}.base_changed(base_config));
    ASSERT_TRUE(result.has_value());

    auto const paths = record_paths(*result);
    EXPECT_THAT(paths, testing::Contains(system_override_dir / "10-system.conf"));
    EXPECT_THAT(paths, testing::Contains(user_override_dir / "20-user.conf"));
}

TEST_F(TestMultiRootOverrideWatcher, files_from_different_roots_sorted_by_basename)
{
    write_system_override("50-system.conf");
    write_user_override("10-user.conf");
    auto watcher = make_watcher();

    auto result = watcher->apply_events(SummaryBuilder{}.base_changed(base_config));
    ASSERT_TRUE(result.has_value());

    EXPECT_THAT(record_paths(*result), testing::ElementsAre(
        base_config,
        user_override_dir / "10-user.conf",
        system_override_dir / "50-system.conf"));
}

TEST_F(TestMultiRootOverrideWatcher, same_basename_user_file_shadows_system_file)
{
    write_system_override("10-shared.conf", "system content");
    write_user_override("10-shared.conf", "user content");
    auto watcher = make_watcher();

    auto result = watcher->apply_events(SummaryBuilder{}.base_changed(base_config));
    ASSERT_TRUE(result.has_value());

    EXPECT_THAT(record_paths(*result), testing::ElementsAre(
        base_config,
        user_override_dir / "10-shared.conf"));
}

TEST_F(TestMultiRootOverrideWatcher, within_same_root_files_are_sorted_lexicographically)
{
    write_system_override("20-beta.conf");
    write_system_override("10-alpha.conf");
    auto watcher = make_watcher();

    auto result = watcher->apply_events(SummaryBuilder{}.base_changed(base_config));
    ASSERT_TRUE(result.has_value());

    EXPECT_THAT(record_paths(*result), testing::ElementsAre(
        base_config,
        system_override_dir / "10-alpha.conf",
        system_override_dir / "20-beta.conf"));
}

TEST_F(TestMultiRootOverrideWatcher, dropping_user_file_unshadows_system_file)
{
    write_system_override("10-shared.conf", "system content");
    write_user_override("10-shared.conf", "user content");
    auto watcher = make_watcher();

    fs::remove(user_override_dir / "10-shared.conf");
    auto result = watcher->apply_events(
        SummaryBuilder{}.file_change(user_override_dir / "10-shared.conf", FileChange::removed));
    ASSERT_TRUE(result.has_value());

    auto const rec = record(*result);
    EXPECT_THAT(rec.fresh, testing::ElementsAre(system_override_dir / "10-shared.conf"));
    EXPECT_THAT(rec.dropped, testing::IsEmpty());
}

TEST_F(TestMultiRootOverrideWatcher, dropping_only_file_for_basename_emits_drop)
{
    write_user_override("10-only.conf");
    auto watcher = make_watcher();

    fs::remove(user_override_dir / "10-only.conf");
    auto result = watcher->apply_events(
        SummaryBuilder{}.file_change(user_override_dir / "10-only.conf", FileChange::removed));
    ASSERT_TRUE(result.has_value());

    auto const rec = record(*result);
    EXPECT_THAT(rec.dropped, testing::ElementsAre(user_override_dir / "10-only.conf"));
    EXPECT_THAT(rec.fresh, testing::IsEmpty());
}

TEST_F(TestMultiRootOverrideWatcher, system_override_dir_created_triggers_reload)
{
    auto watcher = make_watcher(); // no override dirs exist yet

    write_system_override("10-system.conf");
    auto result = watcher->apply_events(SummaryBuilder{}.dir_event(system_dir, OverrideDirEvent::created));
    ASSERT_TRUE(result.has_value());

    auto const rec = record(*result);
    EXPECT_THAT(rec.fresh, testing::Contains(system_override_dir / "10-system.conf"));
}

TEST_F(TestMultiRootOverrideWatcher, system_override_file_change_triggers_reload)
{
    write_system_override("10-system.conf");
    auto watcher = make_watcher();

    write_system_override("10-system.conf", "updated");
    auto result = watcher->apply_events(
        SummaryBuilder{}.file_change(system_override_dir / "10-system.conf", FileChange::written));
    ASSERT_TRUE(result.has_value());

    auto const rec = record(*result);
    EXPECT_THAT(rec.modified, testing::Contains(system_override_dir / "10-system.conf"));
}

TEST_F(TestMultiRootOverrideWatcher, new_system_override_file_classified_as_fresh)
{
    write_system_override("10-existing.conf");
    auto watcher = make_watcher();

    write_system_override("20-new.conf");
    auto result = watcher->apply_events(
        SummaryBuilder{}.file_change(system_override_dir / "20-new.conf", FileChange::written));
    ASSERT_TRUE(result.has_value());

    auto const rec = record(*result);
    EXPECT_THAT(rec.fresh, testing::Contains(system_override_dir / "20-new.conf"));
    EXPECT_THAT(rec.unchanged, testing::Contains(system_override_dir / "10-existing.conf"));
}

TEST_F(TestMultiRootOverrideWatcher, deleted_system_override_file_classified_as_dropped)
{
    write_system_override("10-system.conf");
    write_user_override("20-user.conf");
    auto watcher = make_watcher();

    fs::remove(system_override_dir / "10-system.conf");
    auto result = watcher->apply_events(
        SummaryBuilder{}.file_change(system_override_dir / "10-system.conf", FileChange::removed));
    ASSERT_TRUE(result.has_value());

    auto const rec = record(*result);
    EXPECT_THAT(rec.dropped, testing::Contains(system_override_dir / "10-system.conf"));
    EXPECT_THAT(rec.unchanged, testing::Contains(user_override_dir / "20-user.conf"));
}

TEST_F(TestMultiRootOverrideWatcher, registers_when_home_dir_missing)
{
    fs::remove(base_config);

    std::ofstream{system_dir / "test.conf"} << "system-base";
    write_system_override("10-system.conf");

    auto nonexistent = fs::temp_directory_path() / "nonexistent-dir-for-test";
    setenv("XDG_CONFIG_HOME", nonexistent.c_str(), 1);

    auto watcher = make_watcher();

    auto result = watcher->apply_events(SummaryBuilder{}.base_changed(system_dir / "test.conf"));
    ASSERT_TRUE(result.has_value());

    auto const paths = record_paths(*result);
    EXPECT_THAT(paths, testing::Contains(system_override_dir / "10-system.conf"));
}

TEST_F(TestMultiRootOverrideWatcher, shadowed_base_config_change_does_not_trigger_reload)
{
    std::ofstream{system_dir / "test.conf"} << "system-base";
    auto watcher = make_watcher();

    auto result = watcher->apply_events(SummaryBuilder{}.base_changed(system_dir / "test.conf"));
    EXPECT_FALSE(result.has_value());
}

TEST_F(TestMultiRootOverrideWatcher, active_base_config_change_triggers_reload_as_modified)
{
    std::ofstream{system_dir / "test.conf"} << "system-base";
    auto watcher = make_watcher();

    std::ofstream{base_config} << "updated user base";
    auto result = watcher->apply_events(SummaryBuilder{}.base_changed(base_config));
    ASSERT_TRUE(result.has_value());

    auto const rec = record(*result);
    EXPECT_THAT(rec.modified, testing::Contains(base_config));
}

TEST_F(TestMultiRootOverrideWatcher, duplicate_xdg_config_home_in_config_dirs_does_not_duplicate_overrides)
{
    // If XDG_CONFIG_HOME also appears in XDG_CONFIG_DIRS, the same directory
    // would be watched twice. Override files from it must appear only once.
    write_user_override("10-user.conf");
    setenv("XDG_CONFIG_DIRS", (user_dir.string() + ":" + system_dir.string()).c_str(), 1);
    auto watcher = make_watcher();

    auto result = watcher->apply_events(SummaryBuilder{}.base_changed(base_config));
    ASSERT_TRUE(result.has_value());

    auto const paths = record_paths(*result);
    EXPECT_THAT(paths, testing::ElementsAre(base_config, user_override_dir / "10-user.conf"));
}

TEST_F(TestMultiRootOverrideWatcher, duplicate_paths_within_xdg_config_dirs_does_not_duplicate_overrides)
{
    // If XDG_CONFIG_DIRS contains the same path twice, override files from it
    // must appear only once.
    write_system_override("10-system.conf");
    setenv("XDG_CONFIG_DIRS", (system_dir.string() + ":" + system_dir.string()).c_str(), 1);
    auto watcher = make_watcher();

    auto result = watcher->apply_events(SummaryBuilder{}.base_changed(base_config));
    ASSERT_TRUE(result.has_value());

    auto const paths = record_paths(*result);
    EXPECT_THAT(paths, testing::ElementsAre(base_config, system_override_dir / "10-system.conf"));
}

TEST_F(TestMultiRootOverrideWatcher, override_dirs_created_in_both_roots_simultaneously_are_both_processed)
{
    auto watcher = make_watcher();

    write_user_override("20-user.conf");
    write_system_override("10-system.conf");
    auto result = watcher->apply_events(SummaryBuilder{}.dir_event(system_dir, OverrideDirEvent::created).dir_event(user_dir, OverrideDirEvent::created));
    ASSERT_TRUE(result.has_value());

    auto const rec = record(*result);
    EXPECT_THAT(rec.fresh, testing::Contains(system_override_dir / "10-system.conf"));
    EXPECT_THAT(rec.fresh, testing::Contains(user_override_dir / "20-user.conf"));
}

TEST_F(TestMultiRootOverrideWatcher, override_dir_created_in_one_root_and_removed_in_another_are_both_processed)
{
    write_system_override("10-system.conf");
    auto watcher = make_watcher();

    write_user_override("20-user.conf");
    fs::remove_all(system_override_dir);
    auto result = watcher->apply_events(SummaryBuilder{}.dir_event(user_dir, OverrideDirEvent::created).dir_event(system_dir, OverrideDirEvent::removed));
    ASSERT_TRUE(result.has_value());

    auto const rec = record(*result);
    EXPECT_THAT(rec.fresh, testing::ElementsAre(user_override_dir / "20-user.conf"));
    EXPECT_THAT(rec.dropped, testing::ElementsAre(system_override_dir / "10-system.conf"));
}

TEST_F(TestMultiRootOverrideWatcher, deleting_user_base_config_falls_back_to_system_base_config)
{
    std::ofstream{system_dir / "test.conf"} << "system base";
    write_system_override("10-system.conf");
    auto watcher = make_watcher();

    fs::remove(base_config);
    auto result = watcher->apply_events(SummaryBuilder{}.base_removed(base_config));
    ASSERT_TRUE(result.has_value());

    auto const rec = record(*result);
    EXPECT_THAT(rec.dropped, testing::ElementsAre(base_config));
    EXPECT_THAT(rec.fresh, testing::ElementsAre(system_dir / "test.conf"));
    EXPECT_THAT(rec.unchanged, testing::ElementsAre(system_override_dir / "10-system.conf"));
    EXPECT_THAT(rec.modified, testing::IsEmpty());
}

TEST_F(TestMultiRootOverrideWatcher, deleting_user_base_config_keeps_user_overrides_active_with_system_fallback)
{
    std::ofstream{system_dir / "test.conf"} << "system base";
    write_user_override("20-user.conf");
    write_system_override("10-system.conf");
    auto watcher = make_watcher();

    fs::remove(base_config);
    auto result = watcher->apply_events(SummaryBuilder{}.base_removed(base_config));
    ASSERT_TRUE(result.has_value());

    auto const rec = record(*result);
    EXPECT_THAT(rec.dropped, testing::ElementsAre(base_config));
    EXPECT_THAT(rec.fresh, testing::ElementsAre(system_dir / "test.conf"));
    EXPECT_THAT(rec.unchanged, testing::Contains(system_override_dir / "10-system.conf"));
    EXPECT_THAT(rec.unchanged, testing::Contains(user_override_dir / "20-user.conf"));
}

TEST_F(TestMultiRootOverrideWatcher, dir_event_on_one_root_and_file_change_on_another_are_both_processed)
{
    write_system_override("10-system.conf");
    auto watcher = make_watcher();

    write_user_override("20-user.conf");
    write_system_override("10-system.conf", "updated");
    auto result = watcher->apply_events(
        SummaryBuilder{}
            .dir_event(user_dir, OverrideDirEvent::created)
            .file_change(system_override_dir / "10-system.conf", FileChange::written));
    ASSERT_TRUE(result.has_value());

    auto const rec = record(*result);
    EXPECT_THAT(rec.fresh, testing::Contains(user_override_dir / "20-user.conf"));
    EXPECT_THAT(rec.modified, testing::Contains(system_override_dir / "10-system.conf"));
}

TEST_F(TestMultiRootOverrideWatcher, file_change_on_one_root_and_dir_removed_on_another_are_both_processed)
{
    write_system_override("10-system.conf");
    write_user_override("20-user.conf");
    auto watcher = make_watcher();

    fs::remove_all(system_override_dir);
    write_user_override("20-user.conf", "updated");
    auto result = watcher->apply_events(
        SummaryBuilder{}
            .dir_event(system_dir, OverrideDirEvent::removed)
            .file_change(user_override_dir / "20-user.conf", FileChange::written));
    ASSERT_TRUE(result.has_value());

    auto const rec = record(*result);
    EXPECT_THAT(rec.dropped, testing::Contains(system_override_dir / "10-system.conf"));
    EXPECT_THAT(rec.modified, testing::Contains(user_override_dir / "20-user.conf"));
}

namespace
{
struct TestThreeRootOverrideWatcher : testing::Test
{
    XdgEnvGuard xdg_guard;
    fs::path user_dir;
    fs::path system_dir_a;  // higher priority system dir (listed first in XDG_CONFIG_DIRS)
    fs::path system_dir_b;  // lower priority system dir (listed second)
    fs::path user_override_dir;
    fs::path system_a_override_dir;
    fs::path system_b_override_dir;

    void SetUp() override
    {
        user_dir = make_tmp("test-3root-user");
        system_dir_a = make_tmp("test-3root-sys-a");
        system_dir_b = make_tmp("test-3root-sys-b");

        user_override_dir = user_dir / "test.conf.d";
        system_a_override_dir = system_dir_a / "test.conf.d";
        system_b_override_dir = system_dir_b / "test.conf.d";

        // Base config lives in user dir
        std::ofstream{user_dir / "test.conf"} << "base";

        setenv("XDG_CONFIG_HOME", user_dir.c_str(), 1);
        // Colon-separated: system_dir_a has higher priority than system_dir_b
        auto dirs = system_dir_a.string() + ":" + system_dir_b.string();
        setenv("XDG_CONFIG_DIRS", dirs.c_str(), 1);
    }

    void TearDown() override
    {
        fs::remove_all(user_dir);
        fs::remove_all(system_dir_a);
        fs::remove_all(system_dir_b);
    }

    void write_override(fs::path const& override_dir, std::string const& name, std::string const& content = "content")
    {
        fs::create_directories(override_dir);
        std::ofstream{override_dir / name} << content;
    }

    auto make_watcher() -> std::shared_ptr<mlc::OverrideWatcher>
    {
        return ::make_watcher_common(fs::path{"test.conf"});
    }
};
}


TEST_F(TestThreeRootOverrideWatcher, overrides_from_all_three_roots_are_picked_up)
{
    write_override(system_b_override_dir, "10-lowest.conf");
    write_override(system_a_override_dir, "20-middle.conf");
    write_override(user_override_dir, "30-highest.conf");
    auto watcher = make_watcher();

    auto result = watcher->apply_events(SummaryBuilder{}.base_changed(user_dir / "test.conf"));
    ASSERT_TRUE(result.has_value());

    auto const paths = record_paths(*result);
    EXPECT_THAT(paths, testing::Contains(system_b_override_dir / "10-lowest.conf"));
    EXPECT_THAT(paths, testing::Contains(system_a_override_dir / "20-middle.conf"));
    EXPECT_THAT(paths, testing::Contains(user_override_dir / "30-highest.conf"));
}

TEST_F(TestThreeRootOverrideWatcher, distinct_basenames_sorted_lexicographically_across_all_roots)
{
    write_override(system_b_override_dir, "30-b.conf");
    write_override(system_a_override_dir, "10-a.conf");
    write_override(user_override_dir, "20-user.conf");
    auto watcher = make_watcher();

    auto result = watcher->apply_events(SummaryBuilder{}.base_changed(user_dir / "test.conf"));
    ASSERT_TRUE(result.has_value());

    EXPECT_THAT(record_paths(*result), testing::ElementsAre(
        user_dir / "test.conf",
        system_a_override_dir / "10-a.conf",
        user_override_dir / "20-user.conf",
        system_b_override_dir / "30-b.conf"));
}

TEST_F(TestThreeRootOverrideWatcher, same_basename_highest_priority_shadows_all_lower)
{
    write_override(system_b_override_dir, "10-shared.conf", "lowest");
    write_override(system_a_override_dir, "10-shared.conf", "middle");
    write_override(user_override_dir, "10-shared.conf", "highest");
    auto watcher = make_watcher();

    auto result = watcher->apply_events(SummaryBuilder{}.base_changed(user_dir / "test.conf"));
    ASSERT_TRUE(result.has_value());

    EXPECT_THAT(record_paths(*result), testing::ElementsAre(
        user_dir / "test.conf",
        user_override_dir / "10-shared.conf"));
}

TEST_F(TestThreeRootOverrideWatcher, change_in_lowest_priority_root_triggers_reload_with_all_overrides)
{
    write_override(system_b_override_dir, "10-b.conf");
    write_override(system_a_override_dir, "20-a.conf");
    write_override(user_override_dir, "30-user.conf");
    auto watcher = make_watcher();

    write_override(system_b_override_dir, "10-b.conf", "updated");
    auto result = watcher->apply_events(
        SummaryBuilder{}.file_change(system_b_override_dir / "10-b.conf", FileChange::written));
    ASSERT_TRUE(result.has_value());

    auto const paths = record_paths(*result);
    EXPECT_THAT(paths, testing::Contains(system_b_override_dir / "10-b.conf"));
    EXPECT_THAT(paths, testing::Contains(system_a_override_dir / "20-a.conf"));
    EXPECT_THAT(paths, testing::Contains(user_override_dir / "30-user.conf"));
}

TEST_F(TestThreeRootOverrideWatcher, missing_middle_root_override_dir_does_not_break_others)
{
    write_override(system_b_override_dir, "10-b.conf");
    write_override(user_override_dir, "20-user.conf");
    auto watcher = make_watcher();

    auto result = watcher->apply_events(SummaryBuilder{}.base_changed(user_dir / "test.conf"));
    ASSERT_TRUE(result.has_value());

    auto const paths = record_paths(*result);
    EXPECT_THAT(paths, testing::Contains(system_b_override_dir / "10-b.conf"));
    EXPECT_THAT(paths, testing::Contains(user_override_dir / "20-user.conf"));
    EXPECT_THAT(paths, testing::Not(testing::Contains(testing::HasSubstr("sys-a"))));
}
