/**
 * @file inverse_contract.hpp
 * @brief Inverse contract (coin-margined futures) support
 *
 * Binance offers two types of perpetual futures:
 * 1. USDT-margined (linear): PnL = volume * (exit_price - entry_price)
 * 2. Coin-margined (inverse): PnL = volume * contract_size * (1/entry - 1/exit)
 *
 * Reference: howtrader/gateway/binance/binance_usdt_gateway.py
 */

#pragma once

#include "core/types.hpp"
#include <string>
#include <cmath>

namespace bitquant {

//=============================================================================
// Inverse Contract Endpoints
//=============================================================================

// Coin-margined (inverse) futures endpoints
constexpr const char* DAPI_REST_HOST = "https://dapi.binance.com";
constexpr const char* DAPI_WS_HOST = "dstream.binance.com";
constexpr const char* DAPI_WS_PATH = "/stream";

//=============================================================================
// PnL Calculation Functions
//=============================================================================

/**
 * @brief Calculate unrealized PnL for futures position
 *
 * For linear (USDT-margined) contracts:
 *   PnL = volume * (exit_price - entry_price)
 *
 * For inverse (coin-margined) contracts:
 *   PnL = volume * contract_size * (1/entry_price - 1/exit_price)
 *
 * @param inverse Whether contract is inverse (coin-margined)
 * @param entry_price Entry price
 * @param exit_price Current/exit price
 * @param volume Position volume (in contracts)
 * @param contract_size Contract size (for inverse contracts)
 * @param direction Position direction (LONG or SHORT)
 * @return Unrealized PnL in settlement currency
 */
inline double calculate_pnl(
    bool inverse,
    double entry_price,
    double exit_price,
    double volume,
    double contract_size,
    Direction direction
) {
    if (inverse) {
        // Coin-margined: PnL in coin (BTC, ETH, etc.)
        // For LONG: profit when price rises (1/entry - 1/exit is negative, but we want positive)
        // For SHORT: profit when price falls
        double pnl_per_contract = contract_size * (1.0 / entry_price - 1.0 / exit_price);
        if (direction == Direction::LONG) {
            return volume * pnl_per_contract;
        } else {
            return -volume * pnl_per_contract;
        }
    } else {
        // USDT-margined: PnL in USDT
        if (direction == Direction::LONG) {
            return volume * (exit_price - entry_price);
        } else {
            return volume * (entry_price - exit_price);
        }
    }
}

/**
 * @brief Calculate position value in settlement currency
 *
 * For linear: value = volume * price
 * For inverse: value = volume * contract_size / price
 *
 * @param inverse Whether contract is inverse
 * @param price Current price
 * @param volume Position volume
 * @param contract_size Contract size
 * @return Position value in settlement currency
 */
inline double calculate_position_value(
    bool inverse,
    double price,
    double volume,
    double contract_size
) {
    if (inverse) {
        return volume * contract_size / price;
    } else {
        return volume * price;
    }
}

/**
 * @brief Calculate required margin for position
 *
 * @param inverse Whether contract is inverse
 * @param price Entry price
 * @param volume Position volume
 * @param contract_size Contract size
 * @param leverage Leverage multiplier
 * @return Required margin in settlement currency
 */
inline double calculate_margin(
    bool inverse,
    double price,
    double volume,
    double contract_size,
    int leverage
) {
    double position_value = calculate_position_value(inverse, price, volume, contract_size);
    return position_value / leverage;
}

/**
 * @brief Calculate ROE (Return on Equity) percentage
 *
 * @param pnl Unrealized PnL
 * @param margin Initial margin
 * @return ROE percentage
 */
inline double calculate_roe(double pnl, double margin) {
    if (margin <= 0) return 0.0;
    return (pnl / margin) * 100.0;
}

//=============================================================================
// Contract Type Detection
//=============================================================================

/**
 * @brief Check if symbol is an inverse contract
 *
 * Inverse contract symbols typically end with:
 * - _PERP (e.g., BTCUSD_PERP)
 * - USD (e.g., BTCUSD) for coin-margined
 *
 * @param symbol Symbol string
 * @return true if inverse contract
 */
inline bool is_inverse_contract(const std::string& symbol) {
    // Binance coin-margined futures use USD as quote
    // Examples: BTCUSD_PERP, ETHUSD_PERP
    if (symbol.find("USD_PERP") != std::string::npos) {
        return true;
    }
    // Also check for USD_ patterns (quarterly futures)
    if (symbol.find("USD_") != std::string::npos) {
        return true;
    }
    return false;
}

/**
 * @brief Get settlement currency from symbol
 *
 * @param symbol Symbol string
 * @param inverse Whether contract is inverse
 * @return Settlement currency (e.g., "BTC" for BTCUSD_PERP, "USDT" for BTCUSDT)
 */
inline std::string get_settlement_currency(const std::string& symbol, bool inverse) {
    if (inverse) {
        // For BTCUSD_PERP, settlement is BTC
        size_t usd_pos = symbol.find("USD");
        if (usd_pos != std::string::npos && usd_pos > 0) {
            return symbol.substr(0, usd_pos);
        }
    } else {
        // For BTCUSDT, settlement is USDT
        size_t usdt_pos = symbol.find("USDT");
        if (usdt_pos != std::string::npos && usdt_pos > 0) {
            return "USDT";
        }
        size_t busd_pos = symbol.find("BUSD");
        if (busd_pos != std::string::npos && busd_pos > 0) {
            return "BUSD";
        }
    }
    return "USDT";  // Default
}

//=============================================================================
// Contract Size Constants
//=============================================================================

/**
 * @brief Get contract size for symbol
 *
 * Contract sizes vary by exchange and symbol:
 * - BTCUSD_PERP: 100 USD per contract
 * - ETHUSD_PERP: 10 USD per contract
 *
 * @param symbol Symbol string
 * @return Contract size
 */
inline double get_contract_size(const std::string& symbol) {
    // Binance coin-margined contract sizes
    // Note: Must check for USD_PERP or USD_ pattern to avoid matching USDT symbols
    if (symbol.find("BTCUSD_PERP") != std::string::npos ||
        symbol.find("BTCUSD_") != std::string::npos) {
        return 100.0;  // 100 USD per BTC contract
    }
    if (symbol.find("ETHUSD_PERP") != std::string::npos ||
        symbol.find("ETHUSD_") != std::string::npos) {
        return 10.0;   // 10 USD per ETH contract
    }
    if (symbol.find("BNBUSD_PERP") != std::string::npos ||
        symbol.find("BNBUSD_") != std::string::npos) {
        return 10.0;   // 10 USD per BNB contract
    }
    // Default for USDT-margined (contract size = 1)
    return 1.0;
}

} // namespace bitquant