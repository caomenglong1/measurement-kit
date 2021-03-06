// Part of Measurement Kit <https://measurement-kit.github.io/>.
// Measurement Kit is free software under the BSD license. See AUTHORS
// and LICENSE for more information on the copying conditions.

#define CATCH_CONFIG_MAIN
#include "src/libmeasurement_kit/ext/catch.hpp"

#include "src/libmeasurement_kit/ooni/resources_impl.hpp"

using namespace mk;

TEST_CASE("sanitize_version() works as expected") {
    REQUIRE(ooni::resources::sanitize_version("\t 1.2.3 \r\t\n  \r") ==
            "1.2.3");
}

static void get_fail(std::string, Callback<Error, SharedPtr<http::Response>> cb,
                     http::Headers, Settings, SharedPtr<Reactor>, SharedPtr<Logger>,
                     SharedPtr<http::Response>, int) {
    cb(MockedError(), nullptr);
}

static void get_500(std::string, Callback<Error, SharedPtr<http::Response>> cb,
                    http::Headers, Settings, SharedPtr<Reactor>, SharedPtr<Logger>,
                    SharedPtr<http::Response>, int) {
    SharedPtr<http::Response> response{new http::Response};
    response->status_code = 500;
    cb(NoError(), response);
}

TEST_CASE("get_latest_release() works as expected") {
    SECTION("When http::get() fails") {
        ooni::resources::get_latest_release_impl<get_fail>(
            [=](Error e, std::string s) {
                REQUIRE(e == MockedError());
                REQUIRE(s == "");
            },
            {}, Reactor::global(), Logger::global());
    }

    SECTION("When response is not a redirection") {
        ooni::resources::get_latest_release_impl<get_500>(
            [=](Error e, std::string s) {
                REQUIRE(e == ooni::CannotGetResourcesVersionError());
                REQUIRE(s == "");
            },
            {}, Reactor::global(), Logger::global());
    }

#if ENABLE_INTEGRATION_TESTS
    SECTION("Integration test") {
        SharedPtr<Reactor> reactor = Reactor::make();
        reactor->run_with_initial_event([=]() {
            ooni::resources::get_latest_release(
                [=](Error e, std::string s) {
                    REQUIRE(e == NoError());
                    REQUIRE(s != "");
                    reactor->stop();
                },
                {}, reactor, Logger::global());
        });
    }
#endif
}

static void get_invalid_json(std::string,
                             Callback<Error, SharedPtr<http::Response>> cb,
                             http::Headers, Settings, SharedPtr<Reactor>, SharedPtr<Logger>,
                             SharedPtr<http::Response>, int) {
    SharedPtr<http::Response> response{new http::Response};
    response->status_code = 200;
    response->body = "{";
    cb(NoError(), response);
}

TEST_CASE("get_manifest_as_json() works as expected") {
    SECTION("When http::get() fails") {
        ooni::resources::get_manifest_as_json_impl<get_fail>(
            "2", [=](Error e, Json s) {
                REQUIRE(e == MockedError());
                REQUIRE(s == nullptr);
            },
            {}, Reactor::global(), Logger::global());
    }

    SECTION("When response is not okay") {
        ooni::resources::get_manifest_as_json_impl<get_500>(
            "2", [=](Error e, Json s) {
                REQUIRE(e == ooni::CannotGetResourcesManifestError());
                REQUIRE(s == nullptr);
            },
            {}, Reactor::global(), Logger::global());
    }

    SECTION("When the body is not a valid JSON") {
        ooni::resources::get_manifest_as_json_impl<get_invalid_json>(
            "2", [=](Error e, Json s) {
                REQUIRE(e == JsonParseError());
                REQUIRE(s == nullptr);
            },
            {}, Reactor::global(), Logger::global());
    }
}

