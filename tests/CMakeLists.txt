find_package(Catch2 REQUIRED)

add_executable(SHTTPD_tests tests.cpp)

target_link_libraries(SHTTPD_tests
    StupidHTTPDownloader
    Catch2::Catch2
    spdlog::spdlog
)

include(CTest)
list(APPEND CMAKE_MODULE_PATH ${CATCH_SOURCE_DIR}/contrib)
include(Catch)
catch_discover_tests(SHTTPD_tests)