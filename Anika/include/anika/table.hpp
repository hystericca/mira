#pragma once

#include <array>
#include <span>
#include <type_traits>
#include <utility>

#include "anika/types.hpp"

namespace anika {

/* overflow sets a flag instead of allocating */
template <typename T, usize Capacity> class Table {
    static_assert(std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T>);

  public:
    b8 overflowed = false;

    constexpr Table() = default;
    Table(const Table &) = delete;
    auto operator=(const Table &) -> Table & = delete;
    Table(Table &&) = delete;
    auto operator=(Table &&) -> Table & = delete;

    constexpr void clear() {
        size_ = 0;
        overflowed = false;
    }

    constexpr void truncate(usize size) {
        if (size >= size_) {
            return;
        }
        for (usize index = size; index < size_; ++index) {
            values_[index] = {};
        }
        size_ = size;
        overflowed = false;
    }

    [[nodiscard]] constexpr usize size() const { return size_; }
    [[nodiscard]] static constexpr usize capacity() { return Capacity; }
    [[nodiscard]] constexpr b8 empty() const { return size_ == 0; }
    [[nodiscard]] constexpr usize byte_size() const { return size_ * sizeof(T); }
    [[nodiscard]] static constexpr usize byte_capacity() { return Capacity * sizeof(T); }

    [[nodiscard]] constexpr T *data() { return values_.data(); }
    [[nodiscard]] constexpr const T *data() const { return values_.data(); }
    [[nodiscard]] constexpr std::span<T> span() { return {values_.data(), size_}; }
    [[nodiscard]] constexpr std::span<const T> span() const { return {values_.data(), size_}; }

    [[nodiscard]] constexpr T *begin() { return values_.data(); }
    [[nodiscard]] constexpr T *end() { return values_.data() + size_; }
    [[nodiscard]] constexpr const T *begin() const { return values_.data(); }
    [[nodiscard]] constexpr const T *end() const { return values_.data() + size_; }

    [[nodiscard]] constexpr b8 push(const T &value) {
        if (size_ >= Capacity) {
            overflowed = true;
            return false;
        }
        values_[size_] = value;
        ++size_;
        return true;
    }

    [[nodiscard]] constexpr b8 push(T &&value) {
        if (size_ >= Capacity) {
            overflowed = true;
            return false;
        }
        values_[size_] = std::move(value);
        ++size_;
        return true;
    }

    [[nodiscard]] constexpr b8 insert(usize index, const T &value) {
        if (size_ >= Capacity) {
            overflowed = true;
            return false;
        }
        if (index > size_) {
            index = size_;
        }
        for (usize move_index = size_; move_index > index; --move_index) {
            values_[move_index] = std::move(values_[move_index - 1]);
        }
        values_[index] = value;
        ++size_;
        return true;
    }

    [[nodiscard]] constexpr b8 insert(usize index, T &&value) {
        if (size_ >= Capacity) {
            overflowed = true;
            return false;
        }
        if (index > size_) {
            index = size_;
        }
        for (usize move_index = size_; move_index > index; --move_index) {
            values_[move_index] = std::move(values_[move_index - 1]);
        }
        values_[index] = std::move(value);
        ++size_;
        return true;
    }

    [[nodiscard]] constexpr b8 erase(usize index) {
        if (index >= size_) {
            return false;
        }
        for (usize move_index = index; move_index + 1 < size_; ++move_index) {
            values_[move_index] = std::move(values_[move_index + 1]);
        }
        values_[size_ - 1] = {};
        --size_;
        return true;
    }

    [[nodiscard]] constexpr T &operator[](usize index) { return values_[index]; }
    [[nodiscard]] constexpr const T &operator[](usize index) const { return values_[index]; }

  private:
    std::array<T, Capacity> values_ = {};
    usize size_ = 0;
};

} // namespace anika
