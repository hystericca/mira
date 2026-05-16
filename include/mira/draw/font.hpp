#pragma once

#include <array>

#include "mira/types.hpp"

namespace mira {

constexpr u32 kFontFirst = 32;
constexpr u32 kFontCount = 95;
constexpr u32 kFontWidthPixels = 8;
constexpr u32 kFontHeightPixels = 13;
constexpr u32 kFontAscentPixels = 10;
constexpr u32 kFontStorageRows = 16;

struct GpuFontGlyph {
    std::array<u32, 4> metrics = {};
    std::array<u32, kFontStorageRows> rows = {};
};
static_assert(sizeof(GpuFontGlyph) == 80);

struct GpuFont {
    std::array<u32, 4> metrics = {};
    std::array<GpuFontGlyph, kFontCount> glyphs = {};
};
static_assert(sizeof(GpuFont) == 7616);

[[nodiscard]] const GpuFont &font();
[[nodiscard]] u32 fontrow(u32 code, u32 row);

} // namespace mira
