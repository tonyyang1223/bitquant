# 模拟盘测试配置指南

## 1. 配置文件准备

### 1.1 创建配置文件 `config/paper_trading.json`

```json
{
    "trading": {
        "mode": "paper",
        "symbol": "BTCUSDT",
        "initial_capital": 100000.0,
        "commission_rate": 0.001,
        "slippage_rate": 0.0005
    },
    
    "exchange": {
        "name": "binance_spot",
        "api_key": "",
        "api_secret": "",
        "testnet": false,
        "proxy_host": "",
        "proxy_port": 0
    },
    
    "strategy": {
        "name": "DoubleMa",
        "fast_window": 10,
        "slow_window": 20,
        "trade_size": 0.01
    },
    
    "database": {
        "path": "data/trading.db"
    },
    
    "risk": {
        "enable": true,
        "order_flow_limit": 50,
        "order_size_limit": 1.0,
        "active_order_limit": 10
    },
    
    "websocket": {
        "reconnect_interval_ms": 1000,
        "max_reconnect_attempts": 10,
        "ping_interval_ms": 30000,
        "pong_timeout_ms": 10000
    }
}
```

### 1.2 策略配置文件 `config/strategy_params.json`

```json
{
    "DoubleMa": {
        "fast_window": 10,
        "slow_window": 20,
        "trade_size": 0.01
    },
    "Rsi": {
        "rsi_period": 14,
        "oversold_level": 30,
        "overbought_level": 70,
        "trade_size": 0.01
    },
    "Bollinger": {
        "period": 20,
        "dev": 2.0,
        "trade_size": 0.01
    }
}
```

---

## 2. 网络环境准备

### 2.1 检查网络连接

```bash
# 测试 Binance API 连接
curl -s https://api.binance.com/api/v3/ping

# 测试 WebSocket 连接
curl -s https://stream.binance.com:9443
```

### 2.2 代理配置（如果需要）

如果在中国大陆，需要配置代理：

```json
{
    "exchange": {
        "proxy_host": "127.0.0.1",
        "proxy_port": 7890
    }
}
```

---

## 3. API Key 配置（可选）

### 3.1 模拟盘测试无需 API Key

模拟盘模式只需要实时行情数据，不需要发送真实订单。

- **公共行情数据**: 无需 API Key
- **用户数据流**: 仅在需要账户信息时需要

### 3.2 如需测试账户功能

1. 注册 Binance 测试网账户: https://testnet.binance.vision/
2. 获取测试 API Key
3. 配置到 `config/paper_trading.json`

---

## 4. 数据库准备

### 4.1 创建数据目录

```bash
mkdir -p data logs
```

### 4.2 数据库初始化

首次运行时会自动创建数据库表：
- `dbbardata` - K线数据
- `dbtickdata` - Tick数据
- `dbstrategydata` - 策略状态

---

## 5. 依赖库安装

### 5.1 已安装的依赖

```
- TA-Lib      ✓ (技术指标库)
- SQLite3     ✓ (数据库)
- OpenSSL     ✓ (SSL/加密)
- Boost       ✓ (网络/API)
- zlib        ✓ (压缩)
```

### 5.2 检查依赖

```bash
# 检查 TA-Lib
ldconfig -p | grep ta_lib

# 检查 SQLite
ldconfig -p | grep sqlite3

# 检查 OpenSSL
ldconfig -p | grep ssl
```

---

## 6. 编译项目

### 6.1 编译命令

```bash
cd bitquant_cpp/build
cmake .. -DENABLE_BINANCE_API=ON -DBUILD_TESTS=ON
make -j$(nproc)
```

### 6.2 检查编译产物

```bash
ls -la demo_paper_trading demo_main_engine demo_database
```

---

## 7. 运行测试

### 7.1 数据库测试

```bash
./demo_database
```

### 7.2 模拟盘测试

```bash
# 当前实现状态：WebSocket 是模拟实现
./demo_paper_trading
```

---

## 8. 待完成工作

### 8.1 WebSocket 真实连接（P0 - 必须）

当前 `websocket_client.cpp` 的 `run_io_loop()` 是模拟实现。

**需要实现：**
- 使用 binapi 的真实 WebSocket 连接
- 或集成第三方 WebSocket 库（如 Boost.Beast）

