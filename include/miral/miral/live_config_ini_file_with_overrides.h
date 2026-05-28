/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
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

#ifndef MIRAL_INI_FILE_WITH_OVERRIDES_H
#define MIRAL_INI_FILE_WITH_OVERRIDES_H

#include <miral/live_config.h>

#include <memory>
#include <span>

namespace miral::live_config
{
class OverridesList;

/// An ini-file-based live configuration store that aggregates values from
/// multiple files (a base config plus zero or more override files).
///
/// Non-array keys follow last-writer-wins semantics: if the same key appears
/// in a later (higher-priority) file, it replaces the earlier value.
///
/// Array keys accumulate across files; an empty assignment (\c key=) in a
/// later file clears all previously accumulated values from earlier files.
///
/// A single call to load() wraps the entire set of files in one transaction,
/// so all attribute handlers and \c on_done fire exactly once per call.
///
/// \remark Since MirAL 5.8
class IniFileWithOverrides : public Store
{
public:
    IniFileWithOverrides();
    ~IniFileWithOverrides() override;

    IniFileWithOverrides(IniFileWithOverrides const&) = delete;
    IniFileWithOverrides& operator=(IniFileWithOverrides const&) = delete;

    void add_int_attribute(Key const& key, std::string_view description, HandleInt handler) override;
    void add_ints_attribute(Key const& key, std::string_view description, HandleInts handler) override;
    void add_bool_attribute(Key const& key, std::string_view description, HandleBool handler) override;
    void add_float_attribute(Key const& key, std::string_view description, HandleFloat handler) override;
    void add_floats_attribute(Key const& key, std::string_view description, HandleFloats handler) override;
    void add_string_attribute(Key const& key, std::string_view description, HandleString handler) override;
    void add_strings_attribute(Key const& key, std::string_view description, HandleStrings handler) override;

    void add_int_attribute(Key const& key, std::string_view description, int preset, HandleInt handler) override;
    void add_ints_attribute(
        Key const& key,
        std::string_view description,
        std::span<int const> preset,
        HandleInts handler) override;
    void add_bool_attribute(Key const& key, std::string_view description, bool preset, HandleBool handler) override;
    void add_float_attribute(Key const& key, std::string_view description, float preset, HandleFloat handler) override;
    void add_floats_attribute(
        Key const& key,
        std::string_view description,
        std::span<float const> preset,
        HandleFloats handler) override;
    void add_string_attribute(
        Key const& key,
        std::string_view description,
        std::string_view preset,
        HandleString handler) override;
    void add_strings_attribute(
        Key const& key,
        std::string_view description,
        std::span<std::string const> preset,
        HandleStrings handler) override;

    void on_done(HandleDone handler) override;

    /// Parse a batch of file-system events and dispatch all handlers once.
    ///
    /// \c on_done fires exactly once per call.
    ///
    /// \remark Since MirAL 5.8
    void load(OverridesList const& changes);

private:
    class Self;
    std::shared_ptr<Self> self;
};
}

#endif // MIRAL_INI_FILE_WITH_OVERRIDES_H
