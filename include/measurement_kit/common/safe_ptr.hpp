// Part of measurement-kit <https://measurement-kit.github.io/>.
// Measurement-kit is free software under the BSD license. See AUTHORS
// and LICENSE for more information on the copying conditions.
#ifndef MEASUREMENT_KIT_COMMON_SAFE_PTR_HPP
#define MEASUREMENT_KIT_COMMON_SAFE_PTR_HPP

#include <stdexcept>   // for std::runtime_error
#include <type_traits> // for std::add_pointer_type

namespace mk {

/// # SafePtr
///
/// SafePtr is a generic wrapper for smart pointers where, when the wrapped
/// pointer value is null, we throw a `std::runtime_error` exception.
template <typename SmartPtrType> class SafePtr {
  public:
    using element_type = typename SmartPtrType::element_type;

    /// The get() method returns the underlying pointer, if that is not null, or
    /// throws otherwise. This provides the guarantee that we are not going to
    /// dereference a null pointer after some bad refactoring.
    typename std::add_pointer<element_type>::type get() const {
        if (ptr_.get() == nullptr) {
            throw std::runtime_error("null pointer");
        }
        return ptr_.get();
    }

    /// The operator->() method is syntactic sugar for get().
    typename std::add_pointer<element_type>::type operator->() const {
        return get();
    }

    /// The operator*() method returns a reference to the pointee.
    typename std::add_lvalue_reference<element_type>::type operator*() const {
        if (ptr_.get() == nullptr) {
            throw std::runtime_error("null pointer");
        }
        return ptr_.operator*();
    }

    /// The operator bool() method tells you if the pointee is non-null.
    operator bool() const { return static_cast<bool>(ptr_); }

    /// The constructor with pointer takes ownership of the pointer argument.
    SafePtr(SmartPtrType &&p) : ptr_{std::move(p)} {}

    /// The default constructor constructs an empty pointer.
    SafePtr() {}

    /// The underlying() method returns the underlying pointer.
    SmartPtrType &underlying() { return ptr_; }

  private:
    SmartPtrType ptr_;
};

} // namespace mk
#endif
