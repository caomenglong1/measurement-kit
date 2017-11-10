// Part of measurement-kit <https://measurement-kit.github.io/>.
// Measurement-kit is free software under the BSD license. See AUTHORS
// and LICENSE for more information on the copying conditions.
#ifndef MEASUREMENT_KIT_COMMON_SHARED_PTR_HPP
#define MEASUREMENT_KIT_COMMON_SHARED_PTR_HPP

#include <measurement_kit/common/safe_ptr.hpp> // for mk::SafePtr

namespace mk {

/// `SharedPtr` is a null-safe wrapper for std::shared_ptr. The idea is
/// that we'd rather throw than crash if attempting to access a null pointer.
///
/// You should be able to use SharedPtr like you use std::shared_ptr. In
/// particular, you can use std::make_shared to construct objects and then
/// you can store the result of std::make_shared into a SharedPtr.
///
/// If a method has not been wrapped, used the `underlying()` method to
/// access the real, underlying smart pointer.
template <typename Type> using SharedPtr = SafePtr<std::shared_ptr<Type>>;

} // namespace mk
#endif
