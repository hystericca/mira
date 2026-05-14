#include "mira/draw/draw.hpp"

#include <algorithm>
#include <cmath>

namespace mira {
namespace {

[[nodiscard]] u32 pack_data1(DrawKind kind, u8 clip_index, u8 luma) {
    // one byte each for clip, kind, and luma keeps Draw at 32 bytes
    return ((static_cast<u32>(clip_index) & 0xFFU) << 8U) |
           ((static_cast<u32>(kind) & 0xFFU) << 16U) | (static_cast<u32>(luma) << 24U);
}

} // namespace

void DrawList::clear() {
    draws.clear();
    clips.clear();
    text.clear();
    samples.clear();
}

usize DrawList::upload_bytes() const {
    return draws.byte_size() + clips.byte_size() + text.byte_size() + samples.byte_size();
}

u32 DrawList::overflow_count() const {
    u32 count = 0;
    count += draws.overflowed ? 1U : 0U;
    count += clips.overflowed ? 1U : 0U;
    count += text.overflowed ? 1U : 0U;
    count += samples.overflowed ? 1U : 0U;
    return count;
}

Screen screen_for(i32 width, i32 height) {
    // Layout uses window pixels. Drawing uses this smaller screen.
    const i32 clamped_width = std::max(1, width);
    const i32 clamped_height = std::max(1, height);
    i32 scale = 1;
    if (clamped_width / 3 >= 560 && clamped_height / 3 >= 360) {
        scale = 3;
    } else if (clamped_width / 2 >= 560 && clamped_height / 2 >= 360) {
        scale = 2;
    }
    return {
        .scale = scale,
        .width = (clamped_width + scale - 1) / scale,
        .height = (clamped_height + scale - 1) / scale,
    };
}

DrawView view(const DrawList &list) {
    return {
        .draws = list.draws.span(),
        .clips = list.clips.span(),
        .text = list.text.span(),
        .samples = list.samples.span(),
    };
}

TextRange add_text(DrawList *list, std::string_view text) {
    const usize offset = list->text.size();
    const usize copied = std::min(text.size(), list->text.capacity() - offset);
    for (usize index = 0; index < copied; ++index) {
        (void)list->text.push(text[index]);
    }
    if (copied < text.size()) {
        list->text.overflowed = true;
    }
    return {
        .offset = static_cast<u32>(offset),
        .length = static_cast<u16>(std::min<usize>(copied, 65535U)),
    };
}

b8 add_clip(DrawList *list, Clip clip) { return list->clips.push(clip); }

b8 add_draw(DrawList *list, DrawKind kind, u8 clip_index, u8 luma, Rect rect, f32 p0, f32 p1,
            u32 data0) {
    return list->draws.push({
        .x0 = rect.x,
        .y0 = rect.y,
        .x1 = rect.x + rect.width,
        .y1 = rect.y + rect.height,
        .p0 = p0,
        .p1 = p1,
        .data0 = data0,
        .data1 = pack_data1(kind, clip_index, luma),
    });
}

void build_demo(DrawList *list, Screen screen) {
    list->clear();
    const f32 width = static_cast<f32>(screen.width);
    const f32 height = static_cast<f32>(screen.height);
    const f32 inset_x = std::min(8.0F, width * 0.5F);
    const f32 inset_y = std::min(8.0F, height * 0.5F);
    const f32 graph_x = std::min(18.0F, width * 0.25F);
    const f32 graph_y = std::max(50.0F, height * 0.46F);
    const f32 graph_width = std::max(1.0F, width - (graph_x * 2.0F));
    const f32 graph_height = std::max(16.0F, height - graph_y - inset_y - 10.0F);

    (void)add_clip(list, {.x0 = 0.0F, .y0 = 0.0F, .x1 = width, .y1 = height});
    (void)add_draw(list, DrawKind::kFill, 0, 0,
                   {.x = 0.0F, .y = 0.0F, .width = width, .height = height});
    (void)add_draw(list, DrawKind::kStroke, 0, 255,
                   {.x = inset_x,
                    .y = inset_y,
                    .width = std::max(0.0F, width - (inset_x * 2.0F)),
                    .height = std::max(0.0F, height - (inset_y * 2.0F))},
                   1.0F);

    for (u32 index = 0; index < 7; ++index) {
        const f32 x = inset_x + 16.0F + (static_cast<f32>(index) * 13.0F);
        const f32 shade = static_cast<f32>(index + 1U) * (255.0F / 8.0F);
        (void)add_draw(list, DrawKind::kFill, 0, static_cast<u8>(shade),
                       {.x = x, .y = inset_y + 36.0F, .width = 10.0F, .height = 18.0F});
    }

    (void)add_draw(list, DrawKind::kFill, 0, 142,
                   {.x = inset_x + 22.0F, .y = inset_y + 56.0F, .width = 56.0F, .height = 34.0F});
    (void)add_draw(list, DrawKind::kStroke, 0, 224,
                   {.x = inset_x + 92.0F, .y = inset_y + 56.0F, .width = 54.0F, .height = 34.0F},
                   1.4F);
    (void)add_draw(list, DrawKind::kDash, 0, 210,
                   {.x = inset_x + 24.0F,
                    .y = inset_y + 112.0F,
                    .width = std::max(20.0F, width * 0.34F),
                    .height = 28.0F},
                   1.5F, 9.0F);

    (void)add_draw(list, DrawKind::kFill, 0, 38,
                   {.x = graph_x, .y = graph_y, .width = graph_width, .height = graph_height});
    (void)add_draw(list, DrawKind::kStroke, 0, 199,
                   {.x = graph_x, .y = graph_y, .width = graph_width, .height = graph_height},
                   1.0F);

    const usize graph_offset = list->samples.size();
    const u32 graph_count = static_cast<u32>(
        std::min<usize>(kMaxSamples - graph_offset,
                        std::max<usize>(2U, static_cast<usize>(std::ceil(graph_width)))));
    const f32 graph_mid = graph_y + (graph_height * 0.52F);
    const f32 amplitude_a = graph_height * 0.24F;
    const f32 amplitude_b = graph_height * 0.10F;
    for (u32 sample = 0; sample < graph_count; ++sample) {
        const f32 x = static_cast<f32>(sample) / static_cast<f32>(graph_count - 1U);
        const f32 y =
            graph_mid + (std::sin(x * 13.0F) * amplitude_a) + (std::sin(x * 31.0F) * amplitude_b);
        (void)list->samples.push({.y = y, .flags = 0});
    }
    const u32 graph_data =
        (static_cast<u32>(graph_offset) & 0xFFFFU) | ((graph_count & 0xFFFFU) << 16U);
    (void)add_draw(list, DrawKind::kGraph, 0, 240,
                   {.x = graph_x, .y = graph_y, .width = graph_width, .height = graph_height}, 1.5F,
                   0.0F, graph_data);

    const TextRange text = add_text(list, "MIRA");
    const u32 data0 = (text.offset & 0xFFFFU) | (static_cast<u32>(text.length) << 16U);
    (void)add_draw(list, DrawKind::kText, 0, 255,
                   {.x = inset_x + 18.0F, .y = inset_y + 15.0F, .width = 24.0F, .height = 7.0F},
                   1.0F, 0.0F, data0);
}

} // namespace mira
