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

#pragma once

#include <string>
#include <vector>
#include <string_view>

#include <asio.hpp>
using asio::ip::tcp;

class UrlParser;

class Downloader {
 public:
    struct Response {
        bool hasContentLengthHeader = false;
        unsigned int statusCode = 0;
        std::string redirectUrl;
        std::string messageBody;
        std::vector<std::string> headers;
    };

    static Response dumbGet(const std::string &downloadUrl, bool head = false);

 private:
    enum class HandledSchemes {
        HTTP,
        HTTPS
    };

    template<HandledSchemes scheme>
    static Response _dumbGetFromScheme(const UrlParser &url, bool head, asio::io_service &io_service, tcp::resolver::iterator resolvedEndpoints);

    template<typename Sock>
    static Response _dumbGet(Sock& sock, const UrlParser &url, bool head);

    static constexpr std::string_view LocationTag = "Location: ";
};