TEST_CASE("sanitize_path() works as expected") {
    // Important: let's also make sure that multiple sequences are stripped

    SECTION("When there are neither forward not back slashes") {
        REQUIRE(ooni::resources::sanitize_path("antani") == "antani");
    }

    SECTION("For backward slashes") {
        REQUIRE(ooni::resources::sanitize_path("/etc/passwd///")
                == ".etc.passwd.");
    }

    SECTION("For forward slashes") {
        REQUIRE(ooni::resources::sanitize_path("\\etc\\passwd\\\\\\")
                == ".etc.passwd.");
    }
}

static void get_antani_body(std::string,
                            Callback<Error, SharedPtr<http::Response>> cb,
                            http::Headers, Settings, SharedPtr<Reactor>, SharedPtr<Logger>,
                            SharedPtr<http::Response>, int) {
    SharedPtr<http::Response> response{new http::Response};
    response->status_code = 200;
    response->body = "antani";
    cb(NoError(), response);
}

static bool io_error(const std::ostream &) {
    return true;
}

TEST_CASE("get_resources_for_country() works as expected") {

    SECTION("When manifest is not an object") {
        SharedPtr<Reactor> reactor = Reactor::make();
        reactor->run_with_initial_event([=]() {
            ooni::resources::get_resources_for_country_impl(
                "6", nullptr, "IT",
                [=](Error err) {
                    REQUIRE(err == JsonDomainError());
                    reactor->stop();
                },
                {}, reactor, Logger::global());
        });
    }

    SECTION("When manifest does not contain a resources section") {
        SharedPtr<Reactor> reactor = Reactor::make();
        reactor->run_with_initial_event([=]() {
            ooni::resources::get_resources_for_country_impl(
                "6", Json::object(), "IT",
                [=](Error err) {
                    REQUIRE(err == JsonKeyError());
                    reactor->stop();
                },
                {}, reactor, Logger::global());
        });
    }

    SECTION("When manifest resources are not objects") {
        SharedPtr<Reactor> reactor = Reactor::make();
        reactor->run_with_initial_event([=]() {
            Json root;
            root["resources"].push_back(nullptr);
            root["resources"].push_back(nullptr);
            root["resources"].push_back(nullptr);
            ooni::resources::get_resources_for_country_impl(
                "6", root, "IT",
                [=](Error err) {
                    REQUIRE(err == ParallelOperationError());
                    for (size_t i = 0; i < err.child_errors.size(); ++i) {
                        REQUIRE(err.child_errors[i] ==
                                JsonDomainError());
                    }
                    reactor->stop();
                },
                {}, reactor, Logger::global());
        });
    }

    SECTION("When manifest resources do not contain the country key") {
        SharedPtr<Reactor> reactor = Reactor::make();
        reactor->run_with_initial_event([=]() {
            Json root;
            root["resources"].push_back(Json::object());
            root["resources"].push_back(Json::object());
            root["resources"].push_back(Json::object());
            ooni::resources::get_resources_for_country_impl(
                "6", root, "IT",
                [=](Error err) {
                    REQUIRE(err == ParallelOperationError());
                    for (size_t i = 0; i < err.child_errors.size(); ++i) {
                        REQUIRE(err.child_errors[i] ==
                                JsonKeyError());
                    }
                    reactor->stop();
                },
                {}, reactor, Logger::global());
        });
    }

    SECTION("When a specific country code is selected others are skipped") {
        SharedPtr<Reactor> reactor = Reactor::make();
        reactor->run_with_initial_event([=]() {
            Json root = R"({
                "resources": [{
                    "country_code": "IT"
                }, {
                    "country_code": "DE"
                }, {
                    "country_code": "FR"
                }
            ]})"_json;
            ooni::resources::get_resources_for_country(
                "6", root, "IT",
                [=](Error err) {
                    REQUIRE(err == ParallelOperationError());
                    /*
                     * JsonKeyError because `path` is missing.
                     */
                    REQUIRE(err.child_errors[0] == JsonKeyError());
                    REQUIRE(err.child_errors[1] == NoError());
                    REQUIRE(err.child_errors[2] == NoError());
                    reactor->stop();
                },
                {}, reactor, Logger::global());
        });
    }

    SECTION("The ALL selector selects all countries") {
        SharedPtr<Reactor> reactor = Reactor::make();
        reactor->run_with_initial_event([=]() {
            Json root = R"({
                "resources": [{
                    "country_code": "IT"
                }, {
                    "country_code": "DE"
                }, {
                    "country_code": "FR"
                }
            ]})"_json;
            ooni::resources::get_resources_for_country(
                "6", root, "ALL",
                [=](Error err) {
                    REQUIRE(err == ParallelOperationError());
                    /*
                     * JsonKeyError because `path` is missing.
                     */
                    REQUIRE(err.child_errors[0] == JsonKeyError());
                    REQUIRE(err.child_errors[1] == JsonKeyError());
                    REQUIRE(err.child_errors[2] == JsonKeyError());
                    reactor->stop();
                },
                {}, reactor, Logger::global());
        });
    }

    SECTION("Deals with HTTP GET errors") {
        SharedPtr<Reactor> reactor = Reactor::make();
        reactor->run_with_initial_event([=]() {
            Json root = R"({
                "resources": [{
                    "country_code": "IT",
                    "path": "xx"
                }, {
                    "country_code": "DE",
                    "path": "xx"
                }, {
                    "country_code": "FR",
                    "path": "xx"
                }
            ]})"_json;
            ooni::resources::get_resources_for_country_impl<get_fail>(
                "6", root, "ALL",
                [=](Error err) {
                    REQUIRE(err == ParallelOperationError());
                    REQUIRE(err.child_errors[0] == MockedError());
                    REQUIRE(err.child_errors[1] == MockedError());
                    REQUIRE(err.child_errors[2] == MockedError());
                    reactor->stop();
                },
                {}, reactor, Logger::global());
        });
    }

    SECTION("Deals with HTTP GET returning error") {
        SharedPtr<Reactor> reactor = Reactor::make();
        reactor->run_with_initial_event([=]() {
            Json root = R"({
                "resources": [{
                    "country_code": "IT",
                    "path": "xx"
                }, {
                    "country_code": "DE",
                    "path": "xx"
                }, {
                    "country_code": "FR",
                    "path": "xx"
                }
            ]})"_json;
            ooni::resources::get_resources_for_country_impl<get_500>(
                "6", root, "ALL",
                [=](Error err) {
                    REQUIRE(err == ParallelOperationError());
                    for (size_t i = 0; i < err.child_errors.size(); ++i) {
                        REQUIRE(err.child_errors[i] ==
                                ooni::CannotGetResourceError());
                    }
                    reactor->stop();
                },
                {}, reactor, Logger::global());
        });
    }

    SECTION("Deals with missing sha256 keys in the manifest") {
        SharedPtr<Reactor> reactor = Reactor::make();
        reactor->run_with_initial_event([=]() {
            Json root = R"({
                "resources": [{
                    "country_code": "IT",
                    "path": "xx"
                }, {
                    "country_code": "DE",
                    "path": "xx"
                }, {
                    "country_code": "FR",
                    "path": "xx"
                }
            ]})"_json;
            ooni::resources::get_resources_for_country_impl<get_antani_body>(
                "6", root, "ALL",
                [=](Error err) {
                    REQUIRE(err == ParallelOperationError());
                    for (size_t i = 0; i < err.child_errors.size(); ++i) {
                        REQUIRE(err.child_errors[i] ==
                                JsonKeyError());
                    }
                    reactor->stop();
                },
                {}, reactor, Logger::global());
        });
    }

    SECTION("Deals with invalid sha256 sums") {
        SharedPtr<Reactor> reactor = Reactor::make();
        reactor->run_with_initial_event([=]() {
            Json root = R"({
                "resources": [{
                    "country_code": "IT",
                    "path": "xx",
                    "sha256": "abc"
                }, {
                    "country_code": "DE",
                    "path": "xx",
                    "sha256": "abc"
                }, {
                    "country_code": "FR",
                    "path": "xx",
                    "sha256": "abc"
                }
            ]})"_json;
            ooni::resources::get_resources_for_country_impl<get_antani_body>(
                "6", root, "ALL",
                [=](Error err) {
                    REQUIRE(err == ParallelOperationError());
                    for (size_t i = 0; i < err.child_errors.size(); ++i) {
                        REQUIRE(err.child_errors[i] ==
                                ooni::ResourceIntegrityError());
                    }
                    reactor->stop();
                },
                {}, reactor, Logger::global());
        });
    }

    SECTION("Deals with write file I/O error") {
        SharedPtr<Reactor> reactor = Reactor::make();
        reactor->run_with_initial_event([=]() {
            Json root = R"({
                "resources": [{
                    "country_code": "IT",
                    "path": "xx",
                    "sha256": "b1dc5f0ba862fe3a1608d985ded3c5ed6b9a7418db186d9e6e6201794f59ba54"
                }, {
                    "country_code": "DE",
                    "path": "xx",
                    "sha256": "b1dc5f0ba862fe3a1608d985ded3c5ed6b9a7418db186d9e6e6201794f59ba54"
                }, {
                    "country_code": "FR",
                    "path": "xx",
                    "sha256": "b1dc5f0ba862fe3a1608d985ded3c5ed6b9a7418db186d9e6e6201794f59ba54"
                }
            ]})"_json;
            ooni::resources::get_resources_for_country_impl<get_antani_body,
                                                            io_error>(
                "6", root, "ALL",
                [=](Error err) {
                    REQUIRE(err == ParallelOperationError());
                    for (size_t i = 0; i < err.child_errors.size(); ++i) {
                        REQUIRE(err.child_errors[i] ==
                                FileIoError());
                    }
                    reactor->stop();
                },
                {}, reactor, Logger::global());
        });
    }
}

