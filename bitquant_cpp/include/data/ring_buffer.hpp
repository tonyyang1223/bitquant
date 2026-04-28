/**
 * @file ring_buffer.hpp
 * @brief O(1) ring buffer implementation for high-performance data storage
 *
 * Replaces Python's O(n) array shift with circular buffer for O(1) operations.
 */

#pragma once

#include <vector>
#include <stdexcept>
#include <algorithm>
#include <cstring>

namespace bitquant {

/**
 * @brief Generic ring buffer with O(1) push and access
 * @tparam T Element type
 */
template<typename T>
class RingBuffer {
public:
    explicit RingBuffer(size_t capacity)
        : buffer_(capacity), capacity_(capacity), head_(0), count_(0) {}

    /**
     * @brief Push a new element (O(1))
     */
    void push(const T& value) {
        buffer_[head_] = value;
        head_ = (head_ + 1) % capacity_;
        if (count_ < capacity_) {
            ++count_;
        }
    }

    /**
     * @brief Push a new element by move (O(1))
     */
    void push(T&& value) {
        buffer_[head_] = std::move(value);
        head_ = (head_ + 1) % capacity_;
        if (count_ < capacity_) {
            ++count_;
        }
    }

    /**
     * @brief Get the most recent value (last pushed)
     */
    const T& latest() const {
        if (count_ == 0) {
            throw std::out_of_range("RingBuffer is empty");
        }
        return buffer_[(head_ + capacity_ - 1) % capacity_];
    }

    /**
     * @brief Get value at offset from latest (0 = latest, 1 = one before, etc.)
     */
    const T& at(size_t offset) const {
        if (offset >= count_) {
            throw std::out_of_range("Invalid offset in RingBuffer");
        }
        size_t idx = (head_ + capacity_ - 1 - offset) % capacity_;
        return buffer_[idx];
    }

    /**
     * @brief Get value as if it were a regular array (index 0 = oldest)
     */
    const T& operator[](size_t index) const {
        if (index >= count_) {
            throw std::out_of_range("Index out of range in RingBuffer");
        }
        size_t start = (head_ + capacity_ - count_) % capacity_;
        return buffer_[(start + index) % capacity_];
    }

    /**
     * @brief Get mutable reference (index 0 = oldest)
     */
    T& operator[](size_t index) {
        if (index >= count_) {
            throw std::out_of_range("Index out of range in RingBuffer");
        }
        size_t start = (head_ + capacity_ - count_) % capacity_;
        return buffer_[(start + index) % capacity_];
    }

    // Status methods
    bool full() const { return count_ == capacity_; }
    bool empty() const { return count_ == 0; }
    size_t size() const { return count_; }
    size_t capacity() const { return capacity_; }

    /**
     * @brief Clear all elements
     */
    void clear() {
        head_ = 0;
        count_ = 0;
    }

    /**
     * @brief Convert to contiguous array (for TA-Lib compatibility)
     */
    std::vector<T> to_array() const {
        std::vector<T> result(count_);
        for (size_t i = 0; i < count_; ++i) {
            result[i] = (*this)[i];
        }
        return result;
    }

private:
    std::vector<T> buffer_;
    size_t capacity_;
    size_t head_;     // Next write position
    size_t count_;    // Number of elements
};

/**
 * @brief Specialized ring buffer for OHLCV data with TA-Lib integration
 *
 * Maintains separate arrays for open, high, low, close, volume
 * for direct TA-Lib function access.
 */
class OHLCVRingBuffer {
public:
    explicit OHLCVRingBuffer(size_t size);

    /**
     * @brief Update with new bar data (O(1))
     */
    void update(int64_t datetime, double open, double high,
                double low, double close, double volume);

    /**
     * @brief Check if buffer is fully initialized
     */
    bool inited() const { return inited_; }

    /**
     * @brief Get current count
     */
    size_t count() const { return count_; }

    /**
     * @brief Get capacity
     */
    size_t capacity() const { return capacity_; }

    /**
     * @brief Get raw data arrays for TA-Lib (oldest first)
     * Returns nullptr if not fully initialized
     */
    const double* open_data() const;
    const double* high_data() const;
    const double* low_data() const;
    const double* close_data() const;
    const double* volume_data() const;

    /**
     * @brief Get latest values
     */
    double latest_open() const;
    double latest_high() const;
    double latest_low() const;
    double latest_close() const;
    double latest_volume() const;
    int64_t latest_datetime() const;

    /**
     * @brief Get value at offset from latest (0 = latest)
     */
    double close_at(size_t offset) const;

private:
    std::vector<double> open_;
    std::vector<double> high_;
    std::vector<double> low_;
    std::vector<double> close_;
    std::vector<double> volume_;
    std::vector<int64_t> datetime_;

    size_t capacity_;
    size_t head_;
    size_t count_;
    bool inited_;

    // Contiguous array for TA-Lib (reused)
    mutable std::vector<double> ta_open_;
    mutable std::vector<double> ta_high_;
    mutable std::vector<double> ta_low_;
    mutable std::vector<double> ta_close_;
    mutable std::vector<double> ta_volume_;
    mutable bool ta_data_valid_;

    void update_ta_data() const;
};

} // namespace bitquant
