/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */


#ifndef MIR_ANDROID_UBUNTU_PROPERTYMAP_H_
#define MIR_ANDROID_UBUNTU_PROPERTYMAP_H_

#include <std/String8.h>
#include <std/Errors.h>

#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/lexical_cast.hpp>

#include <fstream>

namespace mir_input
{
/*
 * Provides a mechanism for passing around string-based property key / value pairs
 * and loading them from property files.
 *
 * The property files have the following simple structure:
 *
 * # Comment
 * key = value
 *
 * Keys and values are any sequence of printable ASCII characters.
 * The '=' separates the key from the value.
 * The key and value may not contain whitespace.
 *
 * The '\' character is reserved for escape sequences and is not currently supported.
 * The '"" character is reserved for quoting and is not currently supported.
 * Files that contain the '\' or '"' character will fail to parse.
 *
 * The file must not contain duplicate keys.
 *
 * TODO Support escape sequences and quoted values when needed.
 */
class PropertyMap {
public:
//    /* Creates an empty property map. */
//    PropertyMap();
//    ~PropertyMap();

    /* Clears the property map. */
    void clear() { options = boost::program_options::variables_map(); }

    /* Adds a property.
     * Replaces the property with the same key if it is already present.
     */
    void addProperty(const String8& key, const String8& value)
    {
        namespace po = boost::program_options;
        options.insert(std::make_pair(key, po::variable_value(value, false)));
    }

    void addAll(const PropertyMap* other)
    {
        options.insert(other->options.begin(), other->options.end());
    }

//
//    /* Returns true if the property map contains the specified key. */
//    bool hasProperty(const String8& key) const;

    /* Gets the value of a property and parses it.
     * Returns true and sets outValue if the key was found and its value was parsed successfully.
     * Otherwise returns false and does not modify outValue.
     */
    bool tryGetProperty(const String8& key, String8& outValue) const
    {
        if (!options.count(key)) return false;
        outValue = options[key].as<String8>();
        return true;
    }

    bool tryGetProperty(const String8& key, bool& outValue) const
    {
        return tryGetPropertyImpl(key, outValue);
    }

    bool tryGetProperty(const String8& key, int32_t& outValue) const
    {
        return tryGetPropertyImpl(key, outValue);
    }

    bool tryGetProperty(const String8& key, float& outValue) const
    {
        return tryGetPropertyImpl(key, outValue);
    }

//    /* Adds all values from the specified property map. */
//    void addAll(const PropertyMap* map);
//
//    /* Gets the underlying property map. */
//    inline const KeyedVector<String8, String8>& getProperties() const { return mProperties; }

    /* Loads a property map from a file. */
    static status_t load(const String8& filename, PropertyMap** outMap)
    {
        namespace po = boost::program_options;

        try
        {
            po::options_description description;

            std::ifstream file(filename);
            auto parsed_options = po::parse_config_file(file, description, true);

            // register the options we found so they'll be stored
            for (auto& option : parsed_options.options)
            {
                description.add_options()(option.string_key.data(), "");
                option.unregistered = false;
            }

            *outMap = new PropertyMap();
            po::store(parsed_options, (*outMap)->options);

            return NO_ERROR;
        }
        catch (std::exception const& error)
        {
            return BAD_VALUE;
        }
    }


private:
    template<typename Type>
    bool tryGetPropertyImpl(const String8& key, Type& outValue) const
    {
        if (!options.count(key)) return false;
        try
        {
            outValue = boost::lexical_cast<Type>(options[key].as<String8>());
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    boost::program_options::variables_map options;
};
}

namespace android
{
using ::mir_input::PropertyMap;
}

#endif /* MIR_ANDROID_UBUNTU_PROPERTYMAP_H_ */
