// Part of Measurement Kit <https://measurement-kit.github.io/>.
// Measurement Kit is free software under the BSD license. See AUTHORS
// and LICENSE for more information on the copying conditions.
#ifndef MEASUREMENT_KIT_PORTABLE_IMPL_INSTANCE_HPP
#define MEASUREMENT_KIT_PORTABLE_IMPL_INSTANCE_HPP

#include <measurement_kit/portable/context.hpp>

namespace mk {
namespace portable {

/*static*/ Context *Context::get_instance(std::thread::id id) noexcept {
    std::unique_lock<std::mutex> _{global_mutex_};
    if (global_map_.count(id) <= 0) {
        global_map_[id] = new Context;
    }
    return global_map_.at(id);
}

/*static*/ void Context::set_instance(
        std::thread::id id, Context *ctx) noexcept {
    clear_instance(id); // must be before locking the mutex
    std::unique_lock<std::mutex> _{global_mutex_};
    global_map_[id] = ctx;
}

/*static*/ void Context::clear_instance(std::thread::id id) noexcept {
    std::unique_lock<std::mutex> _{global_mutex_};
    if (global_map_.count(id) != 0) {
        delete global_map_.at(id);
    }
}

} // namespace portable
} // namespace mk
#endif // MEASUREMENT_KIT_PORTABLE_IMPL_INSTANCE_HPP
