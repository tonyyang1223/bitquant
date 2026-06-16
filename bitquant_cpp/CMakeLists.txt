cmake_minimum_required(VERSION 3.20)
project(bitquant VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Add cmake module path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

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
option(ENABLE_COVERAGE "Enable code coverage" OFF)

# Code coverage configuration
if(ENABLE_COVERAGE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage -fprofile-arcs -ftest-coverage")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
endif()

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
    src/data/bar_generator.cpp
    src/data/price_smoother.cpp
    src/data/market_state_analyzer.cpp
    src/database/sqlite_database.cpp
    src/engine/broker.cpp
    src/engine/strategy.cpp
    src/engine/event.cpp
    src/engine/risk_manager.cpp
    src/engine/strategy_manager.cpp
    src/engine/strategy_context.cpp
    src/engine/trading_engine.cpp
    src/engine/main_engine.cpp
    src/engine/strategy_optimizer.cpp
    src/statistics/performance.cpp
    src/utils/datetime.cpp
    src/utils/logger.cpp
    src/utils/thread_pool.cpp
    src/config/config.cpp
    src/exchange/exchange_factory.cpp
    src/strategy/builtin_strategies.cpp
    src/strategy/validation_strategy.cpp
    src/strategy/grid_martin_strategy.cpp
)

target_include_directories(bitquant
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(bitquant
    PUBLIC
        TA-Lib::TA-Lib
        sqlite3
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
        src/exchange/websocket_client.cpp
        src/exchange/binance_mapping.cpp
        src/exchange/binance_spot_rest_api.cpp
        src/exchange/binance_spot_gateway.cpp
        src/exchange/binance_spot_ws_api.cpp
        src/exchange/binance_usdt_gateway.cpp
        src/exchange/binance_usdt_rest_api.cpp
        src/exchange/binance_usdt_ws_api.cpp
        src/exchange/okx_gateway.cpp
        src/exchange/offset_converter.cpp
        src/engine/paper_broker.cpp
    )

    target_link_libraries(bitquant PUBLIC binapi)
endif()

# Tests
if(BUILD_TESTS)
    enable_testing()

    # Integration tests (standalone, no GTest dependency)
    add_executable(bitquant_integration_tests
        tests/integration/test_integration.cpp
    )
    target_link_libraries(bitquant_integration_tests PRIVATE bitquant)
    add_test(NAME integration_tests COMMAND bitquant_integration_tests)

    # Standalone unit tests (no GTest dependency)
    add_executable(test_offset_converter
        tests/unit/test_offset_converter_standalone.cpp
    )
    target_link_libraries(test_offset_converter PRIVATE bitquant)
    add_test(NAME offset_converter_tests COMMAND test_offset_converter)

    add_executable(test_inverse_contract
        tests/unit/test_inverse_contract.cpp
    )
    target_link_libraries(test_inverse_contract PRIVATE bitquant)
    add_test(NAME inverse_contract_tests COMMAND test_inverse_contract)

    add_executable(test_sqlite_database
        tests/unit/test_sqlite_database.cpp
    )
    target_link_libraries(test_sqlite_database PRIVATE bitquant)
    add_test(NAME sqlite_database_tests COMMAND test_sqlite_database)

    # BinanceSpotGateway tests
    add_executable(test_binance_spot_gateway
        tests/unit/test_binance_spot_gateway.cpp
    )
    target_link_libraries(test_binance_spot_gateway PRIVATE bitquant)
    add_test(NAME binance_spot_gateway_tests COMMAND test_binance_spot_gateway)

    # MainEngine tests
    add_executable(test_main_engine
        tests/unit/test_main_engine.cpp
    )
    target_link_libraries(test_main_engine PRIVATE bitquant)
    add_test(NAME main_engine_tests COMMAND test_main_engine)

    # StrategyManager tests
    add_executable(test_strategy_manager
        tests/unit/test_strategy_manager.cpp
    )
    target_link_libraries(test_strategy_manager PRIVATE bitquant)
    add_test(NAME strategy_manager_tests COMMAND test_strategy_manager)

    # WebSocketClient tests
    add_executable(test_websocket_client
        tests/unit/test_websocket_client.cpp
    )
    target_link_libraries(test_websocket_client PRIVATE bitquant)
    add_test(NAME websocket_client_tests COMMAND test_websocket_client)

    # PaperBroker tests
    add_executable(test_paper_broker
        tests/unit/test_paper_broker.cpp
    )
    target_link_libraries(test_paper_broker PRIVATE bitquant)
    add_test(NAME paper_broker_tests COMMAND test_paper_broker)

    # Branch coverage tests
    add_executable(test_branch_coverage
        tests/integration/test_branch_coverage.cpp
    )
    target_link_libraries(test_branch_coverage PRIVATE bitquant)
    add_test(NAME branch_coverage_tests COMMAND test_branch_coverage)

    # BinanceSpotRestApi tests
    add_executable(test_binance_spot_rest_api
        tests/unit/test_binance_spot_rest_api.cpp
    )
    target_link_libraries(test_binance_spot_rest_api PRIVATE bitquant)
    add_test(NAME binance_spot_rest_api_tests COMMAND test_binance_spot_rest_api)

    # BinanceSpotWsApi tests
    add_executable(test_binance_spot_ws_api
        tests/unit/test_binance_spot_ws_api.cpp
    )
    target_link_libraries(test_binance_spot_ws_api PRIVATE bitquant)
    add_test(NAME binance_spot_ws_api_tests COMMAND test_binance_spot_ws_api)

    # Strategy tests
    add_executable(test_strategy
        tests/unit/test_strategy.cpp
    )
    target_link_libraries(test_strategy PRIVATE bitquant)
    add_test(NAME strategy_tests COMMAND test_strategy)

    # BarGenerator tests
    add_executable(test_bar_generator
        tests/unit/test_bar_generator.cpp
    )
    target_link_libraries(test_bar_generator PRIVATE bitquant)
    add_test(NAME bar_generator_tests COMMAND test_bar_generator)

    # Types tests
    add_executable(test_types
        tests/unit/test_types.cpp
    )
    target_link_libraries(test_types PRIVATE bitquant)
    add_test(NAME types_tests COMMAND test_types)

    # StrategyContext tests
    add_executable(test_strategy_context
        tests/unit/test_strategy_context.cpp
    )
    target_link_libraries(test_strategy_context PRIVATE bitquant)
    add_test(NAME strategy_context_tests COMMAND test_strategy_context)

    # DateTime tests
    add_executable(test_datetime
        tests/unit/test_datetime.cpp
    )
    target_link_libraries(test_datetime PRIVATE bitquant)
    add_test(NAME datetime_tests COMMAND test_datetime)

    # Logger tests
    add_executable(test_logger
        tests/unit/test_logger.cpp
    )
    target_link_libraries(test_logger PRIVATE bitquant)
    add_test(NAME logger_tests COMMAND test_logger)

    # ThreadPool tests
    add_executable(test_thread_pool
        tests/unit/test_thread_pool.cpp
    )
    target_link_libraries(test_thread_pool PRIVATE bitquant)
    add_test(NAME thread_pool_tests COMMAND test_thread_pool)

    # Config tests
    add_executable(test_config
        tests/unit/test_config.cpp
    )
    target_link_libraries(test_config PRIVATE bitquant)
    add_test(NAME config_tests COMMAND test_config)

    # ExchangeFactory tests
    add_executable(test_exchange_factory
        tests/unit/test_exchange_factory.cpp
    )
    target_link_libraries(test_exchange_factory PRIVATE bitquant)
    add_test(NAME exchange_factory_tests COMMAND test_exchange_factory)

    # BinanceGateway tests
    add_executable(test_binance_gateway
        tests/unit/test_binance_gateway.cpp
    )
    target_link_libraries(test_binance_gateway PRIVATE bitquant)
    add_test(NAME binance_gateway_tests COMMAND test_binance_gateway)

    # BinanceUsdtGateway tests
    add_executable(test_binance_usdt_gateway
        tests/unit/test_binance_usdt_gateway.cpp
    )
    target_link_libraries(test_binance_usdt_gateway PRIVATE bitquant)
    add_test(NAME binance_usdt_gateway_tests COMMAND test_binance_usdt_gateway)

    # BinanceUsdtRestApi tests
    add_executable(test_binance_usdt_rest_api
        tests/unit/test_binance_usdt_rest_api.cpp
    )
    target_link_libraries(test_binance_usdt_rest_api PRIVATE bitquant)
    add_test(NAME binance_usdt_rest_api_tests COMMAND test_binance_usdt_rest_api)

    # BinanceUsdtWsApi tests
    add_executable(test_binance_usdt_ws_api
        tests/unit/test_binance_usdt_ws_api.cpp
    )
    target_link_libraries(test_binance_usdt_ws_api PRIVATE bitquant)
    add_test(NAME binance_usdt_ws_api_tests COMMAND test_binance_usdt_ws_api)

    # BinanceApiClient tests
    add_executable(test_binance_api
        tests/unit/test_binance_api.cpp
    )
    target_link_libraries(test_binance_api PRIVATE bitquant)
    add_test(NAME binance_api_tests COMMAND test_binance_api)

    # ValidationStrategy tests
    add_executable(test_validation_strategy
        tests/unit/test_validation_strategy.cpp
    )
    target_link_libraries(test_validation_strategy PRIVATE bitquant)
    add_test(NAME validation_strategy_tests COMMAND test_validation_strategy)

    # GridMartinStrategy tests
    add_executable(test_grid_martin
        tests/unit/test_grid_martin.cpp
    )
    target_link_libraries(test_grid_martin PRIVATE bitquant)
    add_test(NAME grid_martin_tests COMMAND test_grid_martin)

    # PriceSmoother tests
    add_executable(test_price_smoother
        tests/unit/test_price_smoother.cpp
    )
    target_link_libraries(test_price_smoother PRIVATE bitquant pthread)
    add_test(NAME price_smoother_tests COMMAND test_price_smoother)

    # MarketStateAnalyzer tests
    add_executable(test_market_state_analyzer
        tests/unit/test_market_state_analyzer.cpp
    )
    target_link_libraries(test_market_state_analyzer PRIVATE bitquant pthread)
    add_test(NAME market_state_analyzer_tests COMMAND test_market_state_analyzer)

    find_package(GTest QUIET)
    if(GTest_FOUND)
        add_executable(bitquant_tests
            tests/unit/test_ring_buffer.cpp
            tests/unit/test_array_manager.cpp
            tests/unit/test_broker.cpp
            tests/unit/test_binance_mapping.cpp
            tests/unit/test_offset_converter.cpp
        )
        target_link_libraries(bitquant_tests PRIVATE bitquant GTest::gtest GTest::gtest_main)
        include(GoogleTest)
        gtest_discover_tests(bitquant_tests)
    else()
        message(STATUS "GTest not found, unit tests will not be built (integration tests will be built)")
    endif()
endif()

# Examples
if(BUILD_EXAMPLES)
    add_executable(demo_strategy examples/demo_strategy.cpp)
    target_link_libraries(demo_strategy PRIVATE bitquant)

    add_executable(demo_bar_generator examples/demo_bar_generator.cpp)
    target_link_libraries(demo_bar_generator PRIVATE bitquant)

    add_executable(demo_database examples/demo_database.cpp)
    target_link_libraries(demo_database PRIVATE bitquant)

    if(ENABLE_BINANCE_API)
        add_executable(demo_binance examples/demo_binance.cpp)
        target_link_libraries(demo_binance PRIVATE bitquant)

        add_executable(demo_binance_spot examples/demo_binance_spot.cpp)
        target_link_libraries(demo_binance_spot PRIVATE bitquant)

        add_executable(demo_exchange examples/demo_exchange.cpp)
        target_link_libraries(demo_exchange PRIVATE bitquant)

        add_executable(demo_multi_strategy examples/demo_multi_strategy.cpp)
        target_link_libraries(demo_multi_strategy PRIVATE bitquant)

        add_executable(demo_backtest_strategy examples/demo_backtest_strategy.cpp)
        target_link_libraries(demo_backtest_strategy PRIVATE bitquant)

        add_executable(demo_main_engine examples/demo_main_engine.cpp)
        target_link_libraries(demo_main_engine PRIVATE bitquant)

        add_executable(demo_paper_trading examples/demo_paper_trading.cpp)
        target_link_libraries(demo_paper_trading PRIVATE bitquant)

        add_executable(test_websocket_real examples/test_websocket_real.cpp)
        target_link_libraries(test_websocket_real PRIVATE bitquant)

        add_executable(demo_paper_trading_full examples/demo_paper_trading_full.cpp)
        target_link_libraries(demo_paper_trading_full PRIVATE bitquant)

        add_executable(demo_grid_martin examples/demo_grid_martin.cpp)
        target_link_libraries(demo_grid_martin PRIVATE bitquant)

        add_executable(demo_grid_martin_paper examples/demo_grid_martin_paper.cpp)
        target_link_libraries(demo_grid_martin_paper PRIVATE bitquant)
    endif()
endif()

# Print configuration
message(STATUS "=== BitQuant C++ Configuration ===")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build tests: ${BUILD_TESTS}")
message(STATUS "Build examples: ${BUILD_EXAMPLES}")
message(STATUS "=================================")
