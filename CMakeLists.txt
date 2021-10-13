cmake_minimum_required(VERSION 3.10.2)

project(StupidHTTPDownloader
    VERSION 0.0.1
    DESCRIPTION "Really stupid library to download HTTP(S) content"
    HOMEPAGE_URL "https://github.com/Amphaal/StupidHTTPDownloader"
    LANGUAGES CXX
)

#cpp standards
SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(asio REQUIRED)
find_package(spdlog REQUIRED)
find_package(OpenSSL REQUIRED)

add_library(StupidHTTPDownloader STATIC)
target_link_directories(StupidHTTPDownloader 
    PUBLIC include
)
target_sources(StupidHTTPDownloader PRIVATE
    src/Downloader.cpp
    src/UrlParser.cpp
)

target_link_libraries(StupidHTTPDownloader PRIVATE
    asio
    spdlog
    OpenSSL::Crypto
    OpenSSL::SSL
)

#dont build tests if included as submodule
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)
    add_subdirectory(tests)
endif()
