//
// Copyright © 2012 Canonical Ltd. Copyright 2007, Google Inc.
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Author: wan@google.com (Zhanyong Wan)
// Authored by: Alan Griffiths <alan@octopull.co.uk>

#ifndef MIR_TEST_GMOCK_FIXES_H_
#define MIR_TEST_GMOCK_FIXES_H_

#include <memory>
#include <gmock/gmock.h>

namespace testing
{
namespace internal
{

template<typename T, typename D>
class ActionResultHolder<std::unique_ptr<T, D>>
: public UntypedActionResultHolderBase {
 public:
  explicit ActionResultHolder(std::unique_ptr<T, D>&& a_value) :
  value_(std::move(a_value)) {}

  // The compiler-generated copy constructor and assignment operator
  // are exactly what we need, so we don't need to define them.

  std::unique_ptr<T, D> Unwrap() {
    return std::move(value_);
  }

  // Returns the held value and deletes this object.
  std::unique_ptr<T, D> GetValueAndDelete() const {
      std::unique_ptr<T, D> retval(std::move(value_));
    delete this;
    return retval;
  }

  // Prints the held value as an action's result to os.
  virtual void PrintAsActionResult(::std::ostream* os) const {
    *os << "\n          Returns: ";
    // T may be a reference type, so we don't use UniversalPrint().
    UniversalPrinter<std::unique_ptr<T, D>>::Print(value_, os);
  }

  // Performs the given mock function's default action and returns the
  // result in a new-ed ActionResultHolder.
  template <typename F>
  static ActionResultHolder* PerformDefaultAction(
      const FunctionMockerBase<F>* func_mocker,
      typename Function<F>::ArgumentTuple args,
      const string& call_description) {
    return new ActionResultHolder(
        func_mocker->PerformDefaultAction(std::move(args), call_description));
  }

  // Performs the given action and returns the result in a new-ed
  // ActionResultHolder.
  template <typename F>
  static ActionResultHolder*
  PerformAction(const Action<F>& action,
                const typename Function<F>::ArgumentTuple& args) {
    return new ActionResultHolder(action.Perform(args));
  }

 private:
  std::unique_ptr<T, D> mutable value_;

  // T could be a reference type, so = isn't supported.
  GTEST_DISALLOW_ASSIGN_(ActionResultHolder);
};

}

template<typename T>
class DefaultValue<std::unique_ptr<T, std::default_delete<T>>> {
 public:
  // Unsets the default value for type T.
  static void Clear() {}

  // Returns true if the user has set the default value for type T.
  static bool IsSet() { return false; }

  // Returns true if T has a default return value set by the user or there
  // exists a built-in default value.
  static bool Exists() {
    return true;
  }

  // Returns the default value for type T if the user has set one;
  // otherwise returns the built-in default value if there is one;
  // otherwise aborts the process.
  static std::unique_ptr<T> Get() {
    return std::unique_ptr<T>();
  }
};

template<typename T>
class DefaultValue<std::unique_ptr<T, void(*)(T*)>> {
 public:
  // Unsets the default value for type T.
  static void Clear() {}

  // Returns true if the user has set the default value for type T.
  static bool IsSet() { return false; }

  // Returns true if T has a default return value set by the user or there
  // exists a built-in default value.
  static bool Exists() {
    return true;
  }

  // Returns the default value for type T if the user has set one;
  // otherwise returns the built-in default value if there is one;
  // otherwise aborts the process.
  static std::unique_ptr<T, void(*)(T*)> Get() {
    return std::unique_ptr<T, void(*)(T*)>(nullptr, nullptr);
  }
};

template<typename T, typename D>
class DefaultValue<std::unique_ptr<T, D>> {
 public:
  // Unsets the default value for type T.
  static void Clear() {}

  // Returns true if the user has set the default value for type T.
  static bool IsSet() { return false; }

  // Returns true if T has a default return value set by the user or there
  // exists a built-in default value.
  static bool Exists() {
    return true;
  }

  // Returns the default value for type T if the user has set one;
  // otherwise returns the built-in default value if there is one;
  // otherwise aborts the process.
  static std::unique_ptr<T, D> Get() {
    return std::unique_ptr<T, D>();
  }
};



}

#endif /* MIR_TEST_GMOCK_FIXES_H_ */
