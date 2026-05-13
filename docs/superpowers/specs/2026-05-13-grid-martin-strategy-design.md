# Grid Martin Strategy Design

网格马丁策略设计文档 - 用于 C++ BitQuant 交易框架模拟盘运行。

## 1. Overview

### 1.1 Strategy Type

**Grid Martin Strategy (网格马丁策略)**

网格马丁策略是一种价格区间交易策略：
- 在基准价格上下设置多个价格网格
- 价格跌破网格线时买入固定金额仓位
- 价格涨破网格线时卖出对应仓位
- 实现震荡行情中的持续盈利

### 1.2 Requirements Summary

| Parameter | Value | Description |
|-----------|-------|-------------|
| 网格数量 | 10 | 基准价格上下各5格 |
| 网格间距 | 1% | 每格间距为1%百分比 |
| 单格仓位 | 固定金额 | 每格买入固定金额（如$100） |
| 基准价格 | 手动设定 | 策略启动时手动配置 |
| 止损机制 | 网格底线止损 | 跌破最低网格时全部清仓 |
| 初始仓位 | 空仓启动 | 策略启动时不持有仓位 |

---

## 2. Architecture

### 2.1 Class Structure

```
┌──────────────────────────────────────────────────────────────┐
│                    GridMartinStrategy                         │
│                    (继承 IStrategy)                           │
├──────────────────────────────────────────────────────────────┤
│  配置参数 (public):                                          │
│  - base_price_: double         基准价格                      │
│  - grid_count_: int            网格数量 (10)                  │
│  - grid_spacing_: double       网格间距百分比 (0.01)          │
│  - amount_per_grid_: double    每格固定金额                   │
│  - symbol_: string             交易品种                       │
├──────────────────────────────────────────────────────────────┤
│  内部状态 (private):                                         │
│  - grid_levels_: vector<double>     网格价格线                │
│  - grid_positions_: vector<double>  每格持仓数量              │
│  - grid_costs_: vector<double>      每格买入成本              │
│  - total_position_: double          总持仓数量                │
│  - avg_cost_: double                平均持仓成本              │
│  - last_grid_index_: int            上一次所在网格            │
│  - buy_queue_: queue<int>           买入顺序队列(FIFO)        │
├──────────────────────────────────────────────────────────────┤
│  核心方法:                                                    │
│  - on_init()           计算网格价格线                         │
│  - on_bar()            检测网格穿越，执行交易                  │
│  - on_order()          订单状态回调                           │
│  - calculate_grid_levels()  计算网格价格                      │
│  - get_grid_index()    判断价格所在网格                       │
│  - execute_grid_trade() 执行网格交易                         │
│  - check_stop_loss()   止损检查                              │
└──────────────────────────────────────────────────────────────┘
```

### 2.2 Grid Layout

以基准价 $100,000 为例，10格网格布局：

```
Grid Index | Price Level | Position | Direction
-----------|-------------|----------|------------
    0      |  $90,000    | 止损格   | 最低网格
    1      |  $91,000    | 买入区   | ↓ 跌破买入
    2      |  $92,000    | 买入区   | ↓ 跌破买入
    3      |  $93,000    | 买入区   | ↓ 跌破买入
    4      |  $94,000    | 买入区   | ↓ 跌破买入
    5      |  $95,000    | 买入区   | ↓ 跌破买入
    6      | $100,000    | 基准格   | 中间位置
    7      | $101,000    | 卖出区   | ↑ 涨破卖出
    8      | $102,000    | 卖出区   | ↑ 涨破卖出
    9      | $103,000    | 卖出区   | ↑ 涨破卖出
```

网格价格计算公式：
```
grid_levels_[i] = base_price * (1 - grid_spacing * (grid_count - i))
```

---

## 3. Trading Logic

### 3.1 Core Flow

```
on_bar(bar)
    │
    ├── 1. Get current price (bar.close_price)
    │
    ├── 2. Calculate current grid index
    │       current_grid = get_grid_index(price)
    │
    ├── 3. Check grid crossing
    │       if current_grid != last_grid:
    │           execute_grid_trade(last_grid, current_grid)
    │
    ├── 4. Stop loss check
    │       if price < grid_levels_[0]:
    │           check_stop_loss(price)
    │
    └── 5. Update state
            last_grid_index = current_grid
            write_log(...)
```

### 3.2 Grid Crossing Detection

价格从网格A移动到网格B：

**向下穿越（价格下跌）**：
- 从 Grid N 移动到 Grid M（M < N）
- 每穿越一格，买入固定金额仓位
- 记录买入数量和成本

**向上穿越（价格上涨）**：
- 从 Grid M 移动到 Grid N（N > M）
- 每穿越一格，卖出最早买入的持仓（FIFO）
- 计算盈亏

