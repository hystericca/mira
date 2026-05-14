#pragma once

#include <span>
#include <string_view>

#include "mira/base/table.hpp"
#include "mira/types.hpp"

namespace mira {

constexpr usize kMaxDraws = 1024;
constexpr usize kMaxClips = 256;
constexpr usize kMaxTextBytes = 4096;
constexpr usize kMaxTextWords = (kMaxTextBytes + sizeof(u32) - 1U) / sizeof(u32);
constexpr usize kMaxSamples = 24576;

enum class DrawKind : u8 {
    kFill,
    kStroke,
    kDash,
    kText,
    kGraph,
};

struct Rect {
    f32 x = 0.0F;
    f32 y = 0.0F;
    f32 width = 0.0F;
    f32 height = 0.0F;
};

struct TextRange {
    u32 offset = 0;
    u16 length = 0;
};

struct Screen {
    i32 scale = 1;
    i32 width = 1;
    i32 height = 1;
};

struct Clip {
    f32 x0 = 0.0F;
    f32 y0 = 0.0F;
    f32 x1 = 0.0F;
    f32 y1 = 0.0F;
};

/* one row copied to the gpu */
struct Draw {
    f32 x0 = 0.0F;
    f32 y0 = 0.0F;
    f32 x1 = 0.0F;
    f32 y1 = 0.0F;
    f32 p0 = 0.0F;
    f32 p1 = 0.0F;
    u32 data0 = 0;
    u32 data1 = 0;
};

struct Sample {
    f32 y = 0.0F;
    u32 flags = 0;
};

static_assert(sizeof(Clip) == 16);
static_assert(sizeof(Draw) == 32);
static_assert(sizeof(Sample) == 8);

struct DrawList {
    Table<Draw, kMaxDraws> draws;
    Table<Clip, kMaxClips> clips;
    Table<char, kMaxTextBytes> text;
    Table<Sample, kMaxSamples> samples;

    void clear();
    [[nodiscard]] usize upload_bytes() const;
    [[nodiscard]] u32 overflow_count() const;
};

struct DrawView {
    std::span<const Draw> draws;
    std::span<const Clip> clips;
    std::span<const char> text;
    std::span<const Sample> samples;
};

[[nodiscard]] Screen screen_for(i32 width, i32 height);
[[nodiscard]] DrawView view(const DrawList &list);
[[nodiscard]] TextRange add_text(DrawList *list, std::string_view text);
[[nodiscard]] b8 add_clip(DrawList *list, Clip clip);
[[nodiscard]] b8 add_draw(DrawList *list, DrawKind kind, u8 clip_index, u8 luma, Rect rect,
                          f32 p0 = 0.0F, f32 p1 = 0.0F, u32 data0 = 0);
void build_demo(DrawList *list, Screen screen);

} // namespace mira
