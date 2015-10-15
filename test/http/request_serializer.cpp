// Part of measurement-kit <https://measurement-kit.github.io/>.
// Measurement-kit is free software. See AUTHORS and LICENSE for more
// information on the copying conditions.

//
// Regression tests for `protocols/http.hpp` and `protocols/http.cpp`.
//

#define CATCH_CONFIG_MAIN
#include "src/ext/Catch/single_include/catch.hpp"

#include <measurement_kit/common.hpp>
#include <measurement_kit/http.hpp>

using namespace measurement_kit::common;
using namespace measurement_kit::net;
using namespace measurement_kit::http;

TEST_CASE("HTTP Request serializer works as expected") {
    auto serializer = RequestSerializer({
        {"follow_redirects", "yes"},
        {"url", "http://www.example.com/antani?clacsonato=yes#melandri"},
        {"ignore_body", "yes"},
        {"method", "GET"},
        {"http_version", "HTTP/1.0"},
    }, {
        {"User-Agent", "Antani/1.0.0.0"},
    }, "0123456789");
    Buffer buffer;
    serializer.serialize(buffer);
    auto serialized = buffer.read();
    std::string expect = "GET /antani?clacsonato=yes HTTP/1.0\r\n";
    expect += "User-Agent: Antani/1.0.0.0\r\n";
    expect += "Host: www.example.com\r\n";
    expect += "Content-Length: 10\r\n";
    expect += "\r\n";
    expect += "0123456789";
    REQUIRE(serialized == expect);
}