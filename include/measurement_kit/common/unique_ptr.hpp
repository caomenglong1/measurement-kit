// Part of measurement-kit <https://measurement-kit.github.io/>.
// Measurement-kit is free software under the BSD license. See AUTHORS
// and LICENSE for more information on the copying conditions.
#ifndef MEASUREMENT_KIT_COMMON_UNIQUE_PTR_HPP
#define MEASUREMENT_KIT_COMMON_UNIQUE_PTR_HPP

#include <measurement_kit/common/safe_ptr.hpp> // for mk::SafePtr

namespace mk {

// # UniquePtr
//
// UniquePtr is a wrapper for `std::unique_ptr` where accessing a null
// pointer will throw a `std::runtime_error` exception.
template <typename Type, typename TypeDeleter = std::default_delete<Type>>
using UniquePtr = SafePtr<std::unique_ptr<Type, TypeDeleter>>;

} // namespace mk
#endif
