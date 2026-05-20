#include "private.hpp"

namespace mira::gui {
namespace {

constexpr u8 kPrimaryButtonMask = 1U;
constexpr u8 kMiddleButtonMask = 4U;

void zoomat(GuiState *state, i32 x, i32 y, i32 wheel_delta) {
    if (!contains(state->layout.viewport, x, y) || wheel_delta == 0) {
        return;
    }
    const f32 document_x = screen_to_document_x(*state, static_cast<f32>(x));
    const f32 document_y = screen_to_document_y(*state, static_cast<f32>(y));
    const f32 factor = wheel_delta < 0 ? 1.125F : 1.0F / 1.125F;
    state->view.zoom = clamp_zoom(state->view.zoom * factor);
    state->view.x =
        document_x - ((static_cast<f32>(x) - state->layout.viewport.x) / state->view.zoom);
    state->view.y =
        document_y - ((static_cast<f32>(y) - state->layout.viewport.y) / state->view.zoom);
}

void scrollview(GuiState *state, i32 dx, i32 dy) {
    state->view.x += static_cast<f32>(dx) / state->view.zoom;
    state->view.y += static_cast<f32>(dy) / state->view.zoom;
}

void panview(GuiState *state, i32 dx, i32 dy) {
    state->view.x -= static_cast<f32>(dx) / state->view.zoom;
    state->view.y -= static_cast<f32>(dy) / state->view.zoom;
}

[[nodiscard]] Rect stamprect(PaintStamp stamp) {
    const f32 diameter = std::max(1.0F, stamp.diameter);
    const f32 offset = (diameter - 1.0F) * 0.5F;
    return {
        .x = stamp.x - offset,
        .y = stamp.y - offset,
        .width = diameter,
        .height = diameter,
    };
}

[[nodiscard]] Rect unionrect(Rect a, Rect b) {
    const f32 x0 = std::min(a.x, b.x);
    const f32 y0 = std::min(a.y, b.y);
    const f32 x1 = std::max(a.x + a.width, b.x + b.width);
    const f32 y1 = std::max(a.y + a.height, b.y + b.height);
    return {
        .x = x0,
        .y = y0,
        .width = x1 - x0,
        .height = y1 - y0,
    };
}

[[nodiscard]] Rect stampviewrect(const GuiState &state, f32 document_x, f32 document_y,
                                 Brush spec) {
    const f32 diameter = std::max(1.0F, spec.diameter);
    const f32 offset = (diameter - 1.0F) * 0.5F;
    return {
        .x = document_to_screen_x(state, document_x - offset),
        .y = document_to_screen_y(state, document_y - offset),
        .width = diameter * state.view.zoom,
        .height = diameter * state.view.zoom,
    };
}

[[nodiscard]] PaintStamp stampfor(f32 document_x, f32 document_y, Brush spec, f32 layer_slot) {
    return {
        .x = document_x,
        .y = document_y,
        .diameter = spec.diameter,
        .tone = static_cast<f32>(tone_value(spec.tone)),
        .layer_slot = layer_slot,
        .tip = static_cast<f32>(spec.tip),
        .coverage = static_cast<f32>(spec.coverage),
    };
}

template <typename Emit> [[nodiscard]] b8 emitrect(f32 x0, f32 y0, f32 x1, f32 y1, Emit emit) {
    const auto left = static_cast<i32>(std::floor(std::min(x0, x1)));
    const auto right = static_cast<i32>(std::floor(std::max(x0, x1)));
    const auto top = static_cast<i32>(std::floor(std::min(y0, y1)));
    const auto bottom = static_cast<i32>(std::floor(std::max(y0, y1)));

    for (i32 x = left; x <= right; ++x) {
        if (!emit(static_cast<f32>(x), static_cast<f32>(top))) {
            return false;
        }
        if (bottom != top && !emit(static_cast<f32>(x), static_cast<f32>(bottom))) {
            return false;
        }
    }
    for (i32 y = top + 1; y < bottom; ++y) {
        if (!emit(static_cast<f32>(left), static_cast<f32>(y))) {
            return false;
        }
        if (right != left && !emit(static_cast<f32>(right), static_cast<f32>(y))) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] b8 draftmark(GuiState *state, f32 document_x, f32 document_y, ToolKind kind) {
    if (!painttool(kind) || !in_document(*state, document_x, document_y)) {
        return true;
    }
    return state->draft_stamps.push(stampfor(document_x, document_y, brushfor(*state, kind),
                                             static_cast<f32>(state->draft_layer_slot)));
}

[[nodiscard]] b8 draftline(GuiState *state, f32 x0, f32 y0, f32 x1, f32 y1, ToolKind kind) {
    if (!draftmark(state, x0, y0, kind)) {
        return false;
    }
    const f32 dx = x1 - x0;
    const f32 dy = y1 - y0;
    const i32 steps = static_cast<i32>(std::max(f32abs(dx), f32abs(dy)));
    for (i32 step = 1; step <= steps; ++step) {
        const f32 t = static_cast<f32>(step) / static_cast<f32>(steps);
        if (!draftmark(state, x0 + (dx * t), y0 + (dy * t), kind)) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] b8 draftrect(GuiState *state, f32 x0, f32 y0, f32 x1, f32 y1, ToolKind kind) {
    return emitrect(x0, y0, x1, y1, [&](f32 x, f32 y) { return draftmark(state, x, y, kind); });
}

[[nodiscard]] b8 draftbuild(GuiState *state, f32 x, f32 y) {
    state->draft_stamps.clear();
    state->draft_x = x;
    state->draft_y = y;
    if (!state->draft_active || !drafttool(state->draft_kind)) {
        return true;
    }
    switch (tooldef(state->draft_kind).stroke) {
    case StrokeKind::kLine:
        return draftline(state, state->draft_start_x, state->draft_start_y, x, y,
                         state->draft_kind);
    case StrokeKind::kRect:
        return draftrect(state, state->draft_start_x, state->draft_start_y, x, y,
                         state->draft_kind);
    case StrokeKind::kNone:
    case StrokeKind::kFree:
        break;
    }
    return true;
}

void historycancel(GuiState *state, b8 paint_overflow, b8 history_overflow) {
    state->paint_delta.truncate(state->active_paint_first);
    state->history_stamps.truncate(state->active_stroke_first);
    state->active_stroke_count = 0;
    state->active_stroke_rect = {};
    state->recording_stroke = false;
    if (paint_overflow) {
        state->paint_delta.overflowed = true;
    }
    if (history_overflow) {
        state->history_stamps.overflowed = true;
    }
}

void draftclear(GuiState *state) {
    state->draft_stamps.clear();
    state->draft_active = false;
    state->draft_kind = ToolKind::kPen;
    state->draft_layer_slot = 0;
    state->draft_start_x = 0.0F;
    state->draft_start_y = 0.0F;
    state->draft_x = 0.0F;
    state->draft_y = 0.0F;
}

void draftfail(GuiState *state) {
    const b8 overflowed = state->draft_stamps.overflowed;
    draftclear(state);
    state->draft_stamps.overflowed = overflowed;
    state->painting = false;
    historycancel(state, false, false);
}

void draftstart(GuiState *state, const Layer &layer, ToolKind kind, f32 x, f32 y) {
    state->draft_active = true;
    state->draft_kind = kind;
    state->draft_layer_slot = layer.layer_slot;
    state->draft_start_x = x;
    state->draft_start_y = y;
    if (!draftbuild(state, x, y)) {
        draftfail(state);
    }
}

void draftupdate(GuiState *state, f32 x, f32 y) {
    if (!state->draft_active) {
        return;
    }
    if (!draftbuild(state, x, y)) {
        draftfail(state);
    }
}

[[nodiscard]] b8 commitstamp(GuiState *state, PaintStamp stamp) {
    if (state->recording_stroke &&
        state->history_stamps.size() >= state->history_stamps.capacity()) {
        state->history_stamps.overflowed = true;
        historycancel(state, false, true);
        return false;
    }
    if (!state->paint_delta.push(stamp)) {
        historycancel(state, true, false);
        return false;
    }
    historymark(state, stamp);
    return state->recording_stroke;
}

[[nodiscard]] b8 draftcommit(GuiState *state) {
    if (!state->draft_active) {
        return false;
    }
    for (const PaintStamp stamp : state->draft_stamps.span()) {
        if (!commitstamp(state, stamp)) {
            draftclear(state);
            return false;
        }
    }
    draftclear(state);
    return true;
}

void drawshapestamps(const GuiState &state, DrawList *draws) {
    if (!state.draft_active) {
        return;
    }
    for (const PaintStamp stamp : state.draft_stamps.span()) {
        (void)add_preview_stamp(draws, stamp);
    }
}

b8 mark(GuiState *state, f32 document_x, f32 document_y, ToolKind kind) {
    const Layer *layer = paintlayer(*state);
    if (!painttool(kind) || layer == nullptr) {
        return false;
    }
    if (!in_document(*state, document_x, document_y)) {
        return true;
    }
    const PaintStamp stamp = stampfor(document_x, document_y, brushfor(*state, kind),
                                      static_cast<f32>(layer->layer_slot));
    if (state->recording_stroke &&
        state->history_stamps.size() >= state->history_stamps.capacity()) {
        state->history_stamps.overflowed = true;
        historycancel(state, false, true);
        return false;
    }
    if (!state->paint_delta.push(stamp)) {
        historycancel(state, true, false);
        return false;
    }
    historymark(state, stamp);
    return true;
}

b8 markline(GuiState *state, f32 x0, f32 y0, f32 x1, f32 y1, ToolKind kind) {
    if (!painttool(kind)) {
        return false;
    }
    const f32 dx = x1 - x0;
    const f32 dy = y1 - y0;
    const i32 steps = static_cast<i32>(std::max(f32abs(dx), f32abs(dy)));
    if (steps == 0) {
        return true;
    }
    for (i32 step = 1; step <= steps; ++step) {
        const f32 t = static_cast<f32>(step) / static_cast<f32>(steps);
        if (!mark(state, x0 + (dx * t), y0 + (dy * t), kind)) {
            return false;
        }
    }
    return true;
}

} // namespace

void historyclear(GuiState *state) {
    state->strokes.clear();
    state->history_stamps.clear();
    draftclear(state);
    state->stroke_cursor = 0;
    state->active_paint_first = 0;
    state->active_stroke_first = 0;
    state->active_stroke_count = 0;
    state->active_stroke_layer = 0;
    state->active_stroke_rect = {};
    state->recording_stroke = false;
    state->replay_strokes = false;
}

void workcancel(GuiState *state) {
    if (state->recording_stroke) {
        historycancel(state, false, false);
    }
    draftclear(state);
    state->painting = false;
    state->panning = false;
}

b8 historystart(GuiState *state, const Layer &layer) {
    if (state->stroke_cursor < state->strokes.size()) {
        const usize stamp_size = state->strokes[state->stroke_cursor].first_stamp;
        state->strokes.truncate(state->stroke_cursor);
        state->history_stamps.truncate(stamp_size);
    }

    state->active_paint_first = static_cast<u32>(state->paint_delta.size());
    state->active_stroke_first = static_cast<u32>(state->history_stamps.size());
    state->active_stroke_count = 0;
    state->active_stroke_layer = layer.id;
    state->active_stroke_rect = {};
    state->recording_stroke = state->strokes.size() < state->strokes.capacity();
    if (!state->recording_stroke) {
        state->strokes.overflowed = true;
        return false;
    }
    return true;
}

void historymark(GuiState *state, PaintStamp stamp) {
    if (!state->recording_stroke) {
        return;
    }
    if (!state->history_stamps.push(stamp)) {
        historycancel(state, false, true);
        return;
    }

    const Rect bounds = stamprect(stamp);
    state->active_stroke_rect =
        state->active_stroke_count == 0 ? bounds : unionrect(state->active_stroke_rect, bounds);
    ++state->active_stroke_count;
}

void historyfinish(GuiState *state) {
    if (!state->recording_stroke) {
        state->recording_stroke = false;
        return;
    }
    if (state->active_stroke_count == 0) {
        state->recording_stroke = false;
        return;
    }
    const Stroke action = {
        .layer_id = state->active_stroke_layer,
        .first_stamp = state->active_stroke_first,
        .stamp_count = state->active_stroke_count,
        .affected = state->active_stroke_rect,
    };
    if (!state->strokes.push(action)) {
        historycancel(state, false, false);
        return;
    }
    state->stroke_cursor = static_cast<u32>(state->strokes.size());
    state->recording_stroke = false;
}

b8 historyundo(GuiState *state) {
    if (state->stroke_cursor == 0) {
        return false;
    }
    --state->stroke_cursor;
    state->replay_strokes = true;
    return true;
}

b8 historyredo(GuiState *state) {
    if (state->stroke_cursor >= state->strokes.size()) {
        return false;
    }
    ++state->stroke_cursor;
    state->replay_strokes = true;
    return true;
}

void historyreplay(GuiState *state) {
    if (!state->replay_strokes) {
        return;
    }

    for (const Stroke action : state->strokes.span()) {
        if (action.stamp_count == 0 || action.first_stamp >= state->history_stamps.size()) {
            continue;
        }
        const PaintStamp stamp = state->history_stamps[action.first_stamp];
        markclear(state, static_cast<u8>(stamp.layer_slot + 0.5F));
    }

    for (u8 slot = 0; slot < kMaxLayers; ++slot) {
        for (usize action_index = 0; action_index < state->stroke_cursor; ++action_index) {
            const Stroke action = state->strokes[action_index];
            for (u32 stamp_index = 0; stamp_index < action.stamp_count; ++stamp_index) {
                const usize index = static_cast<usize>(action.first_stamp) + stamp_index;
                if (index >= state->history_stamps.size()) {
                    break;
                }
                const PaintStamp stamp = state->history_stamps[index];
                if (static_cast<u8>(stamp.layer_slot + 0.5F) != slot) {
                    continue;
                }
                if (!state->paint_delta.push(stamp)) {
                    state->replay_strokes = false;
                    return;
                }
            }
        }
    }
    state->replay_strokes = false;
}

void worklayout(GuiState *state) {
    if (!state->view_initialized) {
        center_document(state);
    } else {
        update_document_rect(state);
    }
    addhit(state, state->layout.viewport, HitKind::kViewport, 0, 10);
}

b8 workwheel(GuiState *state, i32 x, i32 y, i32 dx, i32 dy, u8 mods, HitKind hit_kind) {
    if ((mods & kInputCtrl) != 0) {
        zoomat(state, x, y, dy);
        return true;
    }
    if (hit_kind == HitKind::kViewport) {
        const b8 shift_scroll = (mods & kInputShift) != 0 && dx == 0;
        scrollview(state, shift_scroll ? dy : dx, shift_scroll ? 0 : dy);
        return true;
    }
    return false;
}

b8 workmouse(GuiState *state, HitRecord hit, i32 x, i32 y, u8 button) {
    if (button == 1 && hit.kind == HitKind::kViewport) {
        layerdone(state);
        state->active_menu = kNoMenu;
        state->context_open = false;
        state->panning = true;
        state->last_pan_x = static_cast<i16>(x);
        state->last_pan_y = static_cast<i16>(y);
        return true;
    }
    if (hit.kind != HitKind::kViewport || button != 0) {
        return false;
    }

    layerdone(state);
    state->active_menu = kNoMenu;
    state->context_open = false;
    const ToolKind kind = toolkind(*state);
    if (tooldef(kind).action == ToolAction::kZoom) {
        zoomat(state, x, y, -1);
        return true;
    }

    const Layer *layer = paintlayer(*state);
    if (painttool(kind) && layer != nullptr) {
        const f32 document_x = screen_to_paint_x(*state, x);
        const f32 document_y = screen_to_paint_y(*state, y);
        if (in_document(*state, document_x, document_y)) {
            if (!historystart(state, *layer)) {
                return true;
            }
            state->painting = true;
            state->last_paint_x = document_x;
            state->last_paint_y = document_y;
            if (drafttool(kind)) {
                draftstart(state, *layer, kind, document_x, document_y);
            } else if (freehandtool(kind)) {
                if (!mark(state, document_x, document_y, kind)) {
                    state->painting = false;
                }
            }
        }
    }
    return true;
}

void workmove(GuiState *state, i32 x, i32 y, u8 buttons) {
    if (state->panning) {
        if ((buttons & kMiddleButtonMask) == 0) {
            state->panning = false;
            return;
        }
        panview(state, x - state->last_pan_x, y - state->last_pan_y);
        state->last_pan_x = static_cast<i16>(x);
        state->last_pan_y = static_cast<i16>(y);
        return;
    }
    if (!state->painting) {
        return;
    }
    if ((buttons & kPrimaryButtonMask) == 0) {
        workup(state, x, y);
        state->painting = false;
        return;
    }
    if (!contains(state->layout.viewport, x, y)) {
        return;
    }
    const ToolKind kind = toolkind(*state);
    if (drafttool(kind)) {
        const f32 document_x = screen_to_paint_x(*state, x);
        const f32 document_y = screen_to_paint_y(*state, y);
        draftupdate(state, document_x, document_y);
        return;
    }
    if (!freehandtool(kind)) {
        return;
    }
    const f32 document_x = screen_to_paint_x(*state, x);
    const f32 document_y = screen_to_paint_y(*state, y);
    if (!markline(state, state->last_paint_x, state->last_paint_y, document_x, document_y, kind)) {
        state->painting = false;
        return;
    }
    state->last_paint_x = document_x;
    state->last_paint_y = document_y;
}

void workup(GuiState *state, i32 x, i32 y) {
    if (!state->painting) {
        return;
    }
    const ToolKind kind = toolkind(*state);
    if (drafttool(kind)) {
        if (contains(state->layout.viewport, x, y)) {
            const f32 document_x = screen_to_paint_x(*state, x);
            const f32 document_y = screen_to_paint_y(*state, y);
            draftupdate(state, document_x, document_y);
        }
        if (draftcommit(state)) {
            historyfinish(state);
        }
        return;
    }
    if (contains(state->layout.viewport, x, y)) {
        const f32 document_x = screen_to_paint_x(*state, x);
        const f32 document_y = screen_to_paint_y(*state, y);
        (void)markline(state, state->last_paint_x, state->last_paint_y, document_x, document_y,
                       kind);
    }
    historyfinish(state);
}

void workdraw(const GuiState &state, DrawList *draws) {
    const ToolKind active_tool = toolkind(state);
    if (state.hot_kind == HitKind::kViewport && painttool(active_tool) &&
        !(state.painting && drafttool(active_tool))) {
        const f32 document_x = screen_to_paint_x(state, state.mouse_x);
        const f32 document_y = screen_to_paint_y(state, state.mouse_y);
        const Brush spec = brushfor(state, active_tool);
        const Rect preview = stampviewrect(state, document_x, document_y, spec);
        const Layer *layer = paintlayer(state);
        if (layer != nullptr && in_document(state, document_x, document_y) &&
            !empty(intersectrect(state.layout.viewport, preview)) &&
            !empty(intersectrect(state.layout.document, preview))) {
            (void)add_preview_stamp(
                draws, stampfor(document_x, document_y, spec, static_cast<f32>(layer->layer_slot)));
        }
    }

    const Rect visible_document = intersectrect(state.layout.viewport, state.layout.document);
    if (!empty(visible_document)) {
        drawrect(draws,
                 {.x = state.layout.viewport.x,
                  .y = state.layout.viewport.y,
                  .width = state.layout.viewport.width,
                  .height = visible_document.y - state.layout.viewport.y},
                 Tone::kLight);
        drawrect(draws,
                 {.x = state.layout.viewport.x,
                  .y = visible_document.y + visible_document.height,
                  .width = state.layout.viewport.width,
                  .height = (state.layout.viewport.y + state.layout.viewport.height) -
                            (visible_document.y + visible_document.height)},
                 Tone::kLight);
        drawrect(draws,
                 {.x = state.layout.viewport.x,
                  .y = visible_document.y,
                  .width = visible_document.x - state.layout.viewport.x,
                  .height = visible_document.height},
                 Tone::kLight);
        drawrect(draws,
                 {.x = visible_document.x + visible_document.width,
                  .y = visible_document.y,
                  .width = (state.layout.viewport.x + state.layout.viewport.width) -
                           (visible_document.x + visible_document.width),
                  .height = visible_document.height},
                 Tone::kLight);
    } else {
        drawrect(draws, state.layout.viewport, Tone::kLight);
    }
    drawstroke(draws, state.layout.viewport, Tone::kBlack);
    const Rect tag = {
        .x = state.layout.viewport.x + 1.0F,
        .y = state.layout.viewport.y + 1.0F,
        .width = std::max(1.0F, state.layout.viewport.width - 2.0F),
        .height = 21.0F,
    };
    drawrect(draws, tag, Tone::kWhite);
    drawrect(draws,
             {.x = state.layout.viewport.x,
              .y = state.layout.viewport.y + 22.0F,
              .width = state.layout.viewport.width,
              .height = 1.0F},
             Tone::kBlack);
    drawtext(draws, "document", state.layout.viewport.x + 6.0F, state.layout.viewport.y + 5.0F,
             Tone::kBlack);
    if (!empty(visible_document)) {
        drawstroke(draws, visible_document, Tone::kBlack);
    }
    drawshapestamps(state, draws);
}

} // namespace mira::gui