**推荐方案：**
使用 binapi 的 WebSocket 实现：

```cpp
// 参考 binapi/src/websocket.cpp
// 需要调用 binapi::websockets::api 的真实连接方法
```

### 8.2 配置文件读取（P1）

需要实现 JSON 配置文件的完整解析：

```cpp
// 使用 flatjson.hpp 解析配置
Config config;
config.load("config/paper_trading.json");
```

### 8.3 Bar 数据生成（已完成 ✓）

`BarGenerator` 已实现，可从 Tick 数据生成 K线。

### 8.4 策略集成（已完成 ✓）

内置策略已实现，需要与 PaperBroker 集成。

---

## 9. 快速启动步骤

```bash
# 1. 进入 build 目录
cd /home/ubuntu/project/bitquant/bitquant_cpp/build

# 2. 检查网络连接
curl -s https://api.binance.com/api/v3/ping

# 3. 创建数据目录
mkdir -p data logs config

# 4. 运行数据库测试
./demo_database

# 5. 运行 MainEngine 测试
./demo_main_engine

# 6. 运行模拟盘（当前是模拟 WebSocket）
./demo_paper_trading
```

---

## 10. 模块完成状态

| 项目 | 状态 | 优先级 | 说明 |
|------|------|--------|------|
| WebSocket 真实连接 | ✅ 已完成 | P0 | 使用 binapi::ws::websockets 实现 |
| StrategyManager 订单路由 | ✅ 已完成 | P0 | 支持 PaperBroker 和 Broker |
| RiskManager 活跃订单 | ✅ 已完成 | P0 | ActiveOrderBook 实现 |
| PaperBroker 模拟撮合 | ✅ 已完成 | P0 | 完整的订单撮合逻辑 |
| 配置文件解析 | ⚠️ 部分 | P1 | 需要完整 JSON 配置支持 |
| 用户数据流 | ⚠️ 未实现 | P2 | ListenKey 管理 |
| 代理支持 | ⚠️ 未实现 | P2 | WebSocket 代理配置 |

---

## 11. WebSocket 实现说明

**已完成：WebSocket 真实连接**

`src/exchange/websocket_client.cpp` 已使用 `binapi::ws::websockets` 实现真实连接：

```cpp
// 使用 binapi WebSocket
impl_->websockets = std::make_unique<binapi::ws::websockets>(
    *impl_->io_context,
    impl_->config.host,    // "stream.binance.com"
    impl_->config.port,    // "443"
    [this](const char* channel, const char* data, std::size_t size) {
        this->on_websocket_message(channel, data, size);
        return true;
    },
    nullptr,
    impl_->config.stat_interval_sec
);
```

**支持的订阅类型：**
- `ticker` - 24小时行情
- `bookTicker` - 最优挂单
- `aggTrade` - 归集成交
- `trade` - 成交记录
- `kline_<interval>` - K线数据
- `miniTicker` - 迷你行情

**重连机制：**
- 指数退避重连 (1s → 30s)
- 最大重连次数可配置
- 订阅自动恢复

---

## 12. StrategyManager 订单路由

**已完成：策略订单发送到 Broker**

`src/engine/strategy_manager.cpp` 已实现完整的订单路由：

```cpp
std::string StrategyManager::send_order(...) {
    // 风控检查
    if (risk_manager_ && !risk_manager_->check_risk(req, "")) {
        return "";
    }

    // 发送到 PaperBroker (模拟盘) 或 Broker (回测)
    if (paper_broker_) {
        broker_orderid = paper_broker_->send_order(req);
    } else if (broker_) {
        id = broker_->buy(req.price, req.volume);
    }
}
```

---

## 13. 测试命令

**WebSocket 连接测试：**
```bash
./test_websocket_real
# 输出实时 BTCUSDT 价格和消息统计
```

**模拟盘完整测试：**
```bash
./demo_paper_trading_full
# 完整集成测试：WebSocket + PaperBroker + StrategyManager + RiskManager
```

**输出示例：**
```
[Live]  18s | Ticks:     34 | Bars:    0 | BTCUSDT: $81231.70 | Position: 0.0000 | Equity: $100000.00
```