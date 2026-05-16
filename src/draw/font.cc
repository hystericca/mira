#include "mira/draw/font.hpp"

namespace mira {
namespace {

using SourceGlyph = std::array<u32, kFontHeightPixels>;

// 6x13 ascii no scope shoutout plan 9
constexpr std::array<SourceGlyph, kFontCount> kSourceGlyphs = {{
    SourceGlyph{0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U},
    SourceGlyph{0U, 0U, 8U, 8U, 8U, 8U, 8U, 8U, 8U, 0U, 8U, 0U, 0U},
    SourceGlyph{0U, 0U, 20U, 20U, 20U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U},
    SourceGlyph{0U, 0U, 0U, 20U, 20U, 62U, 20U, 62U, 20U, 20U, 0U, 0U, 0U},
    SourceGlyph{0U, 0U, 8U, 30U, 40U, 40U, 28U, 10U, 10U, 60U, 8U, 0U, 0U},
    SourceGlyph{0U, 0U, 18U, 42U, 20U, 4U, 8U, 16U, 20U, 42U, 36U, 0U, 0U},
    SourceGlyph{0U, 0U, 0U, 16U, 40U, 40U, 16U, 40U, 38U, 36U, 26U, 0U, 0U},
    SourceGlyph{0U, 0U, 8U, 8U, 8U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U},
    SourceGlyph{0U, 4U, 8U, 8U, 16U, 16U, 16U, 16U, 16U, 8U, 8U, 4U, 0U},
    SourceGlyph{0U, 16U, 8U, 8U, 4U, 4U, 4U, 4U, 4U, 8U, 8U, 16U, 0U},
    SourceGlyph{0U, 0U, 8U, 42U, 28U, 42U, 8U, 0U, 0U, 0U, 0U, 0U, 0U},
    SourceGlyph{0U, 0U, 0U, 0U, 8U, 8U, 62U, 8U, 8U, 0U, 0U, 0U, 0U},
    SourceGlyph{0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 12U, 8U, 16U, 0U},
    SourceGlyph{0U, 0U, 0U, 0U, 0U, 0U, 62U, 0U, 0U, 0U, 0U, 0U, 0U},
    SourceGlyph{0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 8U, 28U, 8U, 0U},
    SourceGlyph{0U, 0U, 2U, 2U, 4U, 4U, 8U, 16U, 16U, 32U, 32U, 0U, 0U},
    SourceGlyph{0U, 0U, 8U, 20U, 34U, 34U, 34U, 34U, 34U, 20U, 8U, 0U, 0U},
    SourceGlyph{0U, 0U, 8U, 24U, 40U, 8U, 8U, 8U, 8U, 8U, 62U, 0U, 0U},
    SourceGlyph{0U, 0U, 28U, 34U, 34U, 2U, 4U, 8U, 16U, 32U, 62U, 0U, 0U},
    SourceGlyph{0U, 0U, 62U, 2U, 4U, 8U, 28U, 2U, 2U, 34U, 28U, 0U, 0U},
    SourceGlyph{0U, 0U, 4U, 4U, 12U, 20U, 20U, 36U, 62U, 4U, 4U, 0U, 0U},
    SourceGlyph{0U, 0U, 62U, 32U, 32U, 44U, 50U, 2U, 2U, 34U, 28U, 0U, 0U},
    SourceGlyph{0U, 0U, 28U, 34U, 32U, 32U, 60U, 34U, 34U, 34U, 28U, 0U, 0U},
    SourceGlyph{0U, 0U, 62U, 2U, 4U, 4U, 8U, 8U, 16U, 16U, 16U, 0U, 0U},
    SourceGlyph{0U, 0U, 28U, 34U, 34U, 34U, 28U, 34U, 34U, 34U, 28U, 0U, 0U},
    SourceGlyph{0U, 0U, 28U, 34U, 34U, 34U, 30U, 2U, 2U, 34U, 28U, 0U, 0U},
    SourceGlyph{0U, 0U, 0U, 0U, 8U, 28U, 8U, 0U, 0U, 8U, 28U, 8U, 0U},
    SourceGlyph{0U, 0U, 0U, 0U, 8U, 28U, 8U, 0U, 0U, 12U, 8U, 16U, 0U},
    SourceGlyph{0U, 0U, 2U, 4U, 8U, 16U, 32U, 16U, 8U, 4U, 2U, 0U, 0U},
    SourceGlyph{0U, 0U, 0U, 0U, 0U, 62U, 0U, 0U, 62U, 0U, 0U, 0U, 0U},
    SourceGlyph{0U, 0U, 32U, 16U, 8U, 4U, 2U, 4U, 8U, 16U, 32U, 0U, 0U},
    SourceGlyph{0U, 0U, 28U, 34U, 34U, 2U, 4U, 8U, 8U, 0U, 8U, 0U, 0U},
    SourceGlyph{0U, 0U, 28U, 34U, 34U, 38U, 42U, 42U, 44U, 32U, 30U, 0U, 0U},
    SourceGlyph{0U, 0U, 8U, 20U, 34U, 34U, 34U, 62U, 34U, 34U, 34U, 0U, 0U},
    SourceGlyph{0U, 0U, 60U, 18U, 18U, 18U, 28U, 18U, 18U, 18U, 60U, 0U, 0U},
    SourceGlyph{0U, 0U, 28U, 34U, 32U, 32U, 32U, 32U, 32U, 34U, 28U, 0U, 0U},
    SourceGlyph{0U, 0U, 60U, 18U, 18U, 18U, 18U, 18U, 18U, 18U, 60U, 0U, 0U},
    SourceGlyph{0U, 0U, 62U, 32U, 32U, 32U, 60U, 32U, 32U, 32U, 62U, 0U, 0U},
    SourceGlyph{0U, 0U, 62U, 32U, 32U, 32U, 60U, 32U, 32U, 32U, 32U, 0U, 0U},
    SourceGlyph{0U, 0U, 28U, 34U, 32U, 32U, 32U, 38U, 34U, 34U, 28U, 0U, 0U},
    SourceGlyph{0U, 0U, 34U, 34U, 34U, 34U, 62U, 34U, 34U, 34U, 34U, 0U, 0U},
    SourceGlyph{0U, 0U, 28U, 8U, 8U, 8U, 8U, 8U, 8U, 8U, 28U, 0U, 0U},
    SourceGlyph{0U, 0U, 14U, 4U, 4U, 4U, 4U, 4U, 4U, 36U, 24U, 0U, 0U},
    SourceGlyph{0U, 0U, 34U, 34U, 36U, 40U, 48U, 40U, 36U, 34U, 34U, 0U, 0U},
    SourceGlyph{0U, 0U, 32U, 32U, 32U, 32U, 32U, 32U, 32U, 32U, 62U, 0U, 0U},
    SourceGlyph{0U, 0U, 34U, 34U, 54U, 42U, 42U, 34U, 34U, 34U, 34U, 0U, 0U},
    SourceGlyph{0U, 0U, 34U, 50U, 50U, 42U, 42U, 38U, 38U, 34U, 34U, 0U, 0U},
    SourceGlyph{0U, 0U, 28U, 34U, 34U, 34U, 34U, 34U, 34U, 34U, 28U, 0U, 0U},
    SourceGlyph{0U, 0U, 60U, 34U, 34U, 34U, 60U, 32U, 32U, 32U, 32U, 0U, 0U},
    SourceGlyph{0U, 0U, 28U, 34U, 34U, 34U, 34U, 34U, 34U, 42U, 28U, 2U, 0U},
    SourceGlyph{0U, 0U, 60U, 34U, 34U, 34U, 60U, 40U, 36U, 34U, 34U, 0U, 0U},
    SourceGlyph{0U, 0U, 28U, 34U, 32U, 32U, 28U, 2U, 2U, 34U, 28U, 0U, 0U},
    SourceGlyph{0U, 0U, 62U, 8U, 8U, 8U, 8U, 8U, 8U, 8U, 8U, 0U, 0U},
    SourceGlyph{0U, 0U, 34U, 34U, 34U, 34U, 34U, 34U, 34U, 34U, 28U, 0U, 0U},
    SourceGlyph{0U, 0U, 34U, 34U, 34U, 34U, 20U, 20U, 20U, 8U, 8U, 0U, 0U},
    SourceGlyph{0U, 0U, 34U, 34U, 34U, 34U, 42U, 42U, 42U, 42U, 20U, 0U, 0U},
    SourceGlyph{0U, 0U, 34U, 34U, 20U, 20U, 8U, 20U, 20U, 34U, 34U, 0U, 0U},
    SourceGlyph{0U, 0U, 34U, 34U, 20U, 20U, 8U, 8U, 8U, 8U, 8U, 0U, 0U},
    SourceGlyph{0U, 0U, 62U, 2U, 4U, 4U, 8U, 16U, 16U, 32U, 62U, 0U, 0U},
    SourceGlyph{0U, 28U, 16U, 16U, 16U, 16U, 16U, 16U, 16U, 16U, 16U, 28U, 0U},
    SourceGlyph{0U, 0U, 32U, 32U, 16U, 16U, 8U, 4U, 4U, 2U, 2U, 0U, 0U},
    SourceGlyph{0U, 28U, 4U, 4U, 4U, 4U, 4U, 4U, 4U, 4U, 4U, 28U, 0U},
    SourceGlyph{0U, 0U, 8U, 20U, 34U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U},
    SourceGlyph{0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 62U, 0U},
    SourceGlyph{0U, 8U, 4U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U},
    SourceGlyph{0U, 0U, 0U, 0U, 0U, 28U, 2U, 30U, 34U, 38U, 26U, 0U, 0U},
    SourceGlyph{0U, 0U, 32U, 32U, 32U, 60U, 34U, 34U, 34U, 34U, 60U, 0U, 0U},
    SourceGlyph{0U, 0U, 0U, 0U, 0U, 28U, 34U, 32U, 32U, 34U, 28U, 0U, 0U},
    SourceGlyph{0U, 0U, 2U, 2U, 2U, 30U, 34U, 34U, 34U, 34U, 30U, 0U, 0U},
    SourceGlyph{0U, 0U, 0U, 0U, 0U, 28U, 34U, 62U, 32U, 34U, 28U, 0U, 0U},
    SourceGlyph{0U, 0U, 12U, 18U, 16U, 16U, 60U, 16U, 16U, 16U, 16U, 0U, 0U},
    SourceGlyph{0U, 0U, 0U, 0U, 0U, 28U, 34U, 34U, 34U, 30U, 2U, 34U, 28U},
    SourceGlyph{0U, 0U, 32U, 32U, 32U, 44U, 50U, 34U, 34U, 34U, 34U, 0U, 0U},
    SourceGlyph{0U, 0U, 0U, 8U, 0U, 24U, 8U, 8U, 8U, 8U, 28U, 0U, 0U},
    SourceGlyph{0U, 0U, 0U, 4U, 0U, 12U, 4U, 4U, 4U, 4U, 36U, 36U, 24U},
    SourceGlyph{0U, 0U, 32U, 32U, 32U, 36U, 40U, 48U, 40U, 36U, 34U, 0U, 0U},
    SourceGlyph{0U, 0U, 24U, 8U, 8U, 8U, 8U, 8U, 8U, 8U, 28U, 0U, 0U},
    SourceGlyph{0U, 0U, 0U, 0U, 0U, 52U, 42U, 42U, 42U, 42U, 34U, 0U, 0U},
    SourceGlyph{0U, 0U, 0U, 0U, 0U, 44U, 50U, 34U, 34U, 34U, 34U, 0U, 0U},
    SourceGlyph{0U, 0U, 0U, 0U, 0U, 28U, 34U, 34U, 34U, 34U, 28U, 0U, 0U},
    SourceGlyph{0U, 0U, 0U, 0U, 0U, 60U, 34U, 34U, 34U, 60U, 32U, 32U, 32U},
    SourceGlyph{0U, 0U, 0U, 0U, 0U, 30U, 34U, 34U, 34U, 30U, 2U, 2U, 2U},
    SourceGlyph{0U, 0U, 0U, 0U, 0U, 44U, 50U, 32U, 32U, 32U, 32U, 0U, 0U},
    SourceGlyph{0U, 0U, 0U, 0U, 0U, 28U, 34U, 24U, 4U, 34U, 28U, 0U, 0U},
    SourceGlyph{0U, 0U, 0U, 16U, 16U, 60U, 16U, 16U, 16U, 18U, 12U, 0U, 0U},
    SourceGlyph{0U, 0U, 0U, 0U, 0U, 34U, 34U, 34U, 34U, 38U, 26U, 0U, 0U},
    SourceGlyph{0U, 0U, 0U, 0U, 0U, 34U, 34U, 34U, 20U, 20U, 8U, 0U, 0U},
    SourceGlyph{0U, 0U, 0U, 0U, 0U, 34U, 34U, 42U, 42U, 42U, 20U, 0U, 0U},
    SourceGlyph{0U, 0U, 0U, 0U, 0U, 34U, 20U, 8U, 8U, 20U, 34U, 0U, 0U},
    SourceGlyph{0U, 0U, 0U, 0U, 0U, 34U, 34U, 34U, 38U, 26U, 2U, 34U, 28U},
    SourceGlyph{0U, 0U, 0U, 0U, 0U, 62U, 4U, 8U, 16U, 32U, 62U, 0U, 0U},
    SourceGlyph{0U, 6U, 8U, 8U, 8U, 8U, 48U, 8U, 8U, 8U, 8U, 6U, 0U},
    SourceGlyph{0U, 0U, 8U, 8U, 8U, 8U, 8U, 8U, 8U, 8U, 8U, 0U, 0U},
    SourceGlyph{0U, 48U, 8U, 8U, 8U, 8U, 6U, 8U, 8U, 8U, 8U, 48U, 0U},
    SourceGlyph{0U, 0U, 18U, 42U, 36U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U},
}};

[[nodiscard]] constexpr auto widen(u32 bits) -> u32 {
    u32 out = 0;
    for (u32 x = 0; x < kFontWidthPixels; ++x) {
        const u32 source_x = (x * 6U) / kFontWidthPixels;
        const u32 source_bit = (bits >> (5U - source_x)) & 1U;
        out |= source_bit << ((kFontWidthPixels - 1U) - x);
    }
    return out;
}

[[nodiscard]] constexpr auto makefont() -> GpuFont {
    GpuFont result = {};
    result.metrics = {kFontFirst, kFontCount, kFontWidthPixels, kFontHeightPixels};
    for (u32 glyph = 0; glyph < kFontCount; ++glyph) {
        result.glyphs[glyph].metrics = {kFontWidthPixels, kFontHeightPixels, kFontAscentPixels, 0U};
        for (u32 row = 0; row < kFontHeightPixels; ++row) {
            result.glyphs[glyph].rows[row] = widen(kSourceGlyphs[glyph][row]);
        }
    }
    return result;
}

constexpr GpuFont kFont = makefont();

} // namespace

const GpuFont &font() { return kFont; }

u32 fontrow(u32 code, u32 row) {
    if (code < kFontFirst || code >= kFontFirst + kFontCount || row >= kFontHeightPixels) {
        return 0;
    }
    return kFont.glyphs[code - kFontFirst].rows[row];
}

} // namespace mira
