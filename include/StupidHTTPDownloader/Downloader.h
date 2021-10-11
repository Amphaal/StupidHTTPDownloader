// AudioTube C++
// C++ fork based on https://github.com/Tyrrrz/YoutubeExplode
// Copyright (C) 2019-2021 Guillaume Vara

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#pragma once

#ifdef _WIN32
    #include <windows.h>
    #include <winsock2.h>  // put winsock2.h here since spdlog.h calls windows.h
#endif

#include <string>
#include <vector>

#include <asio.hpp>
#include <asio/ssl/context.hpp>
#include <asio/ssl/stream.hpp>
#include <asio/ssl/rfc2818_verification.hpp>

#include "UrlParser.h"

using asio::ip::tcp;
namespace ssl = asio::ssl;

class Downloader {
 public:
    struct Response {
        std::string messageBody;
        std::vector<std::string> headers;
        bool hasContentLengthHeader = false;
        unsigned int statusCode = 0;
        std::string redirectUrl;
    };
    using DownloadedUtf8 = std::string;
    static NetworkHelper::Response dumbGet(const std::string &downloadUrl, bool head = false);
    static constexpr std::string_view LocationTag = "Location: ";
};

