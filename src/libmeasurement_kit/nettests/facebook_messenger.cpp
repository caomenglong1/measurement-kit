// Part of Measurement Kit <https://measurement-kit.github.io/>.
// Measurement Kit is free software under the BSD license. See AUTHORS
// and LICENSE for more information on the copying conditions.

#include "src/libmeasurement_kit/nettests/runnable.hpp"

#include <measurement_kit/nettests.hpp>
#include <measurement_kit/ooni.hpp>

namespace mk {
namespace nettests {

FacebookMessengerTest::FacebookMessengerTest() : BaseTest() {
    runnable.reset(new FacebookMessengerRunnable);
}

FacebookMessengerRunnable::FacebookMessengerRunnable() noexcept {
    test_name = "facebook_messenger";
    test_version = "0.0.2";
    needs_input = false;
}

void FacebookMessengerRunnable::main(std::string /*input*/, Settings options,
                            Callback<SharedPtr<report::Entry>> cb) {
    ooni::facebook_messenger(options, cb, reactor, logger);
}

} // namespace nettests
} // namespace mk
