#pragma once

/**
 * @file array.hpp
 * @author Nathan Houwaart (Nathan@Houwaart@student.hu.nl)
 * @brief File contains a templated ringbuffer datastructure
 * @version 1.0
 * @date 12-01-2021
 *
 * @copyright Copyright (c) 2020
 *
 */

#include <stdint.h>


 // ======================================================== //
 //      Ringbuffer Dataclass Declaration and Definition     //
 // ======================================================== //

 /**
  * @brief A Simple Circulair buffer class
  *
  * @tparam T The Type that needs to be stored in the buffer
  * @tparam Buffersize The size of the buffer
  */
template<typename T, size_t Buffersize>
class Ringbuffer {
public:

    /**
     * @brief Construct a new Ringbuffer object
     * The constructor initlaises every entry to it's default value.
     * For an integer or float this will be 0
     */
    Ringbuffer() {
        for (size_t i = 0; i < Buffersize; i++) {
            buffer[i] = T();
        }
    }

    /**
     * @brief Returns and pops the most recent added entry
     *
     * @return T The most recent item
     */
    T pop_back() {
        if (is_empty()) {
            return T();
        }

        size--;
        T retval = buffer[tail];
        tail = (head + size - 1) % Buffersize;
        return retval;
    }

    /**
     * @brief Returns and pops the front entry of the ringbuffer
     *
     * @return T The front entry
     */
    T pop_front() {
        if (is_empty()) {
            return T();
        }

        size--;
        T retval = buffer[head];
        head = (head + 1) % Buffersize;
        return retval;
    }

    /**
     * @brief Stores a new entry in the ringbuffer.
     * If the ringbuffer is full. This entry will overwrite
     * the oldest entry in this buffer.
     *
     * @param data The data entry that needs to be insterted into
     * the ringbuffer
     */
    void put(T data) {
        tail = (tail + 1) % Buffersize;
        if (Buffersize == size) {
            head = (head + 1) % Buffersize;
        }
        else {
            size++;
        }

        buffer[tail] = data;
    }

    /**
     * @brief Returns a bool whether the buffer is empty
     *
     * @return true Buffer is empty
     * @return false Buffer is not empty
     */
    bool is_empty() {
        return size == 0;
    }

    /**
     * @brief  Returns a bool whether the buffer full or not
     *
     * @return true Buffer is full
     * @return false Buffer is empty
     */
    bool is_full() {
        return size == Buffersize;
    }

    /**
     * @brief Get the tail index of the buffer
     *
     * @return size_t
     */
    size_t get_tail() {
        return tail;
    }

    /**
     * Get the current size of the ringbuffer.
     *
     * @return size_t The size of the current buffer
     */
    size_t get_size() {
        return size;
    }

    /**
     * @brief Get the head index of the buffer
     *
     * @return size_t
     */
    size_t get_head() {
        return head;
    }

    /**
     * Get the maximum size of the ringbuffer.
     *
     * @return the max buffersize
     */
    size_t get_max_size() {
        return Buffersize;
    }

    /**
     * @brief clears all entries in the ringbuffer
     *
     */
    void clear() {
        head = 0;
        tail = Buffersize - 1;
        size = 0;
    }

    /**
     * @brief Bracket operator to acces the ringbuffer.
     * index 0 indicates to the head item in the buffer
     *
     * @param index The index of the item that needs to be accessed
     * @return const T& The item of the ringbugger
     */
    const T& operator[](const size_t index) {
        return buffer[(((head + index) % Buffersize))];
    }

    /**
     * @brief Bracket operator to acces the ringbuffer.
     * index 0 indicates to the head item in the buffer
     *
     * @param index The index of the item that needs to be accessed
     * @return const T& The item of the ringbugger
     */
    const T& operator[](const size_t index) const {
        return buffer[(head + index) % Buffersize];
    }

protected:
    T buffer[Buffersize] = {};

    size_t head = 0;
    size_t tail = Buffersize - 1;
    size_t size = 0;
};