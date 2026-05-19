#include "mira/draw/draw.hpp"

#include <algorithm>

namespace mira {
namespace {

[[nodiscard]] Rect normalized(Rect rect) {
    const f32 x1 = rect.x + rect.width;
    const f32 y1 = rect.y + rect.height;
    const f32 x0 = std::min(rect.x, x1);
    const f32 y0 = std::min(rect.y, y1);
    return {
        .x = x0,
        .y = y0,
        .width = std::max(rect.x, x1) - x0,
        .height = std::max(rect.y, y1) - y0,
    };
}

[[nodiscard]] b8 add_rect_value(DrawList *list, Rect rect, u32 tone) {
    const Rect r = normalized(rect);
    if (r.width <= 0.0F || r.height <= 0.0F) {
        return true;
    }
    return list->rects.push({
        .x0 = r.x,
        .y0 = r.y,
        .x1 = r.x + r.width,
        .y1 = r.y + r.height,
        .tone = static_cast<f32>(tone),
    });
}

[[nodiscard]] usize plane_index(DrawPlane plane) {
    const usize index = static_cast<usize>(plane);
    return std::min(index, kDrawPlaneCount - 1U);
}

[[nodiscard]] DrawPlaneStart draw_tail(const DrawList &list) {
    return {
        .rect = list.rects.size(),
        .glyph = list.glyphs.size(),
        .icon = list.icons.size(),
        .guide_stamp = list.guide_stamps.size(),
    };
}

} // namespace

void DrawList::clear() {
    rects.clear();
    glyphs.clear();
    icons.clear();
    guide_stamps.clear();
    planes.fill({});
    plane = DrawPlane::kBase;
}

void DrawList::begin_plane(DrawPlane next) {
    const usize current = plane_index(plane);
    const usize target = plane_index(next);
    if (target <= current) {
        return;
    }

    const DrawPlaneStart tail = draw_tail(*this);
    for (usize index = current + 1U; index <= target; ++index) {
        planes[index] = tail;
    }
    plane = static_cast<DrawPlane>(target);
}

usize DrawList::upload_bytes() const {
    return rects.byte_size() + glyphs.byte_size() + icons.byte_size() + guide_stamps.byte_size();
}

u32 DrawList::overflow_count() const {
    u32 count = 0;
    count += rects.overflowed ? 1U : 0U;
    count += glyphs.overflowed ? 1U : 0U;
    count += icons.overflowed ? 1U : 0U;
    count += guide_stamps.overflowed ? 1U : 0U;
    return count;
}

usize DrawList::plane_count() const { return plane_index(plane) + 1U; }

DrawPlaneStart DrawList::plane_begin(DrawPlane draw_plane) const {
    return planes[plane_index(draw_plane)];
}

DrawPlaneStart DrawList::plane_end(DrawPlane draw_plane) const {
    const usize index = plane_index(draw_plane);
    if (index < plane_index(plane)) {
        return planes[index + 1U];
    }
    return draw_tail(*this);
}

Screen screen_for(i32 width, i32 height) {
    const i32 clamped_width = std::max(1, width);
    const i32 clamped_height = std::max(1, height);
    constexpr i32 kScale = 2;
    return {
        .scale = kScale,
        .width = std::max(1, (clamped_width + kScale - 1) / kScale),
        .height = std::max(1, (clamped_height + kScale - 1) / kScale),
    };
}

DrawView view(const DrawList &list) {
    return {
        .rects = list.rects.span(),
        .glyphs = list.glyphs.span(),
        .icons = list.icons.span(),
        .guide_stamps = list.guide_stamps.span(),
    };
}

u32 tone_value(Tone tone) { return static_cast<u32>(tone); }

b8 add_rect(DrawList *list, Rect rect, Tone tone) {
    return add_rect_value(list, rect, tone_value(tone));
}

b8 add_stroke(DrawList *list, Rect rect, Tone tone, f32 width) {
    const Rect r = normalized(rect);
    if (r.width <= 0.0F || r.height <= 0.0F) {
        return true;
    }

    const f32 thick = std::max(1.0F, width);
    const u32 value = tone_value(tone);
    if (r.width <= thick * 2.0F || r.height <= thick * 2.0F) {
        return add_rect_value(list, r, value);
    }

    b8 ok = true;
    ok = add_rect_value(list, {.x = r.x, .y = r.y, .width = r.width, .height = thick}, value) && ok;
    ok = add_rect_value(list,
                        {.x = r.x, .y = r.y + r.height - thick, .width = r.width, .height = thick},
                        value) &&
         ok;
    ok =
        add_rect_value(
            list, {.x = r.x, .y = r.y + thick, .width = thick, .height = r.height - (thick * 2.0F)},
            value) &&
        ok;
    ok = add_rect_value(list,
                        {.x = r.x + r.width - thick,
                         .y = r.y + thick,
                         .width = thick,
                         .height = r.height - (thick * 2.0F)},
                        value) &&
         ok;
    return ok;
}

b8 add_text(DrawList *list, std::string_view text, f32 x, f32 y, Tone tone, f32 scale) {
    const f32 clamped_scale = std::max(1.0F, scale);
    const u32 value = tone_value(tone);
    b8 ok = true;
    f32 cursor = x;
    for (const char c : text) {
        if (c != ' ') {
            ok = list->glyphs.push({
                     .x = cursor,
                     .y = y,
                     .scale = clamped_scale,
                     .code = static_cast<f32>(static_cast<u8>(c)),
                     .tone = static_cast<f32>(value),
                 }) &&
                 ok;
        }
        cursor += kFontWidth * clamped_scale;
    }
    return ok;
}

b8 add_icon(DrawList *list, Icon icon, f32 x, f32 y, Tone tone, f32 scale) {
    const f32 clamped_scale = std::max(0.125F, scale);
    return list->icons.push({
        .x = x,
        .y = y,
        .scale = clamped_scale,
        .code = static_cast<f32>(static_cast<u8>(icon)),
        .tone = static_cast<f32>(tone_value(tone)),
    });
}

b8 add_guide_stamp(DrawList *list, PaintStamp stamp) { return list->guide_stamps.push(stamp); }

} // namespace mira
