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

#include <spdlog/spdlog.h>

#include "Downloader.h"
#include "UrlParser.h"

#include <asio/ssl/context.hpp>
#include <asio/ssl/stream.hpp>
#include <asio/ssl/rfc2818_verification.hpp>

namespace ssl = asio::ssl;

template<>
Downloader::Response Downloader::_dumbGetFromScheme<Downloader::HandledSchemes::HTTPS>(const UrlParser &url, bool head, asio::io_service &io_service, tcp::resolver::iterator resolvedEndpoints) {
    // setup SSL
    ssl::context ssl_ctx(ssl::context::sslv23);
    ssl_ctx.set_default_verify_paths();

    // initiate
    asio::ssl::stream<tcp::socket> ssl_sock(io_service, ssl_ctx);

    // Connect to host
    asio::connect(ssl_sock.lowest_layer(), resolvedEndpoints);
    ssl_sock.lowest_layer().set_option(tcp::no_delay(true));

        // Perform SSL handshake and verify the remote host's certificate.
        // TODO(amphaal) might need to implement https://stackoverflow.com/questions/39772878/reliable-way-to-get-root-ca-certificates-on-windows
        // ssl_sock.set_verify_mode(ssl::verify_peer);
        // ssl_sock.set_verify_callback(ssl::rfc2818_verification(serverName));

    // no check of certificates
    ssl_sock.set_verify_mode(ssl::verify_none);
    ssl_sock.handshake(ssl::stream<tcp::socket>::client);

    //
    return _dumbGet(ssl_sock, url, head);
}

template<>
Downloader::Response Downloader::_dumbGetFromScheme<Downloader::HandledSchemes::HTTP>(const UrlParser &url, bool head, asio::io_service &io_service, tcp::resolver::iterator resolvedEndpoints) {
    // initiate
    tcp::socket socket(io_service);

    // Try each endpoint until we successfully establish a connection.
    asio::error_code error;
    tcp::resolver::iterator end;
    do {
        socket.close();
        socket.connect(*resolvedEndpoints++, error);
    } while (error.value() && resolvedEndpoints != end);

    //
    return _dumbGet(socket, url, head);
}

template<typename Sock>
Downloader::Response Downloader::_dumbGet(Sock& sock, const UrlParser &url, bool head) {
    // start writing
    asio::streambuf request;
    std::ostream request_stream(&request);

    auto method = head ? "HEAD" : "GET";

    const auto getCommand = url.pathAndQuery();
    const auto host = url.host();

    request_stream << method << " " << getCommand << " HTTP/1.0\r\n";
    request_stream << "Host: " << host << "\r\n";
    request_stream << "Accept: */*\r\n";
    request_stream << "Connection: close\r\n\r\n";

    // Send the request.
    asio::write(sock, request);

    if (!head)
        spdlog::debug("StupidHTTPDownloader : Downloading [{}, {}]...",
            host, getCommand);

    // Read the response status line.
    asio::streambuf response;
    asio::read_until(sock, response, "\r\n");

        // Check that response is OK.
        std::istream response_stream(&response);

        // HTTP VERSION
        std::string http_version;
        response_stream >> http_version;

        // STATUS CODE
        unsigned int status_code;
        response_stream >> status_code;

        // STATUS MESSAGE
        std::string status_message;
        std::getline(response_stream, status_message);

    // Read the response headers, which are terminated by a blank line.
    asio::read_until(sock, response, "\r\n\r\n");

    // Process the response headers.
    std::string headerTmp;
    std::vector<std::string> headers;
    bool hasContentLengthHeader = false;
    std::string redirectUrl;

    // iterate to get headers
    while (std::getline(response_stream, headerTmp) && headerTmp != "\r") {
        headers.push_back(headerTmp);

        //
        if (!hasContentLengthHeader) {
            hasContentLengthHeader =
                headerTmp.find("Content-Length") != std::string::npos;
        }

        // find redirection url
        if (status_code == 302 && redirectUrl.empty()) {
            auto found = headerTmp.find(LocationTag);
            if (found == std::string::npos) continue;
            redirectUrl = headerTmp.substr(found + LocationTag.size());
        }
    }

    if (!headers.size())
        throw std::logic_error("StupidHTTPDownloader : Response have no headers !");

    // if not HEAD, read body message
    std::ostringstream output_stream;
    if (!head) {
        // Write whatever content we already have to output.
        if (response.size() > 0) {
            output_stream << &response;
        }
        // Read until EOF, writing data to output as we go.
        asio::error_code error;
        while (asio::read(sock, response, asio::transfer_at_least(1), error)) {
            output_stream << &response;
        }
    }

    spdlog::debug("StupidHTTPDownloader : Finished downloading [{}, {}]",
        host, getCommand);

    Response outResponse {
        hasContentLengthHeader,
        status_code,
        redirectUrl,
        output_stream.str(),
        headers
    };

    spdlog::debug("StupidHTTPDownloader : Response length {}, headers {}",
        outResponse.messageBody.size(),
        outResponse.headers.size());

    return outResponse;
}

Downloader::Response Downloader::dumbGet(const std::string &downloadUrl, bool head) {
    // decompose url
    UrlParser url_decomposer(downloadUrl);
    auto serverName = url_decomposer.host();
    auto scheme = url_decomposer.scheme();

    // setup service
    asio::io_service io_service;

    // resolve IP
    tcp::resolver resolver(io_service);
    tcp::resolver::query query(serverName, scheme);
    tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

    // switch HTTP / HTTPS
    if (scheme == "https")
        return _dumbGetFromScheme<HandledSchemes::HTTPS>(url_decomposer, head, io_service, endpoint_iterator);
    else
        return _dumbGetFromScheme<HandledSchemes::HTTP> (url_decomposer, head, io_service, endpoint_iterator);
}
