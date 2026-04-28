/**
 * @file exchange_factory.cpp
 * @brief Exchange factory implementation
 */

#include "exchange/i_exchange.hpp"
#include <unordered_map>
#include <mutex>

namespace bitquant {

namespace {
    std::unordered_map<std::string, ExchangeFactory::Creator>& get_registry() {
        static std::unordered_map<std::string, ExchangeFactory::Creator> registry;
        return registry;
    }

    std::mutex& get_mutex() {
        static std::mutex mtx;
        return mtx;
    }
}

void ExchangeFactory::register_exchange(const std::string& name, Creator creator) {
    std::lock_guard<std::mutex> lock(get_mutex());
    get_registry()[name] = std::move(creator);
}

std::unique_ptr<IExchange> ExchangeFactory::create(const std::string& name) {
    std::lock_guard<std::mutex> lock(get_mutex());
    auto& registry = get_registry();
    auto it = registry.find(name);
    if (it != registry.end()) {
        return it->second();
    }
    return nullptr;
}

std::vector<std::string> ExchangeFactory::list_exchanges() {
    std::lock_guard<std::mutex> lock(get_mutex());
    std::vector<std::string> names;
    for (const auto& [name, _] : get_registry()) {
        names.push_back(name);
    }
    return names;
}

bool ExchangeFactory::has_exchange(const std::string& name) {
    std::lock_guard<std::mutex> lock(get_mutex());
    return get_registry().count(name) > 0;
}

} // namespace bitquant
