# BitQuant C++ 高性能多策略交易平台

基于Python量化交易框架完整移植的高性能C++版本，融合[howtrader](https://github.com/51bitquant/howtrader)设计模式，支持多策略并行运行。

## 特性

- **高性能**: 100万+ bars/sec 回测吞吐量
- **多策略管理**: 策略独立运行，事件自动分发
- **统一交易所接口**: 支持Binance、Huobi等交易所
- **风险管理**: 订单流控、持仓限制、自成交预防
- **技术指标**: TA-Lib集成 (SMA/EMA/RSI/MACD/BOLL/ATR等)
- **事件驱动**: 异步事件处理，支持实时交易

## 架构

```
┌─────────────────────────────────────────────────────────────────┐
│                      TradingEngine (核心引擎)                    │
├─────────────────────────────────────────────────────────────────┤
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐          │
│  │ EventEngine  │  │StrategyManager│  │ RiskManager  │          │
│  │  事件分发    │  │  多策略管理   │  │  风险控制    │          │
│  └──────────────┘  └──────────────┘  └──────────────┘          │
│  ┌──────────────┐  ┌──────────────┐                            │
│  │    Broker    │  │   IExchange  │                            │
│  │  回测撮合    │  │  交易所接口  │                            │
│  └──────────────┘  └──────────────┘                            │
├─────────────────────────────────────────────────────────────────┤
│  Data Layer: ArrayManager / RingBuffer / DataFeed               │
│  Infrastructure: Logger / Config / ThreadPool                   │
└─────────────────────────────────────────────────────────────────┘
```

## 目录结构

```
bitquant_cpp/
├── include/                 # 头文件
│   ├── core/               # 核心类型定义
│   │   └── types.hpp       # BarData, TickData, OrderData等
│   ├── data/               # 数据管理
│   │   ├── array_manager.hpp   # 时间序列+技术指标
│   │   └── ring_buffer.hpp     # 环形缓冲区
│   ├── engine/             # 交易引擎
│   │   ├── broker.hpp          # 订单撮合引擎
│   │   ├── strategy.hpp        # 策略基类
│   │   ├── strategy_manager.hpp # 多策略管理
│   │   ├── trading_engine.hpp  # 核心协调器
│   │   ├── event.hpp           # 事件系统
│   │   └── risk_manager.hpp    # 风险管理
│   ├── exchange/           # 交易所接口
│   │   ├── i_exchange.hpp      # 统一接口
│   │   ├── binance_gateway.hpp # Binance适配器
│   │   └── exchange_factory.hpp # 工厂模式
│   ├── statistics/         # 统计分析
│   │   └── performance.hpp     # 绩效指标
│   └── utils/              # 工具类
│       ├── logger.hpp          # 日志系统
│       └── thread_pool.hpp     # 线程池
├── src/                    # 实现文件
├── examples/               # 示例代码
│   ├── demo_strategy.cpp       # 策略示例
│   └── demo_multi_strategy.cpp # 多策略示例
├── tests/                  # 测试代码
│   ├── unit/              # 单元测试
│   └── integration/       # 集成测试
└── CMakeLists.txt         # 构建配置
```

## 依赖

- **C++20** 编译器 (GCC 10+, Clang 12+)
- **CMake** 3.20+
- **TA-Lib** 技术指标库
- **Boost** (可选，binapi依赖)
- **OpenSSL** (可选，WebSocket依赖)

### 安装依赖

```bash
# Ubuntu/Debian
sudo apt-get install cmake libta-lib-dev libssl-dev

# macOS
brew install cmake ta-lib openssl
```

## 编译

```bash
cd bitquant_cpp
mkdir build && cd build

# 基础编译 (不含Binance API)
cmake .. -DENABLE_BINANCE_API=OFF
make -j$(nproc)

# 完整编译 (含Binance API，需要先安装binapi)
cmake ..
make -j$(nproc)
```

## 快速开始

### 创建策略

```cpp
#include "engine/strategy.hpp"
#include "data/array_manager.hpp"

class MyStrategy : public bitquant::IStrategy {
public:
    int fast_period = 10;
    int slow_period = 20;
    
    void on_init() override {
        am_ = bitquant::ArrayManager(100);
        inited_ = true;
    }
    
    void on_start() override {
        trading_ = true;
    }
    
    void on_bar(const bitquant::BarData& bar) override {
        am_.update_bar(bar);
        if (!am_.inited()) return;
        
        double fast_ma = am_.sma(fast_period);
        double slow_ma = am_.sma(slow_period);
        
        if (fast_ma > slow_ma && position() <= 0) {
            buy(bar.close_price, 1.0);
        } else if (fast_ma < slow_ma && position() > 0) {
            sell(bar.close_price, position());
        }
    }

private:
    bitquant::ArrayManager am_{100};
};
```

### 运行回测

```cpp
#include "engine/trading_engine.hpp"

int main() {
    // 创建引擎
    auto engine = bitquant::TradingEngineBuilder()
        .set_mode(bitquant::EngineMode::BACKTESTING)
        .set_capital(1'000'000.0)
        .set_commission(0.001)
        .build();
    
    // 加载数据
    engine.load_data_from_csv("BTCUSDT", "data.csv");
    
    // 添加策略
    auto strategy = std::make_unique<MyStrategy>();
    engine.add_strategy("SMA_Strategy", std::move(strategy), "BTCUSDT", {});
    
    // 运行回测
    engine.run_backtest();
    
    // 获取结果
    const auto& metrics = engine.get_performance();
    std::cout << metrics.to_string();
    
    return 0;
}
```

### 多策略运行

```cpp
// 添加多个策略
engine.add_strategy("SMA_Cross", std::make_unique<SMAStrategy>(), "BTCUSDT", {});
engine.add_strategy("RSI_Reversion", std::make_unique<RSIStrategy>(), "BTCUSDT", {});
engine.add_strategy("Bollinger", std::make_unique<BollingerStrategy>(), "BTCUSDT", {});

// 运行
engine.run_backtest();
```

## 运行测试

```bash
cd build
./bitquant_integration_tests
```

### 测试结果示例

```
╔════════════════════════════════════════════════════════════╗
║          BitQuant C++ Integration Tests                    ║
╚════════════════════════════════════════════════════════════╝

Testing: Broker basic functionality... PASSED
Testing: Performance calculation... PASSED
Testing: Multiple trades... PASSED (trades: 2)
Testing: TradingEngine backtest... PASSED
Testing: RiskManager order limit... PASSED
Testing: EventEngine... PASSED
Testing: ArrayManager indicators... PASSED

=== Performance Benchmark ===
Bars: 100000
Throughput: 1.13e+06 bars/sec
```

## 策略API

### 生命周期方法

| 方法 | 说明 |
|------|------|
| `on_init()` | 初始化，加载数据 |
| `on_start()` | 启动交易 |
| `on_stop()` | 停止交易 |
| `on_bar(bar)` | K线数据回调 |
| `on_tick(tick)` | Tick数据回调 |
| `on_order(order)` | 订单更新回调 |
| `on_trade(trade)` | 成交回调 |

### 交易方法

| 方法 | 说明 |
|------|------|
| `buy(price, volume)` | 买入开仓 |
| `sell(price, volume)` | 卖出平仓 |
| `short(price, volume)` | 做空开仓 |
| `cover(price, volume)` | 平空仓 |
| `cancel_order(orderid)` | 撤单 |
| `position()` | 当前持仓 |

### 技术指标 (ArrayManager)

| 方法 | 说明 |
|------|------|
| `sma(period)` | 简单移动平均 |
| `ema(period)` | 指数移动平均 |
| `rsi(period)` | 相对强弱指数 |
| `macd(fast, slow, signal)` | MACD指标 |
| `bollinger(period, std)` | 布林带 |
| `atr(period)` | 平均真实波幅 |
| `adx(period)` | 平均趋向指数 |

## 风险管理

```cpp
bitquant::RiskConfig config;
config.order_flow_limit = 50;      // 订单流限制
config.order_size_limit = 100.0;   // 单笔最大数量
config.trade_limit = 1000.0;       // 日交易限制
config.active_order_limit = 50;    // 活跃订单限制
config.prevent_self_trade = true;  // 自成交预防

engine.set_risk_manager(std::make_unique<bitquant::RiskManager>(config));
```

## 引擎模式

| 模式 | 说明 |
|------|------|
| `BACKTESTING` | 回测模式，使用历史数据 |
| `PAPER` | 模拟盘，实时数据模拟交易 |
| `LIVE` | 实盘，真实交易 |

## 性能对比

| 指标 | Python | C++ |
|------|--------|-----|
| 10万K线回测 | ~10秒 | ~100ms |
| 技术指标计算 | 较慢 | TA-Lib原生 |
| 内存占用 | 较高 | 优化 |

## 参考

- [howtrader](https://github.com/51bitquant/howtrader) - Python多策略框架
- [binapi](https://github.com/niXman/binapi) - Binance C++ API
- [TA-Lib](https://ta-lib.org/) - 技术分析库

## License

MIT License
