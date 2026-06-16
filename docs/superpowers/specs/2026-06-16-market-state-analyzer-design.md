# Market State Analyzer Design

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Create a market state analyzer that determines whether the market is in consolidation (range-bound) or trending state, enabling the Grid Martin strategy to start/stop grid trading dynamically.

**Architecture:** Use volatility channel (standard deviation band) to measure price range width. When channel width <= threshold, market is consolidating (suitable for grid trading). When width > threshold, market is trending (stop trading and wait).

**Tech Stack:** C++20, RingBuffer for O(1) rolling calculation, std::deque for price history, integration with existing GridMartinStrategy

---

## Context

**Problem:** Grid trading works best in consolidation/range-bound markets. In trending markets, it accumulates positions in one direction and suffers losses when the trend continues. Current GridMartinStrategy runs indefinitely until stop loss triggers, then stops permanently.

**Root Cause:**
- No mechanism to detect market state (consolidation vs trending)
- Strategy cannot pause when market becomes unfavorable
- After stop loss, strategy stops permanently without restart mechanism

**Solution:** Add a MarketStateAnalyzer component that:
1. Calculates volatility channel width using rolling standard deviation
2. Determines market state: CONSOLIDATION (width <= threshold) or TRENDING (width > threshold)
3. Provides state transition signals to GridMartinStrategy
4. Strategy handles transitions: close positions when trending, reset grid when consolidation resumes

---

## Design Decisions

| Decision | Choice | Reason |
|----------|--------|--------|
| Detection method | Volatility channel (StdDev band) | Intuitive, matches grid trading logic |
| Calculation period | 50 bars (configurable 50-100) | Balance responsiveness and stability |
| Width threshold | 5% | Wide threshold for BTC volatility |
| Position handling on trend | Immediate close | Risk control priority |
| New base_price on restart | Current market price | Simple, grid centered on current price |
| Confirmation mechanism | 2 consecutive bars | Prevent noise-triggered transitions |

---

## Components

### 1. MarketStateAnalyzer (New Class)

**File:** `include/data/market_state_analyzer.hpp`

**Purpose:** Calculate volatility channel and determine market state.

**Interface:**
```cpp
enum class MarketState {
    WAITING,        // Initializing, not enough data
    CONSOLIDATION,  // Range-bound, suitable for grid trading
    TRENDING        // Trending, should pause trading
};

struct MarketStateConfig {
    int period = 50;                // Rolling calculation period
    double width_threshold_pct = 5.0;  // Channel width threshold (%)
    int confirmation_bars = 2;      // Bars needed to confirm state change
};

class MarketStateAnalyzer {
public:
    explicit MarketStateAnalyzer(const MarketStateConfig& config = {});
    
    // Process new bar data, return current state
    MarketState update(double high, double low, double close);
    
    // Getters
    MarketState state() const;
    double upper_band() const;
    double lower_band() const;
    double middle_band() const;
    double channel_width_pct() const;
    bool is_initialized() const;
    
    // Reset for restart
    void reset();

private:
    MarketStateConfig config_;
    RingBuffer<double> close_buffer_;
    
    double current_middle_ = 0.0;
    double current_upper_ = 0.0;
    double current_lower_ = 0.0;
    double current_width_ = 0.0;
    
    MarketState current_state_ = MarketState::WAITING;
    MarketState pending_state_ = MarketState::WAITING;
    int confirmation_count_ = 0;
    
    void calculate_bands();
    MarketState determine_state(double width) const;
};
```

**Algorithm:**
```
Middle Band = SMA(close, N periods)
Upper Band = Middle + 2 × StdDev(close, N periods)
Lower Band = Middle - 2 × StdDev(close, N periods)
Channel Width = (Upper - Lower) / Middle × 100%

State Logic:
- If width <= threshold: CONSOLIDATION
- If width > threshold: TRENDING

Confirmation:
- Track pending state when state changes
- Only update current_state after confirmation_bars consecutive same signals
```

### 2. GridMartinStrategy Extensions

**File:** `include/strategy/grid_martin_strategy.hpp` (Modify)