static void get_manifest_as_json_fail(std::string,
                                      Callback<Error, Json> callback,
                                      Settings, SharedPtr<Reactor>, SharedPtr<Logger>) {
    callback(MockedError(), nullptr);
}

static void get_manifest_as_json_okay(std::string,
                                      Callback<Error, Json> callback,
                                      Settings, SharedPtr<Reactor>, SharedPtr<Logger>) {
    callback(NoError(), nullptr);
}

static void get_resources_for_country_fail(std::string, Json,
                                           std::string,
                                           Callback<Error> callback, Settings,
                                           SharedPtr<Reactor>, SharedPtr<Logger>) {
    callback(MockedError());
}

TEST_CASE("get_resources() works as expected") {

    SECTION("When get_manifest_as_json() fails") {
        SharedPtr<Reactor> reactor = Reactor::make();
        reactor->run_with_initial_event([=]() {
            ooni::resources::get_resources_impl<get_manifest_as_json_fail>(
                "6", "IT",
                [=](Error error) {
                    REQUIRE(error == MockedError());
                    reactor->stop();
                },
                {}, reactor, Logger::global());
        });
    }

    SECTION("When get_resources_for_country() fails") {
        SharedPtr<Reactor> reactor = Reactor::make();
        reactor->run_with_initial_event([=]() {
            ooni::resources::get_resources_impl<get_manifest_as_json_okay,
                                                get_resources_for_country_fail>(
                "6", "IT",
                [=](Error error) {
                    REQUIRE(error == MockedError());
                    reactor->stop();
                },
                {}, reactor, Logger::global());
        });
    }

#ifdef ENABLE_INTEGRATION_TESTS
    SECTION("Integration test") {
        SharedPtr<Reactor> reactor = Reactor::make();
        SharedPtr<Logger> logger = Logger::global();
        logger->set_verbosity(MK_LOG_INFO);
        reactor->run_with_initial_event([=]() {
            ooni::resources::get_resources("6", "ALL",
                                           [=](Error error) {
                                               REQUIRE(error ==
                                                       NoError());
                                               reactor->stop();
                                           },
                                           {}, reactor, logger);
        });
    }
#endif
}
