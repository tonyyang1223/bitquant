cmake_minimum_required(VERSION 3.20)
project(bitquant VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# Options
option(BUILD_TESTS "Build test suite" ON)
option(BUILD_EXAMPLES "Build examples" ON)
option(BUILD_BENCHMARKS "Build benchmarks" OFF)
option(ENABLE_BINANCE_API "Enable Binance API integration" ON)

# Find dependencies
find_package(TA-Lib REQUIRED)

# Find Boost (required for binapi)
if(ENABLE_BINANCE_API)
    find_package(Boost 1.70 REQUIRED)
    find_package(OpenSSL REQUIRED)
endif()

# Main library
add_library(bitquant
    src/core/types.cpp
    src/data/ring_buffer.cpp
    src/data/array_manager.cpp
    src/engine/broker.cpp
    src/engine/strategy.cpp
    src/engine/event.cpp
    src/engine/risk_manager.cpp
    src/engine/strategy_manager.cpp
    src/engine/strategy_context.cpp
    src/engine/trading_engine.cpp
    src/statistics/performance.cpp
    src/utils/datetime.cpp
    src/utils/logger.cpp
    src/utils/thread_pool.cpp
    src/config/config.cpp
)

target_include_directories(bitquant
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(bitquant
    PUBLIC
        TA-Lib::TA-Lib
)

# Binance API integration
if(ENABLE_BINANCE_API)
    # binapi library (header-only plus sources)
    set(BINAPI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../binapi")

    add_library(binapi STATIC
        ${BINAPI_DIR}/src/api.cpp
        ${BINAPI_DIR}/src/enums.cpp
        ${BINAPI_DIR}/src/errors.cpp
        ${BINAPI_DIR}/src/pairslist.cpp
        ${BINAPI_DIR}/src/reports.cpp
        ${BINAPI_DIR}/src/tools.cpp
        ${BINAPI_DIR}/src/types.cpp
        ${BINAPI_DIR}/src/websocket.cpp
    )

    target_include_directories(binapi PUBLIC
        ${BINAPI_DIR}/include
        ${Boost_INCLUDE_DIRS}
    )

    target_link_libraries(binapi
        PUBLIC
            OpenSSL::SSL
            OpenSSL::Crypto
            z
            pthread
    )

    target_compile_definitions(binapi PUBLIC DTF_HEADER_ONLY)

    # Add binance_api module to bitquant
    target_sources(bitquant PRIVATE
        src/exchange/binance_api.cpp
        src/exchange/binance_gateway.cpp
        src/exchange/exchange_factory.cpp
        src/exchange/websocket_client.cpp
    )

    target_link_libraries(bitquant PUBLIC binapi)
endif()

# Tests
if(BUILD_TESTS)
    enable_testing()
    find_package(GTest QUIET)

    if(GTest_FOUND)
        add_executable(bitquant_tests
            tests/unit/test_ring_buffer.cpp
            tests/unit/test_array_manager.cpp
            tests/unit/test_broker.cpp
        )

        target_link_libraries(bitquant_tests
            PRIVATE
                bitquant
                GTest::gtest
                GTest::gtest_main
        )

        include(GoogleTest)
        gtest_discover_tests(bitquant_tests)
    else()
        message(STATUS "GTest not found, tests will not be built")
    endif()
endif()

# Examples
if(BUILD_EXAMPLES)
    add_executable(demo_strategy examples/demo_strategy.cpp)
    target_link_libraries(demo_strategy PRIVATE bitquant)

    if(ENABLE_BINANCE_API)
        add_executable(demo_binance examples/demo_binance.cpp)
        target_link_libraries(demo_binance PRIVATE bitquant)

        add_executable(demo_exchange examples/demo_exchange.cpp)
        target_link_libraries(demo_exchange PRIVATE bitquant)

        add_executable(demo_multi_strategy examples/demo_multi_strategy.cpp)
        target_link_libraries(demo_multi_strategy PRIVATE bitquant)
    endif()
endif()

# Print configuration
message(STATUS "=== BitQuant C++ Configuration ===")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build tests: ${BUILD_TESTS}")
message(STATUS "Build examples: ${BUILD_EXAMPLES}")
message(STATUS "=================================")
