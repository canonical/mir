/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 2 or 3 as
 * published by the Free Software Foundation.
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

#ifndef MIR_RAII_H_
#define MIR_RAII_H_

#include <memory>
#include <type_traits>

namespace mir
{
/// Utilities for exception safe use of paired function calls
namespace raii
{
template <typename Creator, typename Deleter>
struct PairedCalls
{
    PairedCalls(Creator&& creator, Deleter&& deleter) : deleter(std::move(deleter)), owner(true) { creator(); }
    PairedCalls(PairedCalls&& that) : deleter(that.deleter), owner(that.owner) { that.owner = false; }
    ~PairedCalls()  { if (owner) deleter(); }
private:
    PairedCalls(PairedCalls const& that) = delete;
    PairedCalls& operator=(PairedCalls const& that) = delete;
    Deleter const deleter;
    bool owner;
};

/**
 * Creates an RAII object from a creator and deleter.
 * If creator returns a pointer type then the returned object
 * is a std::unique_ptr initialized with the pointer and deleter.
 * Otherwise, the returned object calls creator on construction and deleter on destruction
 *
 * \param creator called to initialize the returned object
 * \param deleter called to finalize the returned object
 */
template <typename Creator, typename Deleter>
inline auto paired_calls(Creator&& creator, Deleter&& deleter)
-> std::unique_ptr<typename std::remove_reference<decltype(*creator())>::type, Deleter>
{
    return {creator(), deleter};
}

///\overload
template <typename Creator, typename Deleter>
inline auto paired_calls(Creator&& creator, Deleter&& deleter)
-> typename std::enable_if<
    std::is_void<decltype(creator())>::value,
    PairedCalls<Creator, Deleter>>::type
{
    return {std::move(creator), std::move(deleter)};
}

/**
 * Creates an RAII object from an owning pointer and deleter.
 * The returned object is a std::unique_ptr initialized with the pointer and deleter.
 *
 * \param owned   the object to take ownership of
 * \param deleter called to finalize the owned object
 */
template <typename Owned, typename Deleter>
inline auto deleter_for(Owned* owned, Deleter&& deleter)
-> std::unique_ptr<Owned, Deleter>
{
    return {owned, deleter};
}
}
}

#endif /* MIR_RAII_H_ */
