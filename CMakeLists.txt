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

####################
## Deps : OpenSSL ##
####################

if(APPLE)
    set(OPENSSL_ROOT_DIR "/usr/local/opt/openssl@3")
endif()
find_package(OpenSSL REQUIRED)

#################
## Deps : asio ##
#################

add_library(asio INTERFACE)
add_library(asio::asio ALIAS asio)

find_path(Asio_INCLUDE_DIR NAMES asio.hpp REQUIRED)
target_include_directories(asio INTERFACE ${Asio_INCLUDE_DIR})

target_link_libraries(asio INTERFACE 
    OpenSSL::Crypto 
    OpenSSL::SSL
)

if(WIN32)
    target_compile_definitions(asio INTERFACE WIN32_LEAN_AND_MEAN) # prevents <winsock2.h> warnings
    target_link_libraries(asio INTERFACE ws2_32 mswsock)
endif()

###################
## Deps : spdlog ##
###################

find_package(spdlog REQUIRED)

##
## declare library
##

add_library(StupidHTTPDownloader STATIC)
target_include_directories(StupidHTTPDownloader 
    INTERFACE include
    PRIVATE include/StupidHTTPDownloader
)
target_sources(StupidHTTPDownloader PRIVATE
    src/Downloader.cpp
    src/UrlParser.cpp
)

target_link_libraries(StupidHTTPDownloader PRIVATE
    asio::asio
    spdlog::spdlog
    OpenSSL::Crypto
    OpenSSL::SSL
)

# https://bugs.llvm.org/show_bug.cgi?id=50299
if (APPLE)
    target_compile_definitions(StupidHTTPDownloader PRIVATE 
        _LIBCPP_DISABLE_DEPRECATION_WARNINGS
    )
endif()

#dont build tests if included as submodule
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)
    add_subdirectory(tests)
endif()
