/**
 * @file datetime.cpp
 * @brief DateTime utilities implementation
 */

#include "utils/datetime.hpp"
#include <sstream>
#include <iomanip>

namespace bitquant {

std::string timestamp_to_string(int64_t ms) {
    auto time = std::chrono::system_clock::time_point(
        std::chrono::milliseconds(ms)
    );
    auto time_t = std::chrono::system_clock::to_time_t(time);
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

int64_t parse_datetime(const std::string& datetime_str) {
    std::tm tm = {};
    std::istringstream iss(datetime_str);
    iss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    auto time_point = std::chrono::system_clock::from_time_t(std::mktime(&tm));
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        time_point.time_since_epoch()
    ).count();
}

int64_t current_timestamp_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

} // namespace bitquant
