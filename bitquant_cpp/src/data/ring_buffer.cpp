/**
 * @file ring_buffer.cpp
 * @brief Implementation of OHLCVRingBuffer
 */

#include "data/ring_buffer.hpp"
#include <algorithm>

namespace bitquant {

OHLCVRingBuffer::OHLCVRingBuffer(size_t size)
    : open_(size, 0.0),
      high_(size, 0.0),
      low_(size, 0.0),
      close_(size, 0.0),
      volume_(size, 0.0),
      datetime_(size, 0),
      capacity_(size),
      head_(0),
      count_(0),
      inited_(false),
      ta_open_(size),
      ta_high_(size),
      ta_low_(size),
      ta_close_(size),
      ta_volume_(size),
      ta_data_valid_(false) {}

void OHLCVRingBuffer::update(int64_t dt, double open, double high,
                              double low, double close, double vol) {
    open_[head_] = open;
    high_[head_] = high;
    low_[head_] = low;
    close_[head_] = close;
    volume_[head_] = vol;
    datetime_[head_] = dt;

    head_ = (head_ + 1) % capacity_;

    if (count_ < capacity_) {
        ++count_;
        if (count_ == capacity_) {
            inited_ = true;
        }
    }

    // Invalidate cached TA data
    ta_data_valid_ = false;
}

void OHLCVRingBuffer::update_ta_data() const {
    if (ta_data_valid_ || !inited_) {
        return;
    }

    // Copy data in chronological order (oldest first) for TA-Lib
    size_t start = head_;  // This is where the oldest data is
    for (size_t i = 0; i < capacity_; ++i) {
        size_t idx = (start + i) % capacity_;
        ta_open_[i] = open_[idx];
        ta_high_[i] = high_[idx];
        ta_low_[i] = low_[idx];
        ta_close_[i] = close_[idx];
        ta_volume_[i] = volume_[idx];
    }

    ta_data_valid_ = true;
}

const double* OHLCVRingBuffer::open_data() const {
    if (!inited_) return nullptr;
    update_ta_data();
    return ta_open_.data();
}

const double* OHLCVRingBuffer::high_data() const {
    if (!inited_) return nullptr;
    update_ta_data();
    return ta_high_.data();
}

const double* OHLCVRingBuffer::low_data() const {
    if (!inited_) return nullptr;
    update_ta_data();
    return ta_low_.data();
}

const double* OHLCVRingBuffer::close_data() const {
    if (!inited_) return nullptr;
    update_ta_data();
    return ta_close_.data();
}

const double* OHLCVRingBuffer::volume_data() const {
    if (!inited_) return nullptr;
    update_ta_data();
    return ta_volume_.data();
}

double OHLCVRingBuffer::latest_open() const {
    if (count_ == 0) return 0.0;
    size_t idx = (head_ + capacity_ - 1) % capacity_;
    return open_[idx];
}

double OHLCVRingBuffer::latest_high() const {
    if (count_ == 0) return 0.0;
    size_t idx = (head_ + capacity_ - 1) % capacity_;
    return high_[idx];
}

double OHLCVRingBuffer::latest_low() const {
    if (count_ == 0) return 0.0;
    size_t idx = (head_ + capacity_ - 1) % capacity_;
    return low_[idx];
}

double OHLCVRingBuffer::latest_close() const {
    if (count_ == 0) return 0.0;
    size_t idx = (head_ + capacity_ - 1) % capacity_;
    return close_[idx];
}

double OHLCVRingBuffer::latest_volume() const {
    if (count_ == 0) return 0.0;
    size_t idx = (head_ + capacity_ - 1) % capacity_;
    return volume_[idx];
}

int64_t OHLCVRingBuffer::latest_datetime() const {
    if (count_ == 0) return 0;
    size_t idx = (head_ + capacity_ - 1) % capacity_;
    return datetime_[idx];
}

double OHLCVRingBuffer::close_at(size_t offset) const {
    if (offset >= count_) {
        throw std::out_of_range("Invalid offset in OHLCVRingBuffer");
    }
    size_t idx = (head_ + capacity_ - 1 - offset) % capacity_;
    return close_[idx];
}

} // namespace bitquant
