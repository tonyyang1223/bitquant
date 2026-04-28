/**
 * @file test_ring_buffer.cpp
 * @brief Unit tests for RingBuffer
 */

#include <gtest/gtest.h>
#include "data/ring_buffer.hpp"

using namespace bitquant;

TEST(RingBufferTest, BasicOperations) {
    RingBuffer<int> rb(5);

    EXPECT_TRUE(rb.empty());
    EXPECT_EQ(rb.size(), 0);
    EXPECT_EQ(rb.capacity(), 5);

    rb.push(1);
    EXPECT_FALSE(rb.empty());
    EXPECT_EQ(rb.size(), 1);
    EXPECT_EQ(rb.latest(), 1);

    rb.push(2);
    rb.push(3);
    EXPECT_EQ(rb.size(), 3);
    EXPECT_EQ(rb.latest(), 3);
}

TEST(RingBufferTest, IndexAccess) {
    RingBuffer<int> rb(5);

    rb.push(1);
    rb.push(2);
    rb.push(3);

    // Index 0 = oldest
    EXPECT_EQ(rb[0], 1);
    EXPECT_EQ(rb[1], 2);
    EXPECT_EQ(rb[2], 3);
}

TEST(RingBufferTest, OffsetAccess) {
    RingBuffer<int> rb(5);

    rb.push(1);
    rb.push(2);
    rb.push(3);

    // Offset 0 = latest
    EXPECT_EQ(rb.at(0), 3);
    EXPECT_EQ(rb.at(1), 2);
    EXPECT_EQ(rb.at(2), 1);
}

TEST(RingBufferTest, Overwrite) {
    RingBuffer<int> rb(3);

    rb.push(1);
    rb.push(2);
    rb.push(3);
    EXPECT_TRUE(rb.full());

    rb.push(4);  // Should overwrite 1
    EXPECT_EQ(rb.size(), 3);
    EXPECT_EQ(rb[0], 2);  // Oldest is now 2
    EXPECT_EQ(rb.latest(), 4);
}

TEST(RingBufferTest, TOArray) {
    RingBuffer<int> rb(5);

    rb.push(1);
    rb.push(2);
    rb.push(3);

    auto arr = rb.to_array();
    EXPECT_EQ(arr.size(), 3);
    EXPECT_EQ(arr[0], 1);
    EXPECT_EQ(arr[1], 2);
    EXPECT_EQ(arr[2], 3);
}

TEST(OHLCVRingBufferTest, BasicOperations) {
    OHLCVRingBuffer rb(5);

    EXPECT_FALSE(rb.inited());
    EXPECT_EQ(rb.count(), 0);

    rb.update(1000, 100.0, 105.0, 95.0, 102.0, 1000.0);
    EXPECT_EQ(rb.count(), 1);
    EXPECT_EQ(rb.latest_close(), 102.0);

    // Fill buffer
    rb.update(2000, 102.0, 107.0, 101.0, 106.0, 1200.0);
    rb.update(3000, 106.0, 108.0, 104.0, 105.0, 1100.0);
    rb.update(4000, 105.0, 109.0, 103.0, 108.0, 1300.0);
    rb.update(5000, 108.0, 110.0, 106.0, 109.0, 1400.0);

    EXPECT_TRUE(rb.inited());
    EXPECT_EQ(rb.count(), 5);

    // Check TA-Lib data access
    EXPECT_NE(rb.close_data(), nullptr);
}
