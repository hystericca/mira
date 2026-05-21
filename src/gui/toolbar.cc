#include "private.hpp"

namespace mira {
namespace impl = gui;

std::string_view toolname(const Tool &tool) { return impl::fixname(tool.name); }

const Tip *tipcur(const GuiState &state) {
    if (state.curtip >= state.tips.size()) {
        return nullptr;
    }
    return &state.tips[state.curtip];
}

const Size *sizecur(const GuiState &state) {
    if (state.cursize >= state.sizes.size()) {
        return nullptr;
    }
    return &state.sizes[state.cursize];
}

const Coverage *coveragecur(const GuiState &state) {
    if (state.curcoverage >= state.coverages.size()) {
        return nullptr;
    }
    return &state.coverages[state.curcoverage];
}

const Tool *toolcur(const GuiState &state) {
    if (state.curtool >= state.tools.size()) {
        return nullptr;
    }
    return &state.tools[state.curtool];
}

Icon tipicon(u8 index) {
    switch (index) {
    case 1:
        return Icon::kTipSquare;
    case 2:
        return Icon::kTipSlashR;
    case 3:
        return Icon::kTipSlashL;
    case 4:
        return Icon::kTipFlatH;
    case 5:
        return Icon::kTipFlatV;
    case 6:
        return Icon::kTipRake;
    case 7:
        return Icon::kTipScatter;
    case 0:
    default:
        return Icon::kTipRound;
    }
}

Icon sizeicon(u8 index) {
    switch (index) {
    case 1:
        return Icon::kSize2;
    case 2:
        return Icon::kSize3;
    case 3:
        return Icon::kSize4;
    case 4:
        return Icon::kSize5;
    case 5:
        return Icon::kSize6;
    case 6:
        return Icon::kSize7;
    case 7:
        return Icon::kSize8;
    case 0:
    default:
        return Icon::kSize1;
    }
}

Icon coverageicon(u8 index) {
    static constexpr std::array<Icon, kMaxCoverages> kIcons = {{
        Icon::kCoverage16,
        Icon::kCoverage15,
        Icon::kCoverage14,
        Icon::kCoverage13,
        Icon::kCoverage12,
        Icon::kCoverage11,
        Icon::kCoverage10,
        Icon::kCoverage9,
        Icon::kCoverage8,
        Icon::kCoverage7,
        Icon::kCoverage6,
        Icon::kCoverage5,
        Icon::kCoverage4,
        Icon::kCoverage3,
        Icon::kCoverage2,
        Icon::kCoverage1,
    }};
    if (index >= kIcons.size()) {
        return Icon::kCoverage16;
    }
    return kIcons[index];
}

} // namespace mira

