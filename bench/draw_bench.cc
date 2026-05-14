#include "mira/draw/draw.hpp"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <new>

namespace {

std::atomic<mira::b8> g_count_allocations = false;
std::atomic<mira::u64> g_allocation_calls = 0;
std::atomic<mira::u64> g_allocation_bytes = 0;

void RecordAllocation(mira::usize size) {
    if (!g_count_allocations.load(std::memory_order_relaxed)) {
        return;
    }
    g_allocation_calls.fetch_add(1, std::memory_order_relaxed);
    g_allocation_bytes.fetch_add(size, std::memory_order_relaxed);
}

template <typename Integer> void PrintMetric(const char *name, Integer value) {
    std::cout << name << '=' << static_cast<mira::u64>(value) << '\n';
}

void PrintMetric(const char *name, mira::f64 value) {
    std::cout << name << '=' << std::fixed << std::setprecision(3) << value << '\n';
}

} // namespace

void *operator new(mira::usize size) {
    RecordAllocation(size);
    if (void *pointer = std::malloc(size)) {
        return pointer;
    }
    std::abort();
}

void *operator new[](mira::usize size) {
    RecordAllocation(size);
    if (void *pointer = std::malloc(size)) {
        return pointer;
    }
    std::abort();
}

void *operator new(mira::usize size, std::align_val_t alignment) {
    RecordAllocation(size);
    void *pointer = nullptr;
    if (posix_memalign(&pointer, static_cast<mira::usize>(alignment), size) == 0) {
        return pointer;
    }
    std::abort();
}

void *operator new[](mira::usize size, std::align_val_t alignment) {
    RecordAllocation(size);
    void *pointer = nullptr;
    if (posix_memalign(&pointer, static_cast<mira::usize>(alignment), size) == 0) {
        return pointer;
    }
    std::abort();
}

void operator delete(void *pointer) noexcept { std::free(pointer); }
void operator delete[](void *pointer) noexcept { std::free(pointer); }
void operator delete(void *pointer, mira::usize) noexcept { std::free(pointer); }
void operator delete[](void *pointer, mira::usize) noexcept { std::free(pointer); }
void operator delete(void *pointer, std::align_val_t) noexcept { std::free(pointer); }
void operator delete[](void *pointer, std::align_val_t) noexcept { std::free(pointer); }
void operator delete(void *pointer, mira::usize, std::align_val_t) noexcept { std::free(pointer); }
void operator delete[](void *pointer, mira::usize, std::align_val_t) noexcept {
    std::free(pointer);
}

auto main() -> int {
    constexpr mira::i32 kWidth = 1120;
    constexpr mira::i32 kHeight = 720;
    constexpr mira::i32 kFrames = 200000;

    mira::DrawList list;
    const mira::Screen screen = mira::screen_for(kWidth, kHeight);
    mira::build_demo(&list, screen);

    g_allocation_calls.store(0, std::memory_order_relaxed);
    g_allocation_bytes.store(0, std::memory_order_relaxed);
    g_count_allocations.store(true, std::memory_order_relaxed);
    const auto start = std::chrono::steady_clock::now();
    for (mira::i32 frame = 0; frame < kFrames; ++frame) {
        mira::build_demo(&list, screen);
    }
    const auto end = std::chrono::steady_clock::now();
    g_count_allocations.store(false, std::memory_order_relaxed);

    const auto elapsed = std::chrono::duration<mira::f64, std::micro>(end - start).count();
    const mira::f64 frame_us = elapsed / static_cast<mira::f64>(kFrames);
    const mira::f64 upload_bpp =
        static_cast<mira::f64>(list.upload_bytes()) / static_cast<mira::f64>(kWidth * kHeight);

    if (g_allocation_calls.load(std::memory_order_relaxed) != 0) {
        std::cerr << "list builder allocated after warmup\n";
        return 1;
    }
    if (upload_bpp > 1.0) {
        std::cerr << "upload budget exceeded\n";
        return 1;
    }

    PrintMetric("mira_draw_bench_version", 2);
    PrintMetric("frames", kFrames);
    PrintMetric("cpu_frame_us", frame_us);
    PrintMetric("upload_bytes_per_frame", list.upload_bytes());
    PrintMetric("upload_bytes_per_pixel", upload_bpp);
    PrintMetric("draws_storage_bytes", sizeof(list));
    PrintMetric("draw_count", list.draws.size());
    PrintMetric("clip_count", list.clips.size());
    PrintMetric("text_bytes", list.text.size());
    PrintMetric("allocation_calls", g_allocation_calls.load(std::memory_order_relaxed));
    PrintMetric("allocation_bytes", g_allocation_bytes.load(std::memory_order_relaxed));
    PrintMetric("overflow_count", list.overflow_count());
    return 0;
}
