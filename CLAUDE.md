# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a Python cryptocurrency quantitative trading codebase from 51bitquant (YouTube/Bilibili educational content). It contains backtesting frameworks, exchange API integrations, technical indicator implementations, and educational Python tutorials.

## Dependencies

- Python 3.x
- pandas, numpy
- TA-Lib (talib) - for technical indicators
- ccxt - cryptocurrency exchange unified API
- requests

Install TA-Lib: `pip install TA-Lib` (may require system library installation first)

## Running Code

Run Python scripts directly:
```bash
python backtest/demo_strategy.py
python binance_api/binance.py
python technical_indicators/technical_indicators.py
```

## Architecture

### Backtesting Framework (`backtest/backtester/`)

Core components for strategy backtesting:

- **BaseStrategy** (`strategy.py`): Abstract base class for trading strategies. Override `next_bar(bar)` to implement strategy logic. Provides `buy()`, `sell()`, `short()`, `cover()` methods.

- **Broker** (`broker.py`): Order matching engine. Handles commission, leverage, slippage. Methods: `set_strategy()`, `set_cash()`, `set_leverage()`, `set_backtest_data()`, `run()`, `optimize_strategy()`.

- **ArrayManager** (`array_manager.py`): Time series container for bar data with TA-Lib integration. Provides `sma()`, `rsi()`, `macd()`, `boll()`, `atr()`, `cci()`, `adx()`, `keltner()`, `donchian()` methods.

- **BarData** (`data.py`): K-line data model with fields: datetime, open_price, high_price, low_price, close_price, volume.

### Creating a Strategy

1. Inherit from `BaseStrategy`
2. Set `params` dict for strategy parameters
3. Initialize `ArrayManager` in `__init__`
4. Implement `next_bar(bar)` for signal generation
5. Use `self.buy()/sell()/short()/cover()` for orders

### Exchange Integrations

- `binance_api/`: Binance REST API for fetching kline data
- `ccxt_study/`: CCXT unified exchange API examples (Binance, Huobi, Bitmex, OKEx)
- `huobi_api/`: Huobi API integration
- `bybit/`: Bybit kline data crawler

### Technical Indicators (`technical_indicators/`)

Uses TA-Lib for calculations. Custom implementations include CMI (Choppiness Market Index), RSI, StochRSI.

## Data Format

Kline CSV columns: `open_time`, `open`, `high`, `low`, `close`, `volume`

Example data files:
- `backtest/bitmex_btc_usd_1min_data.csv`
- `binance_api/*.csv`
