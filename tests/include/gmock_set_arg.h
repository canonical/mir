/*
 * Copyright © 2013 Canonical Ltd.
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
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef GMOCK_SET_ARG_H_
#define GMOCK_SET_ARG_H_

#include <gmock/gmock.h>

namespace testing
{
namespace internal
{
template <size_t N, typename A, bool kIsProto>
class SetArgumentAction {
 public:
  // Constructs an action that sets the variable pointed to by the
  // N-th function argument to 'value'.
  explicit SetArgumentAction(const A& value) : value_(value) {}

  template <typename Result, typename ArgumentTuple>
  void Perform(const ArgumentTuple& args) const {
    static_assert(std::is_same<void, Result>::value, "Action must have void Result");
    ::std::get<N>(args) = value_;
  }

 private:
  const A value_;

  GTEST_DISALLOW_ASSIGN_(SetArgumentAction);
};
}
template <size_t N, typename T>
PolymorphicAction<
  internal::SetArgumentAction<
    N, T, internal::IsAProtocolMessage<T>::value> >
SetArg(const T& x) {
  return MakePolymorphicAction(internal::SetArgumentAction<
      N, T, internal::IsAProtocolMessage<T>::value>(x));
}
}

#endif /* GMOCK_SET_ARG_H_ */
