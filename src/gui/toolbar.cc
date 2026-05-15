#include "private.hpp"

namespace mira {
namespace impl = gui;

std::string_view toolname(const Tool &tool) { return impl::fixname(tool.name); }

const Size *sizecur(const GuiState &state) {
    if (state.cursize >= state.sizes.size()) {
        return nullptr;
    }
    return &state.sizes[state.cursize];
}

const Tool *toolcur(const GuiState &state) {
    if (state.curtool >= state.tools.size()) {
        return nullptr;
    }
    return &state.tools[state.curtool];
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

Icon brushicon(u8 index) {
    switch (index) {
    case 1:
        return Icon::kBrushSize2;
    case 2:
        return Icon::kBrushSize3;
    case 3:
        return Icon::kBrushSize4;
    case 4:
        return Icon::kBrushSize5;
    case 5:
        return Icon::kBrushSize6;
    case 6:
        return Icon::kBrushSize7;
    case 7:
        return Icon::kBrushSize8;
    case 0:
    default:
        return Icon::kBrushSize1;
    }
}

} // namespace mira

namespace mira::gui {

void toollayout(GuiState *state) {
    addhit(state, state->layout.toolbar, HitKind::kToolbar, 0, 20);

    for (usize index = 0; index < state->tools.size(); ++index) {
        const f32 row_y = state->layout.tools.y + static_cast<f32>(index) * 18.0F;
        addhit(state,
                {.x = state->layout.tools.x,
                 .y = row_y,
                 .width = state->layout.tools.width,
                 .height = 15.0F},
                HitKind::kTool, static_cast<u8>(index), 85);
    }

    for (usize index = 0; index < state->sizes.size(); ++index) {
        const f32 row_y = state->layout.sizes.y + static_cast<f32>(index) * 18.0F;
        addhit(state,
                {.x = state->layout.sizes.x,
                 .y = row_y,
                 .width = state->layout.sizes.width,
                 .height = 15.0F},
                HitKind::kSize, static_cast<u8>(index), 85);
    }
}

b8 toolmouse(GuiState *state, HitRecord hit) {
    if (hit.kind == HitKind::kTool) {
        layerdone(state);
        state->active_menu = kNoMenu;
        select_tool(state, hit.index);
        return true;
    }
    if (hit.kind == HitKind::kSize) {
        layerdone(state);
        state->active_menu = kNoMenu;
        select_size(state, hit.index);
        return true;
    }
    return false;
}

void tooldraw(const GuiState &state, DrawList *draws) {
    drawrect(draws, state.layout.toolbar, Tone::kBlack);
    drawstroke(draws, state.layout.toolbar, Tone::kWhite);
    drawrect(draws,
              {.x = state.layout.sizes.x - 3.0F,
               .y = state.layout.toolbar.y,
               .width = 1.0F,
               .height = state.layout.toolbar.height},
              Tone::kWhite);

    for (usize index = 0; index < state.tools.size(); ++index) {
        const Tool &tool = state.tools[index];
        const f32 row_y = state.layout.tools.y + static_cast<f32>(index) * 18.0F;
        const Rect row = {
            .x = state.layout.tools.x,
            .y = row_y,
            .width = state.layout.tools.width,
            .height = 15.0F,
        };
        const b8 hot_row = state.hot_kind == HitKind::kTool && state.hot_index == index;
        const b8 selected = tool.selected != 0;
        const b8 inverted = selected || hot_row;
        if (inverted) {
            drawrect(draws, row, Tone::kWhite);
        }
        const f32 icon_x = row.x + std::max(0.0F, (row.width - 8.0F) * 0.5F);
        drawicon(draws, toolicon(tool.kind), icon_x, row.y + 4.0F,
                  inverted ? Tone::kBlack : Tone::kWhite);
    }

    for (usize index = 0; index < state.sizes.size(); ++index) {
        const Size &size = state.sizes[index];
        const f32 row_y = state.layout.sizes.y + static_cast<f32>(index) * 18.0F;
        const Rect row = {
            .x = state.layout.sizes.x,
            .y = row_y,
            .width = state.layout.sizes.width,
            .height = 15.0F,
        };
        const b8 hot_row = state.hot_kind == HitKind::kSize && state.hot_index == index;
        const b8 selected = size.selected != 0;
        const b8 inverted = selected || hot_row;
        if (inverted) {
            drawrect(draws, row, Tone::kWhite);
        }
        const f32 icon_x = row.x + std::max(0.0F, (row.width - 8.0F) * 0.5F);
        drawicon(draws, sizeicon(size.index), icon_x, row.y + 4.0F,
                  inverted ? Tone::kBlack : Tone::kWhite);
    }
}

} // namespace mira::gui
