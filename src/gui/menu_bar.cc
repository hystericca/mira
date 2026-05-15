#include "private.hpp"

namespace mira::gui {

void menulayout(GuiState *state) {
    f32 menu_x = 4.0F;
    for (const MenuItem item : kMenuItems) {
        const Rect rect = menurect(item.text, menu_x);
        addhit(state, rect, HitKind::kMenu, item.index, 100);
        menu_x += rect.width + 1.0F;
    }
    if (state->active_menu == 3) {
        for (u8 index = 0; index < static_cast<u8>(kLayerMenuCommandCount); ++index) {
            addhit(state, menucmdrect(index), HitKind::kMenuAction, index, 120);
        }
    }
}

b8 menumouse(GuiState *state, HitRecord hit) {
    if (hit.kind == HitKind::kMenu) {
        layerdone(state);
        state->active_menu = state->active_menu == hit.index ? kNoMenu : hit.index;
        return true;
    }

    if (hit.kind != HitKind::kMenuAction ||
        hit.index >= static_cast<u8>(kLayerMenuCommandCount)) {
        return false;
    }

    switch (kLayerMenuCommands[hit.index].action) {
    case MenuAction::kLayerNew:
        if (layeradd(state, {})) {
            (void)layeredit(state);
        }
        break;
    case MenuAction::kLayerDelete:
        (void)layerdel(state);
        break;
    case MenuAction::kLayerRename:
        (void)layeredit(state);
        break;
    }
    state->active_menu = kNoMenu;
    return true;
}

void menudraw(const GuiState &state, DrawList *draws) {
    drawrect(draws, state.layout.menu_bar, Tone::kBlack);
    drawstroke(draws,
                {.x = 0.0F,
                 .y = state.layout.menu_bar.height - 1.0F,
                 .width = state.layout.window.width,
                 .height = 1.0F},
                Tone::kWhite);

    f32 menu_x = 4.0F;
    for (const MenuItem item : kMenuItems) {
        const Rect rect = menurect(item.text, menu_x);
        const b8 active = state.active_menu == item.index;
        const b8 hot = state.hot_kind == HitKind::kMenu && state.hot_index == item.index;
        if (active || hot) {
            drawrect(draws,
                      {.x = rect.x, .y = rect.y + 1.0F, .width = rect.width, .height = 12.0F},
                      Tone::kWhite);
        }
        drawtext(draws, item.text, rect.x + 5.0F, 4.0F,
                  active || hot ? Tone::kBlack : Tone::kWhite);
        menu_x += rect.width + 1.0F;
    }

    if (state.active_menu != 3) {
        return;
    }

    const Rect menu = {
        .x = layermenux(),
        .y = 14.0F,
        .width = 45.0F,
        .height = static_cast<f32>(kLayerMenuCommandCount) * 13.0F,
    };
    drawrect(draws, menu, Tone::kBlack);
    drawstroke(draws, menu, Tone::kWhite);
    for (u8 index = 0; index < static_cast<u8>(kLayerMenuCommandCount); ++index) {
        const Rect item = menucmdrect(index);
        const b8 hot = state.hot_kind == HitKind::kMenuAction && state.hot_index == index;
        if (hot) {
            drawrect(draws, item, Tone::kWhite);
        }
        drawtext(draws, kLayerMenuCommands[index].text, item.x + 5.0F, item.y + 3.0F,
                  hot ? Tone::kBlack : Tone::kWhite);
    }
}

} // namespace mira::gui