namespace mira::gui {
namespace {

constexpr f32 kBrushPanelWidth = 166.0F;
constexpr f32 kBrushPanelHeight = 312.0F;
constexpr f32 kCoveragePanelHeight = 84.0F;
constexpr f32 kBrushHeaderHeight = 17.0F;
constexpr usize kCoverageColumns = 8;
constexpr f32 kCoverageLabelY = 52.0F;
constexpr f32 kCoverageLabelYNoSize = 24.0F;
constexpr f32 kCoverageStripY = 64.0F;
constexpr f32 kCoverageStripYNoSize = 36.0F;
constexpr f32 kCoverageSlotHeight = 15.0F;
constexpr f32 kTipLabelY = 102.0F;
constexpr f32 kTipRowY = 113.0F;
constexpr f32 kTipRowHeight = 24.0F;

[[nodiscard]] f32 smooth(f32 t) { return t * t * (3.0F - (2.0F * t)); }

[[nodiscard]] Rect brushbutton(const GuiState &state, ToolKind kind) {
    Rect rect = state.layout.brush_button;
    if (!tiptool(kind)) {
        rect.x = state.layout.tips.x;
    }
    rect.y = state.layout.tools.y;
    rect.height = 21.0F;
    return rect;
}

[[nodiscard]] f32 brushheight(const GuiState &state) {
    return tiptool(toolkind(state)) ? kBrushPanelHeight : kCoveragePanelHeight;
}

void placebrush(GuiState *state) {
    if (!state->brush_placed) {
        state->brush_x = state->layout.layers.x - kBrushPanelWidth - 10.0F;
        state->brush_y = state->layout.menu_bar.y + state->layout.menu_bar.height + 10.0F;
        state->brush_placed = true;
    }
}

void clampbrush(GuiState *state, f32 height) {
    state->brush_x = std::clamp(state->brush_x, 0.0F,
                                std::max(0.0F, state->layout.window.width - kBrushPanelWidth));
    state->brush_y =
        std::clamp(state->brush_y, 0.0F, std::max(0.0F, state->layout.window.height - height));
}

[[nodiscard]] Rect brushpanel(const GuiState &state) {
    return {.x = state.brush_x,
            .y = state.brush_y,
            .width = kBrushPanelWidth,
            .height = brushheight(state)};
}

[[nodiscard]] Rect brushtitle(const GuiState &state) {
    return {.x = state.layout.brush_panel.x + 1.0F,
            .y = state.layout.brush_panel.y + 1.0F,
            .width = std::max(1.0F, state.layout.brush_panel.width - 2.0F),
            .height = kBrushHeaderHeight};
}

[[nodiscard]] Rect brushclose(const GuiState &state) {
    return {.x = state.layout.brush_panel.x + state.layout.brush_panel.width - 15.0F,
            .y = state.layout.brush_panel.y + 4.0F,
            .width = 9.0F,
            .height = 9.0F};
}

[[nodiscard]] f32 coveragelabely(ToolKind kind) {
    return sizetool(kind) ? kCoverageLabelY : kCoverageLabelYNoSize;
}

[[nodiscard]] f32 coveragestripy(ToolKind kind) {
    return sizetool(kind) ? kCoverageStripY : kCoverageStripYNoSize;
}

[[nodiscard]] Rect tiprow(const GuiState &state, u8 index) {
    return {
        .x = state.layout.brush_panel.x + 1.0F,
        .y = state.layout.brush_panel.y + kTipRowY + (static_cast<f32>(index) * kTipRowHeight),
        .width = state.layout.brush_panel.width - 2.0F,
        .height = kTipRowHeight,
    };
}

[[nodiscard]] Rect panelsizehit(const GuiState &state, u8 index) {
    const f32 slot = (state.layout.brush_panel.width - 34.0F) / 8.0F;
    return {
        .x = state.layout.brush_panel.x + 17.0F + (static_cast<f32>(index) * slot),
        .y = state.layout.brush_panel.y + 36.0F,
        .width = slot,
        .height = 12.0F,
    };
}

[[nodiscard]] Rect coveragehit(const GuiState &state, u8 index) {
    const f32 slot = (state.layout.brush_panel.width - 14.0F) / static_cast<f32>(kCoverageColumns);
    const usize column = static_cast<usize>(index) % kCoverageColumns;
    const usize row = static_cast<usize>(index) / kCoverageColumns;
    return {
        .x = state.layout.brush_panel.x + 7.0F + (static_cast<f32>(column) * slot),
        .y = state.layout.brush_panel.y + coveragestripy(toolkind(state)) +
             (static_cast<f32>(row) * (kCoverageSlotHeight + 2.0F)),
        .width = slot,
        .height = kCoverageSlotHeight,
    };
}

[[nodiscard]] b8 brushvisible(const GuiState &state) {
    return state.brush_open || state.brush_t > 0.0F;
}

void closebrush(GuiState *state) {
    state->brush_open = false;
    state->moving_brush = false;
}

void openbrush(GuiState *state) {
    layerdone(state);
    state->active_menu = kNoMenu;
    state->context_open = false;
    placebrush(state);
    clampbrush(state, brushheight(*state));
    state->brush_open = !state->brush_open;
}

void drawsizevalue(const GuiState &state, DrawList *draws, f32 x, f32 y, Tone tone) {
    const std::array<char, 1> text = {static_cast<char>('1' + state.cursize)};
    drawtext(draws, std::string_view(text.data(), text.size()), x, y, tone);
}

void drawtipstamp(DrawList *draws, u8 tip, f32 center_x, f32 center_y, Tone ink, f32 scale) {
    const f32 clamped_scale = std::max(0.375F, scale);
    drawicon(draws, tipicon(tip), center_x - (4.0F * clamped_scale),
             center_y - (4.0F * clamped_scale), ink, clamped_scale);
}

[[nodiscard]] f32 previewwidth(u8 size) {
    return std::clamp(1.0F + (static_cast<f32>(size) * 0.72F), 1.0F, 6.0F);
}

void drawcleanstroke(DrawList *draws, Rect row, u8 size, Tone ink, b8 square) {
    static constexpr std::array<f32, 8> kWave = {-1.0F, -2.0F, -1.0F, 1.0F,
                                                 2.0F,  1.0F,  0.0F,  -1.0F};
    const f32 x0 = row.x + 48.0F;
    const f32 y0 = row.y + (row.height * 0.5F);
    const f32 step = 11.5F;
    const f32 thick = previewwidth(size);
    for (usize segment = 0; segment < kWave.size(); ++segment) {
        const b8 end = segment == 0 || segment + 1U == kWave.size();
        const f32 segment_thick = square || !end ? thick : std::max(1.0F, thick - 2.0F);
        drawrect(draws,
                 {.x = x0 + (static_cast<f32>(segment) * step),
                  .y = y0 + kWave[segment] - (segment_thick * 0.5F),
                  .width = step + 1.0F,
                  .height = segment_thick},
                 ink);
    }
}

void drawslashstroke(DrawList *draws, Rect row, b8 rising, Tone ink) {
    static constexpr std::array<f32, 8> kWave = {-1.0F, -2.0F, -1.0F, 1.0F,
                                                 2.0F,  1.0F,  0.0F,  -1.0F};
    const f32 x0 = row.x + 48.0F;
    const f32 y0 = row.y + (row.height * 0.5F);
    const f32 step = 11.5F;
    for (usize segment = 0; segment < kWave.size(); ++segment) {
        for (u8 line = 0; line < 3; ++line) {
            const f32 offset = rising ? static_cast<f32>(line) : static_cast<f32>(2U - line);
            drawrect(draws,
                     {.x = x0 + (static_cast<f32>(segment) * step) + offset,
                      .y = y0 + kWave[segment] + static_cast<f32>(line) - 2.0F,
                      .width = step,
                      .height = 1.0F},
                     ink);
        }
    }
}

void drawrakestroke(DrawList *draws, Rect row, Tone ink) {
    static constexpr std::array<f32, 8> kWave = {-1.0F, -2.0F, -1.0F, 1.0F,
                                                 2.0F,  1.0F,  0.0F,  -1.0F};
    const f32 x0 = row.x + 48.0F;
    const f32 y0 = row.y + (row.height * 0.5F);
    const f32 step = 11.5F;
    for (u8 line = 0; line < 4; ++line) {
        for (usize segment = 0; segment < kWave.size(); ++segment) {
            drawrect(draws,
                     {.x = x0 + (static_cast<f32>(segment) * step),
                      .y = y0 + kWave[segment] + static_cast<f32>(line) - 2.0F,
                      .width = step + 1.0F,
                      .height = 1.0F},
                     line == 0 ? Tone::kMid : ink);
        }
    }
}

void drawscatterstroke(DrawList *draws, Rect row, Tone ink) {
    static constexpr std::array<f32, 12> kDotsX = {49.0F, 58.0F,  65.0F,  73.0F,  82.0F,  89.0F,
                                                   98.0F, 106.0F, 114.0F, 121.0F, 130.0F, 139.0F};
    static constexpr std::array<f32, 12> kDotsY = {10.0F, 13.0F, 9.0F,  12.0F, 11.0F, 14.0F,
                                                   9.0F,  12.0F, 10.0F, 13.0F, 9.0F,  12.0F};
    for (usize index = 0; index < kDotsX.size(); ++index) {
        drawrect(draws,
                 {.x = row.x + kDotsX[index],
                  .y = row.y + kDotsY[index],
                  .width = index % 3U == 0U ? 2.0F : 1.0F,
                  .height = index % 3U == 0U ? 2.0F : 1.0F},
                 ink);
    }
}

void drawtippreview(DrawList *draws, Rect row, Brush spec) {
    if (spec.coverage != 0) {
        drawrect(draws,
                 {.x = row.x + 47.0F,
                  .y = row.y + 7.0F,
                  .width = std::max(1.0F, row.width - 57.0F),
                  .height = 10.0F},
                 Tone::kLight);
    }

    switch (spec.tip) {
    case 1:
        drawcleanstroke(draws, row, static_cast<u8>(std::max(0.0F, spec.diameter - 1.0F)),
                        spec.tone, true);
        break;
    case 2:
        drawslashstroke(draws, row, true, spec.tone);
        break;
    case 3:
        drawslashstroke(draws, row, false, spec.tone);
        break;
    case 4:
        drawcleanstroke(draws, row, 1, spec.tone, true);
        break;
    case 5:
        drawcleanstroke(draws, row, 7, spec.tone, true);
        break;
    case 6:
        drawrakestroke(draws, row, spec.tone);
        break;
    case 7:
        drawscatterstroke(draws, row, spec.tone);
        break;
    case 0:
    default:
        drawcleanstroke(draws, row, static_cast<u8>(std::max(0.0F, spec.diameter - 1.0F)),
                        spec.tone, false);
        break;
    }
}

} // namespace

void toollayout(GuiState *state) {
    addhit(state, state->layout.toolbar, HitKind::kToolbar, 0, 20);
    const ToolKind kind = toolkind(*state);
    if (!coveragetool(kind)) {
        closebrush(state);
    }
    if (coveragetool(kind)) {
        state->layout.brush_button = brushbutton(*state, kind);
        placebrush(state);
        clampbrush(state, brushheight(*state));
        state->layout.brush_panel = brushpanel(*state);
    } else {
        state->layout.brush_button = {};
        state->layout.brush_panel = {};
    }

    for (usize index = 0; index < state->tools.size(); ++index) {
        const f32 row_y = state->layout.tools.y + static_cast<f32>(index) * 24.0F;
        addhit(state,
               {.x = state->layout.tools.x,
                .y = row_y,
                .width = state->layout.tools.width,
                .height = 21.0F},
               HitKind::kTool, static_cast<u8>(index), 85);
    }

    if (tiptool(kind)) {
        for (usize index = 0; index < state->tips.size(); ++index) {
            const f32 row_y = state->layout.tips.y + static_cast<f32>(index) * 24.0F;
            addhit(state,
                   {.x = state->layout.tips.x,
                    .y = row_y,
                    .width = state->layout.tips.width,
                    .height = 21.0F},
                   HitKind::kTip, static_cast<u8>(index), 85);
        }
    }

    if (coveragetool(kind)) {
        addhit(state, state->layout.brush_button, HitKind::kBrushButton, 0, 88);
    }

    if (state->brush_open && coveragetool(kind)) {
        addhit(state, state->layout.brush_panel, HitKind::kBrushPanel, 0, 130);
        addhit(state, brushtitle(*state), HitKind::kBrushTitle, 0, 180);
        addhit(state, brushclose(*state), HitKind::kBrushClose, 0, 190);
        if (sizetool(kind)) {
            for (usize index = 0; index < state->sizes.size(); ++index) {
                addhit(state, panelsizehit(*state, static_cast<u8>(index)), HitKind::kSize,
                       static_cast<u8>(index), 175);
            }
        }
        for (usize index = 0; index < state->coverages.size(); ++index) {
            addhit(state, coveragehit(*state, static_cast<u8>(index)), HitKind::kCoverage,
                   static_cast<u8>(index), 170);
        }
        if (tiptool(kind)) {
            for (usize index = 0; index < state->tips.size(); ++index) {
                addhit(state, tiprow(*state, static_cast<u8>(index)), HitKind::kTip,
                       static_cast<u8>(index), 165);
            }
        }
    }
}

void tooltick(GuiState *state) {
    if (!coveragetool(toolkind(*state))) {
        closebrush(state);
    }
    const f32 target = state->brush_open ? 1.0F : 0.0F;
    if (state->brush_t < target) {
        state->brush_t = std::min(target, state->brush_t + 0.18F);
    } else if (state->brush_t > target) {
        state->brush_t = std::max(target, state->brush_t - 0.18F);
    }
}

b8 toolanimating(const GuiState &state) {
    return (state.brush_open && state.brush_t < 1.0F) ||
           (!state.brush_open && state.brush_t > 0.0F);
}

b8 toolkey(GuiState *state, Key key) {
    if (!brushvisible(*state) || key != Key::kEscape) {
        return false;
    }
    closebrush(state);
    return true;
}

b8 toolmouse(GuiState *state, Hit hit) {
    const ToolKind kind = toolkind(*state);
    if (hit.kind == HitKind::kTool) {
        layerdone(state);
        state->active_menu = kNoMenu;
        select_tool(state, hit.index);
        return true;
    }
    if (hit.kind == HitKind::kTip && tiptool(kind)) {
        layerdone(state);
        state->active_menu = kNoMenu;
        select_tip(state, hit.index);
        return true;
    }
    if (hit.kind == HitKind::kSize && sizetool(kind)) {
        layerdone(state);
        state->active_menu = kNoMenu;
        select_size(state, hit.index);
        return true;
    }
    if (hit.kind == HitKind::kBrushButton && coveragetool(kind)) {
        openbrush(state);
        return true;
    }
    if (hit.kind == HitKind::kBrushClose && state->brush_open) {
        closebrush(state);
        return true;
    }
    if (hit.kind == HitKind::kBrushTitle && state->brush_open) {
        state->moving_brush = true;
        state->brush_drag_x =
            static_cast<i16>(state->mouse_x - static_cast<i32>(state->layout.brush_panel.x));
        state->brush_drag_y =
            static_cast<i16>(state->mouse_y - static_cast<i32>(state->layout.brush_panel.y));
        return true;
    }
    if (hit.kind == HitKind::kBrushPanel && state->brush_open) {
        return true;
    }
    if (hit.kind == HitKind::kCoverage && coveragetool(kind)) {
        layerdone(state);
        state->active_menu = kNoMenu;
        select_coverage(state, hit.index);
        return true;
    }
    return false;
}

void toolmove(GuiState *state, i32 x, i32 y, u8 buttons) {
    if (!state->moving_brush) {
        return;
    }
    if ((buttons & 1U) == 0) {
        state->moving_brush = false;
        return;
    }
    state->brush_x = static_cast<f32>(x - state->brush_drag_x);
    state->brush_y = static_cast<f32>(y - state->brush_drag_y);
    clampbrush(state, brushheight(*state));
}

void toolup(GuiState *state) { state->moving_brush = false; }

void tooldraw(const GuiState &state, DrawList *draws) {
    const ToolKind kind = toolkind(state);
    const b8 show_tips = tiptool(kind);
    const b8 show_coverage = coveragetool(kind);

    drawrect(draws, state.layout.toolbar, Tone::kWhite);
    drawstroke(draws, state.layout.toolbar, Tone::kBlack);
    if (show_tips || show_coverage) {
        drawrect(draws,
                 {.x = state.layout.tips.x - 3.0F,
                  .y = state.layout.toolbar.y,
                  .width = 1.0F,
                  .height = state.layout.toolbar.height},
                 Tone::kBlack);
    }
    if (show_tips && show_coverage) {
        drawrect(draws,
                 {.x = state.layout.brush_button.x - 3.0F,
                  .y = state.layout.toolbar.y,
                  .width = 1.0F,
                  .height = state.layout.toolbar.height},
                 Tone::kBlack);
    }

    for (usize index = 0; index < state.tools.size(); ++index) {
        const Tool &tool = state.tools[index];
        const f32 row_y = state.layout.tools.y + static_cast<f32>(index) * 24.0F;
        const Rect row = {
            .x = state.layout.tools.x,
            .y = row_y,
            .width = state.layout.tools.width,
            .height = 21.0F,
        };
        const b8 hot_row = state.hot_kind == HitKind::kTool && state.hot_index == index;
        const b8 selected = tool.selected != 0;
        const b8 inverted = selected || hot_row;
        if (inverted) {
            drawrect(draws, row, Tone::kBlack);
        }
        const f32 icon_x = row.x + std::max(0.0F, (row.width - 12.0F) * 0.5F);
        drawicon(draws, toolicon(tool.kind), icon_x, row.y + 4.0F,
                 inverted ? Tone::kWhite : Tone::kBlack, 1.5F);
    }

    if (show_tips) {
        for (usize index = 0; index < state.tips.size(); ++index) {
            const Tip &tip = state.tips[index];
            const f32 row_y = state.layout.tips.y + static_cast<f32>(index) * 24.0F;
            const Rect row = {
                .x = state.layout.tips.x,
                .y = row_y,
                .width = state.layout.tips.width,
                .height = 21.0F,
            };
            const b8 hot_row = state.hot_kind == HitKind::kTip && state.hot_index == index;
            const b8 selected = tip.selected != 0;
            const b8 inverted = selected || hot_row;
            if (inverted) {
                drawrect(draws, row, Tone::kBlack);
            }
            drawtipstamp(draws, tip.index, row.x + (row.width * 0.5F), row.y + (row.height * 0.5F),
                         inverted ? Tone::kWhite : Tone::kBlack, 1.35F);
        }
    }

    if (show_coverage) {
        const b8 hot = state.hot_kind == HitKind::kBrushButton;
        const b8 active = brushvisible(state);
        if (hot || active) {
            drawrect(draws, state.layout.brush_button, active ? Tone::kBlack : Tone::kLight);
            if (hot && !active) {
                drawstroke(draws, state.layout.brush_button, Tone::kBlack);
            }
        }
        const Coverage *coverage = coveragecur(state);
        const u8 index = coverage == nullptr ? 0 : coverage->index;
        drawicon(draws, coverageicon(index), state.layout.brush_button.x + 4.0F,
                 state.layout.brush_button.y + 4.0F, active ? Tone::kWhite : Tone::kBlack, 1.5F);
    }
}

void toolpopupdraw(const GuiState &state, DrawList *draws) {
    const ToolKind kind = toolkind(state);
    if (!brushvisible(state) || !coveragetool(kind)) {
        return;
    }

    drawplane(draws, DrawPlane::kMenu);
    const f32 t = smooth(state.brush_t);
    const Rect panel = state.layout.brush_panel;
    drawrect(
        draws,
        {.x = panel.x + 2.0F, .y = panel.y + 2.0F, .width = panel.width, .height = panel.height},
        Tone::kMid);
    drawrect(draws, panel, Tone::kWhite);
    drawrect(draws, brushtitle(state), Tone::kBlack);
    drawstroke(draws, panel, Tone::kBlack);
    if (state.brush_t > 0.18F) {
        drawtext(draws, "brush", panel.x + 5.0F, panel.y + 4.0F, Tone::kWhite);
        const Rect close = brushclose(state);
        drawrect(draws, close,
                 state.hot_kind == HitKind::kBrushClose ? Tone::kLight : Tone::kWhite);
        drawstroke(draws, close, Tone::kWhite);
        drawrect(draws, {.x = close.x + 2.0F, .y = close.y + 4.0F, .width = 5.0F, .height = 1.0F},
                 Tone::kBlack);
        drawrect(draws, {.x = close.x + 4.0F, .y = close.y + 2.0F, .width = 1.0F, .height = 5.0F},
                 Tone::kBlack);
    }
    if (state.brush_t > 0.32F && sizetool(kind)) {
        drawtext(draws, "size", panel.x + 6.0F, panel.y + 22.0F, Tone::kBlack);
        const Rect field = {
            .x = panel.x + 74.0F,
            .y = panel.y + 19.0F,
            .width = 36.0F,
            .height = 15.0F,
        };
        drawrect(draws, field, Tone::kWhite);
        drawstroke(draws, field, Tone::kBlack);
        drawsizevalue(state, draws, field.x + 6.0F, field.y + 3.0F, Tone::kBlack);
        drawtext(draws, "px", field.x + 18.0F, field.y + 3.0F, Tone::kBlack);
        const Rect track = {
            .x = panel.x + 17.0F,
            .y = panel.y + 41.0F,
            .width = panel.width - 34.0F,
            .height = 3.0F,
        };
        drawrect(draws, track, Tone::kMid);
        const f32 knob_x = track.x + (static_cast<f32>(state.cursize) * (track.width / 7.0F));
        drawrect(draws, {.x = knob_x - 3.0F, .y = track.y - 3.0F, .width = 7.0F, .height = 8.0F},
                 Tone::kBlack);
    }

    if (state.brush_t > 0.34F) {
        drawtext(draws, "coverage", panel.x + 6.0F, panel.y + coveragelabely(kind), Tone::kBlack);
    }

    if (state.brush_t > 0.36F) {
        for (usize index = 0; index < state.coverages.size(); ++index) {
            const Coverage &coverage = state.coverages[index];
            Rect hit = coveragehit(state, static_cast<u8>(index));
            hit.y -= (1.0F - t) * 4.0F;
            const b8 hot = state.hot_kind == HitKind::kCoverage && state.hot_index == index;
            const b8 selected = coverage.selected != 0;
            if (hot || selected) {
                drawrect(draws, hit, selected ? Tone::kBlack : Tone::kLight);
                if (hot && !selected) {
                    drawstroke(draws, hit, Tone::kBlack);
                }
            }
            drawicon(draws, coverageicon(coverage.index), hit.x + 3.0F, hit.y + 3.0F,
                     selected ? Tone::kWhite : Tone::kBlack, 1.25F);
        }
    }

    if (!tiptool(kind)) {
        return;
    }
    if (state.brush_t > 0.40F) {
        drawtext(draws, "tip", panel.x + 6.0F, panel.y + kTipLabelY, Tone::kBlack);
    }
    for (usize index = 0; index < state.tips.size(); ++index) {
        const f32 threshold = 0.28F + (static_cast<f32>(index) * 0.045F);
        if (state.brush_t < threshold) {
            continue;
        }
        const Tip &tip = state.tips[index];
        Rect row = tiprow(state, static_cast<u8>(index));
        row.y -= (1.0F - t) * 6.0F;
        const b8 hot = state.hot_kind == HitKind::kTip && state.hot_index == index;
        const b8 selected = tip.selected != 0;
        if (hot || selected) {
            drawrect(draws, row, selected ? Tone::kBlack : Tone::kLight);
            if (hot && !selected) {
                drawstroke(draws, row, Tone::kBlack);
            }
        }
        drawstroke(draws, {.x = row.x, .y = row.y, .width = row.width, .height = 1.0F}, Tone::kMid);
        const Tone ink = selected ? Tone::kWhite : Tone::kBlack;
        drawtipstamp(draws, tip.index, row.x + 16.0F, row.y + (row.height * 0.5F), ink, 1.25F);
        Brush spec = brushfor(state, kind);
        spec.tip = tip.index;
        spec.tone = ink;
        drawtippreview(draws, row, spec);
    }
}

} // namespace mira::gui