### 3.3 Trade Execution Example

```
初始状态: 空仓，基准价 $100,000

场景1: 价格从 $100,000 跌至 $99,000 (穿越 Grid 5)
  → 执行: buy($99,000, 100/$99,000 = 0.00101 BTC)
  → 状态: grid_positions_[5] = 0.00101
          grid_costs_[5] = $99,000
          total_position = 0.00101
          avg_cost = $99,000

场景2: 价格继续跌至 $97,000 (穿越 Grid 4, 3)
  → 执行 Grid 4: buy($97,500, 100/$97,500 = 0.00103 BTC)
  → 执行 Grid 3: buy($96,500, 100/$96,500 = 0.00104 BTC)
  → 状态: total_position = 0.00308
          avg_cost = ($99,000*0.00101 + $97,500*0.00103 + $96,500*0.00104) / 0.00308
                   ≈ $97,650

场景3: 价格涨回 $100,000 (穿越 Grid 5, 6)
  → 执行 Grid 5: sell($100,000, 0.00104 BTC)  // FIFO: 卖出最早买入的
  → 盈亏: ($100,000 - $96,500) * 0.00104 = $5.72
  → 执行 Grid 6: sell($100,000, 0.00103 BTC)
  → 盈亏: ($100,000 - $97,500) * 0.00103 = $2.58
  → 状态: total_position = 0.00101
          avg_cost = $99,000
```

---

## 4. Risk Management

### 4.1 Stop Loss Mechanism

网格底线止损：

```
止损价格 = grid_levels_[0]
         = base_price * (1 - grid_count * grid_spacing)
         = $100,000 * (1 - 10 * 0.01)
         = $90,000

触发条件: price < $90,000

执行动作:
  1. 全部清仓: sell(current_price, total_position)
  2. 暂停策略: trading_ = false
  3. 记录止损: write_log("[STOP LOSS] Triggered at $90,000")
  4. 计算亏损: (current_price - avg_cost) * total_position
```

### 4.2 Position Limits

- **最大持仓网格数**: grid_count_ (10格)
- **单格买入金额**: amount_per_grid_ ($100)
- **总投入上限**: grid_count_ * amount_per_grid_ ($1000)

### 4.3 Risk Parameters

```json
{
  "risk": {
    "stop_loss_percent": 10.0,
    "max_position_grids": 10,
    "daily_buy_limit": 20,
    "capital_reserve_ratio": 0.5
  }
}
```

---

## 5. File Structure

### 5.1 New Files

```
bitquant_cpp/
├── include/strategy/
│   └── grid_martin_strategy.hpp    # Strategy header
│
├── src/strategy/
│   └── grid_martin_strategy.cpp    # Strategy implementation
│
├── examples/
│   └── demo_grid_martin.cpp        # Demo program
│
└── tests/unit/
│   └── test_grid_martin.cpp        # Unit tests
```

### 5.2 Header File Structure

```cpp
// include/strategy/grid_martin_strategy.hpp

#pragma once

#include "engine/strategy.hpp"
#include <vector>
#include <queue>

namespace bitquant {

class GridMartinStrategy : public IStrategy {
public:
    // Configurable parameters
    double base_price_ = 0.0;
    int grid_count_ = 10;
    double grid_spacing_ = 0.01;
    double amount_per_grid_ = 100.0;
    std::string symbol_ = "BTCUSDT";

    // Constructor
    GridMartinStrategy();

    // Lifecycle callbacks
    void on_init() override;
    void on_start() override;
    void on_bar(const BarData& bar) override;
    void on_order(const Order& order) override;

    // Internal methods
    void calculate_grid_levels();
    int get_grid_index(double price) const;
    void execute_grid_trade(int from_grid, int to_grid, double price);
    void check_stop_loss(double price);

private:
    std::vector<double> grid_levels_;
    std::vector<double> grid_positions_;
    std::vector<double> grid_costs_;
    std::queue<int> buy_queue_;
    double total_position_ = 0.0;
    double avg_cost_ = 0.0;
    int last_grid_index_ = -1;
};

} // namespace bitquant
```

### 5.3 Demo Program

```cpp
// examples/demo_grid_martin.cpp

int main() {
    // Create paper trading engine
    PaperTradingEngine engine;
    engine.set_capital(10000.0);
    engine.set_commission(0.001);

    // Create strategy with parameters
    auto strategy = std::make_unique<GridMartinStrategy>();
    strategy->base_price_ = 85000.0;
    strategy->grid_count_ = 10;
    strategy->grid_spacing_ = 0.01;
    strategy->amount_per_grid_ = 100.0;

    // Add strategy
    engine.add_strategy("GridMartin", std::move(strategy), "BTCUSDT");

    // Start WebSocket
    engine.start_websocket("BTCUSDT", {"kline_1m"});

    // Run
    engine.run();

    return 0;
}
```

