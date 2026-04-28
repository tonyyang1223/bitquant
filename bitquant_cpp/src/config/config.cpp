/**
 * @file config.cpp
 * @brief Configuration implementation (PIMPL pattern)
 */

#include "config/config.hpp"
#include <unordered_map>

namespace bitquant {

/**
 * @brief Implementation details
 */
class ConfigImpl {
public:
    std::unordered_map<std::string, ConfigValue> values;
    std::unordered_map<std::string, Config> sections;
};

Config::Config() : impl_(std::make_unique<ConfigImpl>()) {}
Config::~Config() = default;

Config::Config(const std::string& filename) : impl_(std::make_unique<ConfigImpl>()) {
    load(filename);
}

Config::Config(const Config& other) : impl_(std::make_unique<ConfigImpl>(*other.impl_)) {}

Config::Config(Config&& other) noexcept : impl_(std::move(other.impl_)) {}

Config& Config::operator=(const Config& other) {
    if (this != &other) {
        impl_ = std::make_unique<ConfigImpl>(*other.impl_);
    }
    return *this;
}

Config& Config::operator=(Config&& other) noexcept {
    if (this != &other) {
        impl_ = std::move(other.impl_);
    }
    return *this;
}

bool Config::load(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return false;

    std::stringstream buffer;
    buffer << file.rdbuf();
    return load_from_string(buffer.str());
}

bool Config::load_from_string(const std::string& content) {
    clear();

    size_t pos = 0;
    while (pos < content.size() && content[pos] != '{') pos++;
    if (pos >= content.size()) return false;
    pos++;

    while (pos < content.size()) {
        while (pos < content.size() && std::isspace(content[pos])) pos++;
        if (pos >= content.size()) break;
        if (content[pos] == '}') { pos++; break; }

        // Parse key
        if (content[pos] != '"') return false;
        pos++;
        std::string key;
        while (pos < content.size() && content[pos] != '"') {
            key += content[pos++];
        }
        pos++;

        // Skip to value
        while (pos < content.size() && (std::isspace(content[pos]) || content[pos] == ':')) pos++;

        // Parse value
        if (content[pos] == '"') {
            pos++;
            std::string value;
            while (pos < content.size() && content[pos] != '"') {
                if (content[pos] == '\\' && pos + 1 < content.size()) {
                    pos++;
                    switch (content[pos]) {
                        case 'n': value += '\n'; break;
                        case 't': value += '\t'; break;
                        default: value += content[pos]; break;
                    }
                } else {
                    value += content[pos];
                }
                pos++;
            }
            pos++;
            impl_->values[key] = ConfigValue(value);

        } else if (content[pos] == '{') {
            size_t start = pos;
            int brace_count = 1;
            pos++;
            while (pos < content.size() && brace_count > 0) {
                if (content[pos] == '{') brace_count++;
                else if (content[pos] == '}') brace_count--;
                pos++;
            }
            Config section;
            section.load_from_string(content.substr(start, pos - start));
            impl_->sections[key] = section;

        } else if (std::isdigit(content[pos]) || content[pos] == '-') {
            std::string num;
            if (content[pos] == '-') num += content[pos++];
            while (pos < content.size() && (std::isdigit(content[pos]) || content[pos] == '.')) {
                num += content[pos++];
            }
            if (num.find('.') != std::string::npos) {
                impl_->values[key] = ConfigValue(std::stod(num));
            } else {
                impl_->values[key] = ConfigValue(std::stoi(num));
            }

        } else if (content.substr(pos, 4) == "true") {
            impl_->values[key] = ConfigValue(true);
            pos += 4;
        } else if (content.substr(pos, 5) == "false") {
            impl_->values[key] = ConfigValue(false);
            pos += 5;
        }

        while (pos < content.size() && (std::isspace(content[pos]) || content[pos] == ',')) pos++;
    }

    return true;
}

bool Config::save(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) return false;
    file << to_string();
    return true;
}

std::string Config::to_string() const {
    std::ostringstream oss;
    oss << "{\n";

    bool first = true;
    for (const auto& [key, value] : impl_->values) {
        if (!first) oss << ",\n";
        first = false;
        oss << "  \"" << key << "\": \"" << value.as_string() << "\"";
    }

    for (const auto& [key, section] : impl_->sections) {
        if (!first) oss << ",\n";
        first = false;
        oss << "  \"" << key << "\": " << section.to_string();
    }

    oss << "\n}\n";
    return oss.str();
}

