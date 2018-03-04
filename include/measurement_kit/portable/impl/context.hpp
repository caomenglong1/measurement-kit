// Part of Measurement Kit <https://measurement-kit.github.io/>.
// Measurement Kit is free software under the BSD license. See AUTHORS
// and LICENSE for more information on the copying conditions.
#ifndef MEASUREMENT_KIT_PORTABLE_IMPL_CONTEXT_HPP
#define MEASUREMENT_KIT_PORTABLE_IMPL_CONTEXT_HPP

#include <measurement_kit/portable/context.hpp>

#include <measurement_kit/portable/impl/flags.h>

namespace mk {
namespace portable {

Context::~Context() noexcept {}

void Context::interrupt() noexcept { flags_ |= MK_F_INTR; }

} // namespace portable
} // namespace mk
#endif // MEASUREMENT_KIT_PORTABLE_IMPL_CONTEXT_HPP
