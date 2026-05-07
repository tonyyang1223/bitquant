/**
 * @file binance_mapping.hpp
 * @brief Binance enum mappings for converting between internal types and Binance API strings
 *
 * Based on howtrader design patterns from https://github.com/51bitquant/howtrader
 */

#pragma once

#include "core/types.hpp"
#include <string>
#include <unordered_map>

namespace bitquant {

//=============================================================================
// Binance <-> VT Mappings (from Python howtrader)
//=============================================================================

/**
 * @brief Convert Binance status string to Status enum
 */
Status status_from_binance(const std::string& status);

/**
 * @brief Convert Status enum to Binance status string
 */
std::string status_to_binance(Status status);

/**
 * @brief Convert OrderType enum to Binance order type string
 */
std::string order_type_to_binance(OrderType type);

/**
 * @brief Convert Binance order type string to OrderType enum
 */
OrderType order_type_from_binance(const std::string& type);

/**
 * @brief Convert Direction enum to Binance side string
 */
std::string direction_to_binance(Direction dir);

/**
 * @brief Convert Binance side string to Direction enum
 */
Direction direction_from_binance(const std::string& side);

/**
 * @brief Convert Interval enum to Binance interval string
 */
std::string interval_to_binance(Interval interval);

/**
 * @brief Convert Binance interval string to Interval enum
 */
Interval interval_from_binance(const std::string& interval);

} // namespace bitquant
