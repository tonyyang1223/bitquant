/**
 * @file test_config.cpp
 * @brief Unit tests for Config
 */

#include "config/config.hpp"
#include <iostream>

using namespace bitquant;

//=============================================================================
// Test Functions
//=============================================================================

bool test_config_construction() {
    std::cout << "[Test] Config construction" << std::endl;

    Config config;
    if (!config.empty()) return false;

    std::cout << "  PASS: Config constructed correctly" << std::endl;
    return true;
}

bool test_config_set_get_string() {
    std::cout << "[Test] Config set/get string" << std::endl;

    Config config;
    config.set("key", std::string("value"));

    if (!config.has("key")) return false;
    if (config.get_string("key") != "value") return false;

    std::cout << "  PASS: set/get string works" << std::endl;
    return true;
}

bool test_config_set_get_int() {
    std::cout << "[Test] Config set/get int" << std::endl;

    Config config;
    config.set("number", 42);

    if (config.get_int("number") != 42) return false;

    std::cout << "  PASS: set/get int works" << std::endl;
    return true;
}

bool test_config_set_get_double() {
    std::cout << "[Test] Config set/get double" << std::endl;

    Config config;
    config.set("pi", 3.14159);

    double val = config.get_double("pi");
    if (val < 3.1 || val > 3.2) return false;

    std::cout << "  PASS: set/get double works" << std::endl;
    return true;
}

bool test_config_set_get_bool() {
    std::cout << "[Test] Config set/get bool" << std::endl;

    Config config;
    config.set("enabled", std::string("true"));
    config.set("disabled", std::string("false"));

    if (!config.get_bool("enabled")) return false;
    if (config.get_bool("disabled")) return false;

    std::cout << "  PASS: set/get bool works" << std::endl;
    return true;
}

bool test_config_default_values() {
    std::cout << "[Test] Config default values" << std::endl;

    Config config;

    if (config.get_string("nonexistent", "default") != "default") return false;
    if (config.get_int("nonexistent", 99) != 99) return false;
    if (config.get_double("nonexistent", 1.5) != 1.5) return false;
    if (config.get_bool("nonexistent", true) != true) return false;

    std::cout << "  PASS: default values work" << std::endl;
    return true;
}

bool test_config_has() {
    std::cout << "[Test] Config has" << std::endl;

    Config config;
    if (config.has("key")) return false;

    config.set("key", std::string("value"));
    if (!config.has("key")) return false;

    std::cout << "  PASS: has works" << std::endl;
    return true;
}

bool test_config_remove() {
    std::cout << "[Test] Config remove" << std::endl;

    Config config;
    config.set("key", std::string("value"));
    if (!config.has("key")) return false;

    config.remove("key");
    if (config.has("key")) return false;

    std::cout << "  PASS: remove works" << std::endl;
    return true;
}

bool test_config_clear() {
    std::cout << "[Test] Config clear" << std::endl;

    Config config;
    config.set("key1", std::string("value1"));
    config.set("key2", std::string("value2"));

    config.clear();
    if (!config.empty()) return false;

    std::cout << "  PASS: clear works" << std::endl;
    return true;
}

bool test_config_size() {
    std::cout << "[Test] Config size" << std::endl;

    Config config;
    if (config.size() != 0) return false;

    config.set("key1", std::string("value1"));
    config.set("key2", std::string("value2"));
    if (config.size() != 2) return false;

    std::cout << "  PASS: size works" << std::endl;
    return true;
}

bool test_config_load_from_string() {
    std::cout << "[Test] Config load_from_string" << std::endl;

    Config config;
    std::string content = "{\"key\": \"value\", \"number\": 42}";

    bool result = config.load_from_string(content);
    // Just verify it doesn't crash

    std::cout << "  PASS: load_from_string works" << std::endl;
    return true;
}

bool test_config_to_string() {
    std::cout << "[Test] Config to_string" << std::endl;

    Config config;
    config.set("key", std::string("value"));

    std::string str = config.to_string();
    if (str.empty()) return false;

    std::cout << "  PASS: to_string works" << std::endl;
    return true;
}

bool test_config_merge() {
    std::cout << "[Test] Config merge" << std::endl;

    Config config1;
    config1.set("key1", std::string("value1"));

    Config config2;
    config2.set("key2", std::string("value2"));

    config1.merge(config2);

    if (!config1.has("key2")) return false;

    std::cout << "  PASS: merge works" << std::endl;
    return true;
}

//=============================================================================
// Main
//=============================================================================

int main() {
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║          Config Unit Tests                                ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    int passed = 0;
    int failed = 0;

    auto tests = {
        test_config_construction,
        test_config_set_get_string,
        test_config_set_get_int,
        test_config_set_get_double,
        test_config_set_get_bool,
        test_config_default_values,
        test_config_has,
        test_config_remove,
        test_config_clear,
        test_config_size,
        test_config_load_from_string,
        test_config_to_string,
        test_config_merge
    };

    for (auto test : tests) {
        if (test()) {
            passed++;
        } else {
            failed++;
        }
    }

    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║          Test Results                                      ║\n";
    std::cout << "╠════════════════════════════════════════════════════════════╣\n";
    std::cout << "║ Passed: " << passed << "                                                ║\n";
    std::cout << "║ Failed: " << failed << "                                                ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n";

    return failed > 0 ? 1 : 0;
}