/**
 * @file datetime.hpp
 * @brief DateTime utilities
 */

#pragma once

#include <chrono>
#include <string>
#include <cstdint>

namespace bitquant {

/**
 * @brief Convert milliseconds since epoch to string
 */
std::string timestamp_to_string(int64_t ms);

/**
 * @brief Parse ISO datetime string to milliseconds
 */
int64_t parse_datetime(const std::string& datetime_str);

/**
 * @brief Get current timestamp in milliseconds
 */
int64_t current_timestamp_ms();

} // namespace bitquant