**Additions:**
```cpp
// New enum and includes
#include "data/market_state_analyzer.hpp"

// New members in class
private:
    MarketState current_market_state_ = MarketState::WAITING;
    std::unique_ptr<MarketStateAnalyzer> state_analyzer_;
    
    // Configuration for state analyzer
    int state_period_ = 50;
    double state_width_threshold_ = 5.0;
    int state_confirmation_bars_ = 2;
    
    // State transition methods
    void handle_state_transition(MarketState new_state, double price);
    void transition_to_trending(double price);
    void transition_to_consolidation(double price);
```

### 3. State Transition Logic

**File:** `src/strategy/grid_martin_strategy.cpp` (Modify)

**transition_to_trending:**
```
1. If position > 0, sell all at current price
2. Log close position details (price, volume, profit/loss)
3. Clear all grid state (positions, costs, queue)
4. Set trading_ = false
5. Set current_market_state_ = TRENDING
```

**transition_to_consolidation:**
```
1. Set base_price_ = current price
2. Recalculate grid levels via calculate_grid_levels()
3. Reset grid_positions_ and grid_costs_
4. Reset PriceSmoother
5. Clear buy_queue_
6. Set last_grid_index_ = -1
7. Set trading_ = true
8. Set current_market_state_ = CONSOLIDATION
9. Log new grid setup
```

---

## File Structure

| File | Action | Purpose |
|------|--------|---------|
| `include/data/market_state_analyzer.hpp` | Create | MarketStateAnalyzer class definition |
| `src/data/market_state_analyzer.cpp` | Create | Implementation of band calculation and state logic |
| `include/strategy/grid_martin_strategy.hpp` | Modify | Add state analyzer member and transition methods |
| `src/strategy/grid_martin_strategy.cpp` | Modify | Integrate state analyzer in on_bar, implement transitions |
| `tests/unit/test_market_state_analyzer.cpp` | Create | Unit tests for analyzer |
| `CMakeLists.txt` | Modify | Add new source file and test |

---

## Data Flow

```
WebSocket Tick
     ↓
BarGenerator.on_tick()
     ↓
BarGenerator.on_bar() → MarketStateAnalyzer.update(high, low, close)
                              ↓
                        Calculate bands, check width
                              ↓
                        Determine pending state
                              ↓
                        Confirm after 2 bars → Return state
                              ↓
GridMartinStrategy.on_bar()
     ↓
Receive MarketState from analyzer
     ↓
If state changed:
     - TRENDING: transition_to_trending(price)
     - CONSOLIDATION: transition_to_consolidation(price)
Else if CONSOLIDATION:
     - Continue normal grid trading logic
Else if TRENDING:
     - Do nothing (wait)
```

---

## Testing Strategy

### Unit Tests for MarketStateAnalyzer

1. **test_initialization** - New analyzer starts in WAITING state
2. **test_warmup** - After period bars, should have valid state
3. **test_consolidation_detection** - Small volatility → CONSOLIDATION
4. **test_trending_detection** - Large volatility → TRENDING
5. **test_state_transition_with_confirmation** - Requires 2 bars to transition
6. **test_reset** - Reset clears all state
7. **test_channel_values** - Verify band calculations are correct

### Integration Tests

1. Run demo with simulated price data showing consolidation → trend → consolidation
2. Verify strategy starts grid in consolidation
3. Verify strategy closes position when trending
4. Verify strategy restarts grid when consolidation resumes

---

## Configuration Parameters

| Parameter | Default | Range | Description |
|-----------|---------|-------|-------------|
| `state_period` | 50 | 50-100 | Rolling calculation period for bands |
| `state_width_threshold` | 5.0 | 4-5% | Channel width threshold (%) |
| `state_confirmation_bars` | 2 | 1-3 | Bars needed to confirm state change |

---

## Edge Cases

| Scenario | Handling |
|----------|----------|
| Insufficient bars (warm-up) | State = WAITING, no trading |
| Single bar anomaly | Confirmation mechanism prevents transition |
| Rapid state oscillation | Requires 2 bars, reduces noise |
| Position during transition to trending | Immediate close at current price |
| Zero position during trending | No action needed, just wait |

---

## Summary

- **New Component:** MarketStateAnalyzer (2 files)
- **Modified:** GridMartinStrategy (2 files)
- **Tests:** New unit test file
- **Key benefit:** Strategy dynamically adapts to market conditions, starts grid in consolidation, stops in trend, restarts when conditions favorable
- **Risk:** Channel width threshold may need tuning for different assets/timeframes