---

## 6. Testing Plan

### 6.1 Unit Tests

| Test Case | Description |
|-----------|-------------|
| CalculateGridLevels | Verify grid price calculation accuracy |
| GetGridIndex | Verify price-to-grid index mapping |
| BuyAmountCalculation | Verify fixed amount → quantity conversion |
| AvgCostCalculation | Verify average cost after multiple buys |
| FIFOOrder | Verify first-in-first-out selling order |
| StopLossTrigger | Verify stop loss execution |

### 6.2 Test Cases

```cpp
TEST(GridMartinStrategy, CalculateGridLevels) {
    GridMartinStrategy s;
    s.base_price_ = 100000.0;
    s.grid_count_ = 10;
    s.grid_spacing_ = 0.01;
    s.on_init();

    // Grid 0 (lowest): base_price * (1 - 10*0.01) = 90000
    EXPECT_NEAR(s.grid_levels_[0], 90000.0, 0.01);
    // Grid 9 (highest): base_price * (1 - 0*0.01) = 100000
    EXPECT_NEAR(s.grid_levels_[9], 100000.0, 0.01);
}

TEST(GridMartinStrategy, GridCrossingDown) {
    GridMartinStrategy s;
    s.base_price_ = 100000.0;
    s.grid_count_ = 10;
    s.grid_spacing_ = 0.01;
    s.amount_per_grid_ = 100.0;
    s.on_init();

    // Price drops from $100,000 to $99,000 (crossing Grid 5)
    BarData bar;
    bar.close_price = 99000.0;
    s.last_grid_index_ = 6;  // Was at base grid
    s.on_bar(bar);

    // Should have bought
    EXPECT_GT(s.total_position_, 0.0);
}

TEST(GridMartinStrategy, StopLossTrigger) {
    GridMartinStrategy s;
    s.base_price_ = 100000.0;
    s.grid_count_ = 10;
    s.grid_spacing_ = 0.01;
    s.amount_per_grid_ = 100.0;
    s.on_init();
    s.on_start();

    // Simulate having position
    s.total_position_ = 0.01;

    // Price drops below stop loss
    BarData bar;
    bar.close_price = 85000.0;  // Below $90,000
    s.on_bar(bar);

    // Should have stopped out
    EXPECT_EQ(s.position(), 0.0);
    EXPECT_FALSE(s.trading_);
}
```

### 6.3 Paper Trading Test

```bash
cd bitquant_cpp/build
cmake .. -DENABLE_BINANCE_API=ON
make -j$(nproc)

# Run unit tests
./test_grid_martin

# Run paper trading demo
./demo_grid_martin
```

---

## 7. Integration Points

### 7.1 StrategyFactory Integration

Add to `builtin_strategies.cpp`:

```cpp
std::unique_ptr<IStrategy> StrategyFactory::create(const std::string& name) {
    // ... existing strategies ...
    if (name == "GridMartin" || name == "GridMartinStrategy") {
        return std::make_unique<GridMartinStrategy>();
    }
    return nullptr;
}

std::vector<std::string> StrategyFactory::list_strategies() {
    return {
        "DoubleMa", "Rsi", "Bollinger", "AtrRsi", "Turtle",
        "GridMartin"  // Add new strategy
    };
}
```

### 7.2 CMakeLists.txt Integration

```cmake
# Add to src/strategy sources
set(STRATEGY_SOURCES
    src/strategy/builtin_strategies.cpp
    src/strategy/grid_martin_strategy.cpp  # Add
)

# Add demo executable
add_executable(demo_grid_martin
    examples/demo_grid_martin.cpp
    ${STRATEGY_SOURCES}
    ${ENGINE_SOURCES}
)
```

---

## 8. Success Criteria

| Criteria | Verification Method |
|----------|---------------------|
| 网格价格计算正确 | 单元测试通过 |
| 网格穿越检测正确 | 单元测试通过 |
| 买入卖出执行正确 | 模拟盘运行验证 |
| 止损触发正确 | 单元测试 + 模拟盘验证 |
| FIFO平仓顺序正确 | 单元测试通过 |
| 模拟盘稳定运行 | 实时行情连接测试 |

---

## 9. References

- [howtrader Grid Strategy](https://github.com/51bitquant/howtrader)
- [BitQuant C++ Framework](../bitquant_cpp/README.md)
- [Paper Trading Setup](../bitquant_cpp/docs/PAPER_TRADING_SETUP.md)