static std::pair<std::string, std::string> split_key(const std::string& key) {
    size_t dot_pos = key.find('.');
    if (dot_pos == std::string::npos) return {"", key};
    return {key.substr(0, dot_pos), key.substr(dot_pos + 1)};
}

std::string Config::get_string(const std::string& key, const std::string& default_val) const {
    auto parts = split_key(key);
    if (parts.first.empty()) {
        auto it = impl_->values.find(parts.second);
        return it != impl_->values.end() ? it->second.as_string(default_val) : default_val;
    }
    auto it = impl_->sections.find(parts.first);
    if (it != impl_->sections.end()) {
        return it->second.get_string(parts.second, default_val);
    }
    return default_val;
}

int Config::get_int(const std::string& key, int default_val) const {
    auto parts = split_key(key);
    if (parts.first.empty()) {
        auto it = impl_->values.find(parts.second);
        return it != impl_->values.end() ? it->second.as_int(default_val) : default_val;
    }
    auto it = impl_->sections.find(parts.first);
    if (it != impl_->sections.end()) {
        return it->second.get_int(parts.second, default_val);
    }
    return default_val;
}

double Config::get_double(const std::string& key, double default_val) const {
    auto parts = split_key(key);
    if (parts.first.empty()) {
        auto it = impl_->values.find(parts.second);
        return it != impl_->values.end() ? it->second.as_double(default_val) : default_val;
    }
    auto it = impl_->sections.find(parts.first);
    if (it != impl_->sections.end()) {
        return it->second.get_double(parts.second, default_val);
    }
    return default_val;
}

bool Config::get_bool(const std::string& key, bool default_val) const {
    auto parts = split_key(key);
    if (parts.first.empty()) {
        auto it = impl_->values.find(parts.second);
        return it != impl_->values.end() ? it->second.as_bool(default_val) : default_val;
    }
    auto it = impl_->sections.find(parts.first);
    if (it != impl_->sections.end()) {
        return it->second.get_bool(parts.second, default_val);
    }
    return default_val;
}

void Config::set(const std::string& key, const std::string& value) {
    auto parts = split_key(key);
    if (parts.first.empty()) {
        impl_->values[parts.second] = ConfigValue(value);
    } else {
        auto it = impl_->sections.find(parts.first);
        if (it == impl_->sections.end()) {
            impl_->sections[parts.first] = Config();
            it = impl_->sections.find(parts.first);
        }
        it->second.set(parts.second, value);
    }
}

void Config::set(const std::string& key, int value) {
    set(key, std::to_string(value));
}

void Config::set(const std::string& key, double value) {
    set(key, std::to_string(value));
}

void Config::set(const std::string& key, bool value) {
    set(key, value ? "true" : "false");
}

Config Config::get_section(const std::string& key) const {
    auto it = impl_->sections.find(key);
    return it != impl_->sections.end() ? it->second : Config();
}

void Config::set_section(const std::string& key, const Config& section) {
    impl_->sections[key] = section;
}

bool Config::has(const std::string& key) const {
    auto parts = split_key(key);
    if (parts.first.empty()) {
        return impl_->values.find(parts.second) != impl_->values.end() ||
               impl_->sections.find(parts.second) != impl_->sections.end();
    }
    auto it = impl_->sections.find(parts.first);
    if (it != impl_->sections.end()) {
        return it->second.has(parts.second);
    }
    return false;
}

bool Config::empty() const {
    return impl_->values.empty() && impl_->sections.empty();
}

size_t Config::size() const {
    return impl_->values.size() + impl_->sections.size();
}

void Config::remove(const std::string& key) {
    auto parts = split_key(key);
    if (parts.first.empty()) {
        impl_->values.erase(parts.second);
        impl_->sections.erase(parts.second);
    } else {
        auto it = impl_->sections.find(parts.first);
        if (it != impl_->sections.end()) {
            it->second.remove(parts.second);
        }
    }
}

void Config::clear() {
    impl_->values.clear();
    impl_->sections.clear();
}

void Config::merge(const Config& other) {
    for (const auto& [key, value] : other.impl_->values) {
        impl_->values[key] = value;
    }
    for (const auto& [key, section] : other.impl_->sections) {
        auto it = impl_->sections.find(key);
        if (it == impl_->sections.end()) {
            impl_->sections[key] = section;
        } else {
            it->second.merge(section);
        }
    }
}

} // namespace bitquant