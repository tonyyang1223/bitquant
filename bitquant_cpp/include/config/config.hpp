/**
 * @file config.hpp
 * @brief Configuration management system
 */

#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <optional>
#include <fstream>
#include <sstream>
#include <memory>

namespace bitquant {

/**
 * @brief Configuration value wrapper
 */
class ConfigValue {
public:
    ConfigValue() : value_(), type_(Type::EMPTY) {}
    explicit ConfigValue(const std::string& v) : value_(v), type_(Type::STRING) {}
    explicit ConfigValue(int v) : value_(std::to_string(v)), type_(Type::INT) {}
    explicit ConfigValue(double v) : value_(std::to_string(v)), type_(Type::DOUBLE) {}
    explicit ConfigValue(bool v) : value_(v ? "true" : "false"), type_(Type::BOOL) {}

    std::string as_string(const std::string& default_val = "") const {
        return type_ == Type::EMPTY ? default_val : value_;
    }

    int as_int(int default_val = 0) const {
        if (type_ == Type::EMPTY) return default_val;
        try { return std::stoi(value_); }
        catch (...) { return default_val; }
    }

    double as_double(double default_val = 0.0) const {
        if (type_ == Type::EMPTY) return default_val;
        try { return std::stod(value_); }
        catch (...) { return default_val; }
    }

    bool as_bool(bool default_val = false) const {
        if (type_ == Type::EMPTY) return default_val;
        return value_ == "true" || value_ == "1" || value_ == "yes";
    }

    bool is_empty() const { return type_ == Type::EMPTY; }

private:
    enum class Type { EMPTY, STRING, INT, DOUBLE, BOOL };
    std::string value_;
    Type type_;
};

// Forward declaration
class ConfigImpl;

/**
 * @brief Configuration class
 */
class Config {
public:
    Config();
    ~Config();
    explicit Config(const std::string& filename);

    // Copy and move
    Config(const Config& other);
    Config(Config&& other) noexcept;
    Config& operator=(const Config& other);
    Config& operator=(Config&& other) noexcept;

    // Load/Save
    bool load(const std::string& filename);
    bool save(const std::string& filename) const;
    bool load_from_string(const std::string& content);
    std::string to_string() const;

    // Value access with dot notation
    std::string get_string(const std::string& key, const std::string& default_val = "") const;
    int get_int(const std::string& key, int default_val = 0) const;
    double get_double(const std::string& key, double default_val = 0.0) const;
    bool get_bool(const std::string& key, bool default_val = false) const;

    // Set values
    void set(const std::string& key, const std::string& value);
    void set(const std::string& key, int value);
    void set(const std::string& key, double value);
    void set(const std::string& key, bool value);

    // Nested sections
    Config get_section(const std::string& key) const;
    void set_section(const std::string& key, const Config& section);

    // Check
    bool has(const std::string& key) const;
    bool empty() const;
    size_t size() const;

    // Remove
    void remove(const std::string& key);
    void clear();

    // Merge
    void merge(const Config& other);

private:
    std::unique_ptr<ConfigImpl> impl_;
};

} // namespace bitquant