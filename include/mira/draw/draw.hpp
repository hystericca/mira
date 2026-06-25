#pragma once

#include <array>
#include <span>
#include <string_view>

#include "mira/base/table.hpp"
#include "mira/draw/font.hpp"
#include "mira/types.hpp"

namespace mira {

constexpr usize kMaxRects{2048};
constexpr usize kMaxGlyphs{2048};
constexpr usize kMaxIcons{8192};
constexpr usize kMaxPreviewStamps{32768};
constexpr f32 kFontWidth{static_cast<f32>(kFontWidthPixels)};
constexpr f32 kFontHeight{static_cast<f32>(kFontHeightPixels)};

enum class Tone : u8 {
    kBlack,
    kDark,
    kMid,
    kLight,
    kWhite,
};

enum class Icon : u8 {
    kPen,
    kBrush,
    kLine,
    kMagic,
    kRect,
    kZoom,
    kErase,
    kSize1,
    kSize2,
    kSize3,
    kSize4,
    kSize5,
    kSize6,
    kSize7,
    kSize8,
    kCoverage16,
    kCoverage15,
    kCoverage14,
    kCoverage13,
    kCoverage12,
    kCoverage11,
    kCoverage10,
    kCoverage9,
    kCoverage8,
    kCoverage7,
    kCoverage6,
    kCoverage5,
    kCoverage4,
    kCoverage3,
    kCoverage2,
    kCoverage1,
    kLockOpen,
    kLockClosed,
    kTipRound,
    kTipSquare,
    kTipSlashR,
    kTipSlashL,
    kTipFlatH,
    kTipFlatV,
    kTipRake,
    kTipScatter,
};

struct Rect {
    f32 x{0.0F};
    f32 y{0.0F};
    f32 width{0.0F};
    f32 height{0.0F};
};

struct Screen {
    i32 scale{1};
    i32 width{1};
    i32 height{1};
};

struct RectDraw {
    f32 x0{0.0F};
    f32 y0{0.0F};
    f32 x1{0.0F};
    f32 y1{0.0F};
    f32 tone{0.0F};
    f32 _pad0{0.0F};
    f32 _pad1{0.0F};
    f32 _pad2{0.0F};
};

struct GlyphDraw {
    f32 x{0.0F};
    f32 y{0.0F};
    f32 scale{1.0F};
    f32 _pad0{0.0F};
    f32 code{0.0F};
    f32 tone{0.0F};
    f32 _pad1{0.0F};
    f32 _pad2{0.0F};
};

struct IconDraw {
    f32 x{0.0F};
    f32 y{0.0F};
    f32 scale{1.0F};
    f32 _pad0{0.0F};
    f32 code{0.0F};
    f32 tone{0.0F};
    f32 _pad1{0.0F};
    f32 _pad2{0.0F};
};

struct PaintStamp {
    f32 x{0.0F};
    f32 y{0.0F};
    f32 diameter{1.0F};
    f32 tone{0.0F};
    f32 layer_slot{0.0F};
    f32 tip{0.0F};
    f32 coverage{0.0F};
    f32 _pad{0.0F};
};

static_assert(sizeof(RectDraw) == 32);
static_assert(sizeof(GlyphDraw) == 32);
static_assert(sizeof(IconDraw) == 32);
static_assert(sizeof(PaintStamp) == 32);

enum class DrawPlane : u8 {
    kBase,
    kPanel,
    kControl,
    kMenu,
    kModal,
    kCursor,
    kCount,
};

constexpr usize kDrawPlaneCount{static_cast<usize>(DrawPlane::kCount)};

struct DrawPlaneStart {
    usize rect{0};
    usize glyph{0};
    usize icon{0};
    usize preview_stamp{0};
};

struct DrawList {
    Table<RectDraw, kMaxRects> rects;
    Table<GlyphDraw, kMaxGlyphs> glyphs;
    Table<IconDraw, kMaxIcons> icons;
    Table<PaintStamp, kMaxPreviewStamps> preview_stamps;
    std::array<DrawPlaneStart, kDrawPlaneCount> planes = {};
    DrawPlane plane = DrawPlane::kBase;

    void clear();
    void begin_plane(DrawPlane next);
    [[nodiscard]] usize upload_bytes() const;
    [[nodiscard]] u32 overflow_count() const;
    [[nodiscard]] usize plane_count() const;
    [[nodiscard]] DrawPlaneStart plane_begin(DrawPlane draw_plane) const;
    [[nodiscard]] DrawPlaneStart plane_end(DrawPlane draw_plane) const;
};

struct DrawView {
    std::span<const RectDraw> rects;
    std::span<const GlyphDraw> glyphs;
    std::span<const IconDraw> icons;
    std::span<const PaintStamp> preview_stamps;
};

[[nodiscard]] Screen screen_for(i32 width, i32 height);
[[nodiscard]] DrawView view(const DrawList &list);
[[nodiscard]] u32 tone_value(Tone tone);
[[nodiscard]] b8 add_rect(DrawList *list, Rect rect, Tone tone);
[[nodiscard]] b8 add_stroke(DrawList *list, Rect rect, Tone tone, f32 width = 1.0F);
[[nodiscard]] b8 add_text(DrawList *list, std::string_view text, f32 x, f32 y, Tone tone,
                          f32 scale = 1.0F);
[[nodiscard]] b8 add_icon(DrawList *list, Icon icon, f32 x, f32 y, Tone tone, f32 scale = 1.0F);
[[nodiscard]] b8 add_preview_stamp(DrawList *list, PaintStamp stamp);

} // namespace mira
