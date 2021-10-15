// StupidHTTPDownloader
// Really stupid library to download HTTP(S) content
// Copyright (C) 2021 Guillaume Vara

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#define CATCH_CONFIG_MAIN

#include <string>

#include <StupidHTTPDownloader/Downloader.h>
#include <StupidHTTPDownloader/UrlParser.h>

#include <catch2/catch.hpp>

TEST_CASE("Download HTTPS with missing PATH initiator", "[download]") {
    std::string testAddr {"https://api.ipify.org?format=json"};
    UrlParser p1(testAddr);

    REQUIRE(p1.isValid());
    REQUIRE(p1.scheme() == "https");
    REQUIRE(p1.host() == "api.ipify.org");
    REQUIRE(p1.pathAndQuery() == "/?format=json");

    auto response = Downloader::dumbGet(testAddr);
    REQUIRE(response.headers.size());
    REQUIRE(response.hasContentLengthHeader == true);
    REQUIRE(response.statusCode == 200);
    REQUIRE(response.messageBody.size());
    REQUIRE(response.redirectUrl.empty());
}

TEST_CASE("Download HTTP", "[download]") {
    std::string testAddr {"http://api.ipify.org/?format=json"};
    UrlParser p1(testAddr);

    REQUIRE(p1.isValid());
    REQUIRE(p1.scheme() == "http");
    REQUIRE(p1.host() == "api.ipify.org");
    REQUIRE(p1.pathAndQuery() == "/?format=json");

    auto response = Downloader::dumbGet(testAddr);
    REQUIRE(response.headers.size());
    REQUIRE(response.hasContentLengthHeader == true);
    REQUIRE(response.statusCode == 200);
    REQUIRE(response.messageBody.size());
    REQUIRE(response.redirectUrl.empty());
}

TEST_CASE("No detailed path", "[url_parsing]") {
    UrlQuery p1 {"format=json"};
    REQUIRE(p1.hasSubqueries());
    REQUIRE(p1.key().empty());
    
    auto sub = p1.subqueries();
    REQUIRE(sub.size() == 1);
    REQUIRE(sub[0].key() == "format");
    REQUIRE(sub[0].undecoded() == "json");
    REQUIRE(p1["format"].undecoded() == "json");
}
