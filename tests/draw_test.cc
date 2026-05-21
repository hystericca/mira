#include "mira/draw/draw.hpp"
#include "mira/gui/gui.hpp"
#include "test_support.hpp"

#include <algorithm>
#include <array>
#include <string_view>
#include <type_traits>

auto main() -> int {
    const auto close = [](mira::f32 a, mira::f32 b) -> bool {
        const mira::f32 delta = a - b;
        return delta > -0.01F && delta < 0.01F;
    };
    const auto same_stamp = [&](const mira::PaintStamp &a, const mira::PaintStamp &b) -> bool {
        return close(a.x, b.x) && close(a.y, b.y) && close(a.diameter, b.diameter) &&
               close(a.tone, b.tone) && close(a.layer_slot, b.layer_slot) && close(a.tip, b.tip) &&
               close(a.coverage, b.coverage);
    };

    static_assert(sizeof(mira::RectDraw) == 32);
    static_assert(sizeof(mira::GlyphDraw) == 32);
    static_assert(sizeof(mira::IconDraw) == 32);
    static_assert(sizeof(mira::FontGlyph) == 80);
    static_assert(sizeof(mira::Font) == 7616);
    static_assert(sizeof(mira::ToolDef) == 9);
    static_assert(sizeof(mira::Layer) == 24);
    static_assert(sizeof(mira::Tool) == 16);
    static_assert(sizeof(mira::Tip) == 4);
    static_assert(sizeof(mira::Size) == 4);
    static_assert(sizeof(mira::Coverage) == 4);
    static_assert(sizeof(mira::PaintStamp) == 32);
    static_assert(sizeof(mira::Brush) == 8);
    static_assert(sizeof(mira::Stroke) == 28);
    static_assert(sizeof(mira::GuiAction) == 4);
    static_assert(sizeof(mira::Document) == 8);
    static_assert(sizeof(mira::View) == 12);
    static_assert(sizeof(mira::InputEvent) == 20);
    static_assert(sizeof(mira::Hit) == 24);
    static_assert(!std::is_copy_constructible_v<mira::DrawList>);
    static_assert(!std::is_copy_constructible_v<mira::GuiState>);

    const mira::Screen small = mira::screen_for(900, 600);
    const mira::Screen normal = mira::screen_for(1120, 720);
    const mira::Screen large = mira::screen_for(1800, 1200);
    MIRA_TEST(small.scale == 2);
    MIRA_TEST(small.width == 450);
    MIRA_TEST(small.height == 300);
    MIRA_TEST(normal.scale == 2);
    MIRA_TEST(normal.width == 560);
    MIRA_TEST(normal.height == 360);
    MIRA_TEST(large.scale == 2);
    MIRA_TEST(large.width == 900);
    MIRA_TEST(large.height == 600);

    mira::Table<mira::i32, 2> table;
    MIRA_TEST(table.push(1));
    MIRA_TEST(table.push(2));
    MIRA_TEST(!table.push(3));
    MIRA_TEST(table.overflowed);
    MIRA_TEST(table.size() == 2);
    table.clear();
    MIRA_TEST(table.empty());
    MIRA_TEST(!table.overflowed);
    MIRA_TEST(mira::font().metrics[0] == mira::kFontFirst);
    MIRA_TEST(mira::font().metrics[1] == mira::kFontCount);
    MIRA_TEST(mira::font().metrics[2] == mira::kFontWidthPixels);
    MIRA_TEST(mira::font().metrics[3] == mira::kFontHeightPixels);
    MIRA_TEST(mira::fontrow('A', 2) != 0);
    MIRA_TEST(mira::fontrow(0, 0) == 0);
    mira::Table<mira::i32, 3> edit_table;
    MIRA_TEST(edit_table.push(1));
    MIRA_TEST(edit_table.push(3));
    MIRA_TEST(edit_table.insert(1, 2));
    MIRA_TEST(edit_table[0] == 1);
    MIRA_TEST(edit_table[1] == 2);
    MIRA_TEST(edit_table[2] == 3);
    MIRA_TEST(!edit_table.insert(0, 0));
    MIRA_TEST(edit_table.overflowed);
    edit_table.truncate(1);
    MIRA_TEST(edit_table.size() == 1);
    MIRA_TEST(!edit_table.overflowed);
    MIRA_TEST(edit_table[0] == 1);
    MIRA_TEST(edit_table.push(4));
    MIRA_TEST(edit_table[1] == 4);
    MIRA_TEST(!edit_table.erase(4));
    MIRA_TEST(edit_table.erase(0));
    MIRA_TEST(edit_table[0] == 4);

    static mira::DrawList packing;
    MIRA_TEST(mira::add_rect(&packing, {.x = 1.0F, .y = 2.0F, .width = 3.0F, .height = 4.0F},
                             mira::Tone::kWhite));
    MIRA_TEST(packing.rects[0].x0 == 1.0F);
    MIRA_TEST(packing.rects[0].y0 == 2.0F);
    MIRA_TEST(packing.rects[0].x1 == 4.0F);
    MIRA_TEST(packing.rects[0].y1 == 6.0F);
    MIRA_TEST(packing.rects[0].tone == mira::tone_value(mira::Tone::kWhite));
    MIRA_TEST(mira::add_text(&packing, "A", 5.0F, 6.0F, mira::Tone::kWhite));
    MIRA_TEST(packing.glyphs[0].x == 5.0F);
    MIRA_TEST(packing.glyphs[0].y == 6.0F);
    MIRA_TEST(packing.glyphs[0].code == 'A');
    MIRA_TEST(packing.glyphs[0].tone == mira::tone_value(mira::Tone::kWhite));
    MIRA_TEST(mira::add_text(&packing, "AB", 1.0F, 2.0F, mira::Tone::kBlack));
    MIRA_TEST(packing.glyphs[2].x == 1.0F + mira::kFontWidth);
    MIRA_TEST(packing.glyphs[2].code == 'B');
    MIRA_TEST(mira::add_icon(&packing, mira::Icon::kPen, 7.0F, 8.0F, mira::Tone::kBlack));
    MIRA_TEST(packing.icons[0].x == 7.0F);
    MIRA_TEST(packing.icons[0].y == 8.0F);
    MIRA_TEST(packing.icons[0].code == 0.0F);
    MIRA_TEST(packing.icons[0].tone == mira::tone_value(mira::Tone::kBlack));
    MIRA_TEST(mira::sizeicon(7) == mira::Icon::kSize8);
    MIRA_TEST(mira::tipicon(7) == mira::Icon::kTipScatter);
    MIRA_TEST(mira::coverageicon(0) == mira::Icon::kCoverage16);
    MIRA_TEST(mira::coverageicon(7) == mira::Icon::kCoverage9);
    MIRA_TEST(mira::coverageicon(15) == mira::Icon::kCoverage1);
    MIRA_TEST(mira::kToolDefs.size() == 7);
    MIRA_TEST(mira::toolindex(mira::ToolKind::kErase) == 6);
    const mira::ToolDef pen = mira::tooldef(mira::ToolKind::kPen);
    MIRA_TEST(pen.ink == mira::InkKind::kBlack);
    MIRA_TEST(pen.stroke == mira::StrokeKind::kFree);
    MIRA_TEST(pen.trace == mira::TraceKind::kAccumulate);
    MIRA_TEST(!mira::tooluses(pen, mira::kToolUsesSize));
    const mira::ToolDef brush = mira::tooldef(mira::ToolKind::kBrush);
    MIRA_TEST(mira::toolfreehand(brush));
    MIRA_TEST(mira::tooluses(brush, mira::kToolUsesSize | mira::kToolUsesTip |
                                        mira::kToolUsesCoverage));
    const mira::ToolDef line = mira::tooldef(mira::ToolKind::kLine);
    MIRA_TEST(line.stroke == mira::StrokeKind::kLine);
    MIRA_TEST(mira::tooldraft(line));
    const mira::ToolDef magic = mira::tooldef(mira::ToolKind::kMagic);
    MIRA_TEST(magic.ink == mira::InkKind::kWhite);
    MIRA_TEST(magic.size == 7);
    MIRA_TEST(magic.tip == 7);
    const mira::ToolDef zoom = mira::tooldef(mira::ToolKind::kZoom);
    MIRA_TEST(!mira::toolpaints(zoom));
    MIRA_TEST(zoom.action == mira::ToolAction::kZoom);
    MIRA_TEST(mira::add_icon(&packing, mira::Icon::kSize8, 9.0F, 10.0F, mira::Tone::kBlack,
                             0.25F));
    MIRA_TEST(packing.icons[1].scale == 0.25F);
    MIRA_TEST(mira::add_icon(&packing, mira::Icon::kLockClosed, 11.0F, 12.0F, mira::Tone::kWhite));
    MIRA_TEST(packing.icons[2].code ==
              static_cast<mira::f32>(static_cast<mira::u8>(mira::Icon::kLockClosed)));
    packing.begin_plane(mira::DrawPlane::kMenu);
    const mira::DrawPlaneStart menu_plane = packing.plane_begin(mira::DrawPlane::kMenu);
    MIRA_TEST(packing.plane_count() == static_cast<mira::usize>(mira::DrawPlane::kMenu) + 1U);
    MIRA_TEST(menu_plane.rect == 1);
    MIRA_TEST(menu_plane.glyph == 3);
    MIRA_TEST(menu_plane.icon == 3);
    MIRA_TEST(mira::add_rect(&packing, {.x = 0.0F, .y = 0.0F, .width = 1.0F, .height = 1.0F},
                             mira::Tone::kBlack));
    MIRA_TEST(packing.plane_end(mira::DrawPlane::kMenu).rect == menu_plane.rect + 1);

    static mira::GuiState gui;
    mira::guiinit(&gui);
    MIRA_TEST(gui.tools.size() == 7);
    for (mira::usize index = 0; index < gui.tools.size(); ++index) {
        MIRA_TEST(gui.tools[index].kind == mira::kToolDefs[index].kind);
    }
    MIRA_TEST(mira::toolname(gui.tools[0]) == "pen");
    MIRA_TEST(mira::toolname(gui.tools[1]) == "brush");
    MIRA_TEST(mira::toolname(gui.tools[6]) == "erase");
    MIRA_TEST(gui.tools[0].selected == 1);
    MIRA_TEST(mira::toolcur(gui) == &gui.tools[0]);
    MIRA_TEST(gui.tips.size() == 8);
    MIRA_TEST(gui.tips[3].selected == 1);
    MIRA_TEST(mira::tipcur(gui) == &gui.tips[3]);
    MIRA_TEST(gui.sizes.size() == 8);
    MIRA_TEST(gui.sizes[3].selected == 1);
    MIRA_TEST(mira::sizecur(gui) == &gui.sizes[3]);
    MIRA_TEST(gui.coverages.size() == mira::kMaxCoverages);
    MIRA_TEST(gui.coverages[0].selected == 1);
    MIRA_TEST(mira::coveragecur(gui) == &gui.coverages[0]);
    MIRA_TEST(gui.document.width == mira::kDefaultDocumentWidth);
    MIRA_TEST(gui.document.height == mira::kDefaultDocumentHeight);
    MIRA_TEST(gui.layers.size() == 2);
    MIRA_TEST(mira::layername(gui.layers[0]) == "ink");
    MIRA_TEST(mira::layername(gui.layers[1]) == "background");
    MIRA_TEST(mira::layerselected(gui.layers[0]));
    MIRA_TEST(!mira::layerlocked(gui.layers[0]));
    MIRA_TEST(mira::layerlocked(gui.layers[1]));
    MIRA_TEST(gui.layers[0].layer_slot == 0);
    MIRA_TEST(gui.layers[1].layer_slot == mira::kBackgroundTextureSlot);
    MIRA_TEST(gui.layers[1].opacity_u8 == 255);
    MIRA_TEST(gui.next_layer_id == 3);
    MIRA_TEST(mira::layercur(gui) == &gui.layers[0]);
    MIRA_TEST(gui.strokes.empty());
    MIRA_TEST(gui.history_stamps.empty());
    MIRA_TEST(gui.stroke_cursor == 0);

    mira::guilayout(&gui, normal);
    MIRA_TEST(gui.layout.window.x == 0.0F);
    MIRA_TEST(gui.layout.window.y == 0.0F);
    MIRA_TEST(gui.layout.window.width == 560.0F);
    MIRA_TEST(gui.layout.window.height == 360.0F);
    MIRA_TEST(gui.layout.menu_bar.x == 0.0F);
    MIRA_TEST(gui.layout.menu_bar.y == 0.0F);
    MIRA_TEST(gui.layout.menu_bar.width == 560.0F);
    MIRA_TEST(gui.layout.menu_bar.height == 18.0F);
    MIRA_TEST(gui.layout.toolbar.x == 0.0F);
    MIRA_TEST(gui.layout.toolbar.width == 84.0F);
    MIRA_TEST(gui.layout.layers.x == 420.0F);
    MIRA_TEST(gui.layout.layers.width == 140.0F);
    MIRA_TEST(gui.layout.viewport.x == 84.0F);
    MIRA_TEST(gui.layout.viewport.y == 18.0F);
    MIRA_TEST(gui.layout.viewport.width == 336.0F);
    MIRA_TEST(gui.layout.document.x == -88.0F);
    MIRA_TEST(gui.layout.document.y == 5.0F);
    MIRA_TEST(gui.layout.document.width == 680.0F);
    MIRA_TEST(gui.layout.document.height == 368.0F);
    MIRA_TEST(close(gui.view.x, 344.0F));
    MIRA_TEST(close(gui.view.y, 26.0F));

    static mira::GuiState nav_gui;
    mira::guiinit(&nav_gui);
    mira::guilayout(&nav_gui, normal);
    const mira::i32 nav_x = static_cast<mira::i32>(nav_gui.layout.viewport.x + 100.0F);
    const mira::i32 nav_y = static_cast<mira::i32>(nav_gui.layout.viewport.y + 60.0F);
    mira::InputEvent wheel_down = {};
    wheel_down.kind = mira::InputKind::kWheel;
    wheel_down.x = nav_x;
    wheel_down.y = nav_y;
    wheel_down.dy = 24;
    mira::guievent(&nav_gui, {&wheel_down, 1});
    MIRA_TEST(close(nav_gui.view.x, 344.0F));
    MIRA_TEST(close(nav_gui.view.y, 74.0F));

    mira::InputEvent wheel_side = wheel_down;
    wheel_side.mods = mira::kInputShift;
    wheel_side.dy = 16;
    mira::guievent(&nav_gui, {&wheel_side, 1});
    MIRA_TEST(close(nav_gui.view.x, 376.0F));
    MIRA_TEST(close(nav_gui.view.y, 74.0F));

    const mira::f32 document_x_before =
        nav_gui.view.x +
        ((static_cast<mira::f32>(nav_x) - nav_gui.layout.viewport.x) / nav_gui.view.zoom);
    const mira::f32 document_y_before =
        nav_gui.view.y +
        ((static_cast<mira::f32>(nav_y) - nav_gui.layout.viewport.y) / nav_gui.view.zoom);
    mira::InputEvent wheel_zoom = wheel_down;
    wheel_zoom.mods = mira::kInputCtrl;
    wheel_zoom.dy = -12;
    mira::guievent(&nav_gui, {&wheel_zoom, 1});
    MIRA_TEST(close(nav_gui.view.zoom, mira::kInitialViewZoom * 1.125F));
    const mira::f32 document_x_after =
        nav_gui.view.x +
        ((static_cast<mira::f32>(nav_x) - nav_gui.layout.viewport.x) / nav_gui.view.zoom);
    const mira::f32 document_y_after =
        nav_gui.view.y +
        ((static_cast<mira::f32>(nav_y) - nav_gui.layout.viewport.y) / nav_gui.view.zoom);
    MIRA_TEST(close(document_x_before, document_x_after));
    MIRA_TEST(close(document_y_before, document_y_after));

    const mira::f32 pan_x_before = nav_gui.view.x;
    const mira::f32 pan_y_before = nav_gui.view.y;
    mira::InputEvent pan_down = {};
    pan_down.kind = mira::InputKind::kMouseDown;
    pan_down.button = 1;
    pan_down.x = nav_x;
    pan_down.y = nav_y;
    mira::InputEvent pan_move = {};
    pan_move.kind = mira::InputKind::kMouseMove;
    pan_move.button = 1;
    pan_move.buttons = 4;
    pan_move.x = nav_x + 18;
    pan_move.y = nav_y - 9;
    mira::InputEvent pan_up = pan_move;
    pan_up.kind = mira::InputKind::kMouseUp;
    pan_up.button = 1;
    const mira::InputEvent pan_events[] = {pan_down, pan_move, pan_up};
    mira::guievent(&nav_gui, pan_events);
    MIRA_TEST(close(nav_gui.view.x, pan_x_before - (18.0F / nav_gui.view.zoom)));
    MIRA_TEST(close(nav_gui.view.y, pan_y_before + (9.0F / nav_gui.view.zoom)));
    MIRA_TEST(!nav_gui.panning);

    const mira::Hit menu_hit = mira::guihit(gui, 8, 5);
    MIRA_TEST(menu_hit.kind == mira::HitKind::kMenu);
    MIRA_TEST(menu_hit.index == 0);
    static mira::GuiState about_gui;
    mira::guiinit(&about_gui);
    mira::guilayout(&about_gui, normal);
    mira::InputEvent open_mira_menu = {};
    open_mira_menu.kind = mira::InputKind::kMouseDown;
    open_mira_menu.x = 8;
    open_mira_menu.y = 5;
    mira::guievent(&about_gui, {&open_mira_menu, 1});
    MIRA_TEST(about_gui.active_menu == 0);
    mira::guilayout(&about_gui, normal);
    const mira::Hit about_hit = mira::guihit(about_gui, 8, 20);
    MIRA_TEST(about_hit.kind == mira::HitKind::kMenuAction);
    MIRA_TEST(mira::menuaction(about_gui, about_hit) == mira::MenuAction::kMiraAbout);
    mira::InputEvent click_about = {};
    click_about.kind = mira::InputKind::kMouseDown;
    click_about.x = 8;
    click_about.y = 20;
    mira::guievent(&about_gui, {&click_about, 1});
    MIRA_TEST(about_gui.about_dialog);
    MIRA_TEST(!about_gui.new_dialog);
    mira::guilayout(&about_gui, normal);
    MIRA_TEST(about_gui.layout.dialog.width == 248.0F);
    MIRA_TEST(about_gui.layout.dialog_logo.width == 42.0F);
    const mira::Hit about_ok =
        mira::guihit(about_gui, static_cast<mira::i32>(about_gui.layout.dialog_ok.x + 1.0F),
                     static_cast<mira::i32>(about_gui.layout.dialog_ok.y + 1.0F));
    MIRA_TEST(about_ok.kind == mira::HitKind::kDialogButton);
    MIRA_TEST(about_ok.priority > 120);
    mira::InputEvent close_about = {};
    close_about.kind = mira::InputKind::kKeyDown;
    close_about.button = static_cast<mira::u8>(mira::Key::kEnter);
    mira::guievent(&about_gui, {&close_about, 1});
    MIRA_TEST(!about_gui.about_dialog);

    static mira::GuiState about_frame_gui;
    static mira::DrawList about_frame_draws;
    mira::guiframe(&about_frame_gui, normal, {&open_mira_menu, 1}, &about_frame_draws);
    mira::guiframe(&about_frame_gui, normal, {&click_about, 1}, &about_frame_draws);
    MIRA_TEST(about_frame_gui.about_dialog);
    MIRA_TEST(about_frame_gui.layout.dialog.width == 248.0F);
    MIRA_TEST(about_frame_gui.layout.dialog.x > 0.0F);
    MIRA_TEST(about_frame_gui.layout.dialog.y > 0.0F);
    const mira::DrawPlaneStart modal_begin =
        about_frame_draws.plane_begin(mira::DrawPlane::kModal);
    MIRA_TEST(about_frame_draws.plane_count() ==
              static_cast<mira::usize>(mira::DrawPlane::kModal) + 1U);
    MIRA_TEST(modal_begin.rect < about_frame_draws.rects.size());
    MIRA_TEST(modal_begin.glyph < about_frame_draws.glyphs.size());

    static mira::GuiState canvas_context_gui;
    mira::guiinit(&canvas_context_gui);
    mira::guilayout(&canvas_context_gui, normal);
    mira::InputEvent canvas_context = {};
    canvas_context.kind = mira::InputKind::kMouseDown;
    canvas_context.button = 2;
    canvas_context.x = static_cast<mira::i32>(canvas_context_gui.layout.viewport.x + 24.0F);
    canvas_context.y = static_cast<mira::i32>(canvas_context_gui.layout.viewport.y + 24.0F);
    static mira::DrawList canvas_context_draws;
    mira::guiframe(&canvas_context_gui, normal, {&canvas_context, 1}, &canvas_context_draws);
    MIRA_TEST(canvas_context_gui.context_open);
    MIRA_TEST(mira::contextkind(canvas_context_gui) == mira::ContextKind::kWorkspace);
    MIRA_TEST(!canvas_context_gui.painting);
    MIRA_TEST(canvas_context_gui.paint_delta.empty());
    MIRA_TEST(canvas_context_gui.layout.context.x == static_cast<mira::f32>(canvas_context.x));
    MIRA_TEST(canvas_context_draws.plane_count() >=
              static_cast<mira::usize>(mira::DrawPlane::kMenu) + 1U);
    const mira::Hit undo_context =
        mira::guihit(canvas_context_gui,
                     static_cast<mira::i32>(canvas_context_gui.layout.context.x + 2.0F),
                     static_cast<mira::i32>(canvas_context_gui.layout.context.y + 2.0F));
    MIRA_TEST(undo_context.kind == mira::HitKind::kContextAction);
    MIRA_TEST(mira::menuaction(canvas_context_gui, undo_context) == mira::MenuAction::kUndo);
    mira::InputEvent reopen_context = {};
    reopen_context.kind = mira::InputKind::kMouseDown;
    reopen_context.button = 2;
    reopen_context.x = static_cast<mira::i32>(canvas_context_gui.layout.layerrows.x + 36.0F);
    reopen_context.y = static_cast<mira::i32>(canvas_context_gui.layout.layerrows.y + 8.0F);
    mira::guievent(&canvas_context_gui, {&reopen_context, 1});
    MIRA_TEST(canvas_context_gui.context_open);
    MIRA_TEST(mira::contextkind(canvas_context_gui) == mira::ContextKind::kLayer);
    MIRA_TEST(canvas_context_gui.context_target == 0);

    static mira::GuiState sidebar_context_gui;
    mira::guiinit(&sidebar_context_gui);
    mira::guilayout(&sidebar_context_gui, normal);
    mira::InputEvent sidebar_context = {};
    sidebar_context.kind = mira::InputKind::kMouseDown;
    sidebar_context.button = 2;
    sidebar_context.x = static_cast<mira::i32>(sidebar_context_gui.layout.layers.x + 18.0F);
    sidebar_context.y = static_cast<mira::i32>(sidebar_context_gui.layout.layerrows.y + 120.0F);
    mira::guievent(&sidebar_context_gui, {&sidebar_context, 1});
    MIRA_TEST(sidebar_context_gui.context_open);
    MIRA_TEST(mira::contextkind(sidebar_context_gui) == mira::ContextKind::kLayer);
    MIRA_TEST(sidebar_context_gui.context_target == mira::kNoLayer);
    mira::guilayout(&sidebar_context_gui, normal);
    const mira::Hit sidebar_new_context =
        mira::guihit(sidebar_context_gui,
                     static_cast<mira::i32>(sidebar_context_gui.layout.context.x + 2.0F),
                     static_cast<mira::i32>(sidebar_context_gui.layout.context.y + 2.0F));
    MIRA_TEST(sidebar_new_context.kind == mira::HitKind::kContextAction);
    MIRA_TEST(mira::menuaction(sidebar_context_gui, sidebar_new_context) ==
              mira::MenuAction::kLayerNew);

    static mira::GuiState layer_context_gui;
    mira::guiinit(&layer_context_gui);
    mira::guilayout(&layer_context_gui, normal);
    mira::InputEvent layer_context = {};
    layer_context.kind = mira::InputKind::kMouseDown;
    layer_context.button = 2;
    layer_context.x = static_cast<mira::i32>(layer_context_gui.layout.layerrows.x + 36.0F);
    layer_context.y = static_cast<mira::i32>(layer_context_gui.layout.layerrows.y + 8.0F);
    mira::guievent(&layer_context_gui, {&layer_context, 1});
    MIRA_TEST(layer_context_gui.context_open);
    MIRA_TEST(mira::contextkind(layer_context_gui) == mira::ContextKind::kLayer);
    MIRA_TEST(layer_context_gui.context_target == 0);
    MIRA_TEST(layer_context_gui.curlayer == 0);
    mira::guilayout(&layer_context_gui, normal);
    const mira::Hit rename_context =
        mira::guihit(layer_context_gui,
                     static_cast<mira::i32>(layer_context_gui.layout.context.x + 2.0F),
                     static_cast<mira::i32>(layer_context_gui.layout.context.y + 32.0F));
    MIRA_TEST(rename_context.kind == mira::HitKind::kContextAction);
    MIRA_TEST(mira::menuaction(layer_context_gui, rename_context) ==
              mira::MenuAction::kLayerRename);
    mira::InputEvent click_rename_context = {};
    click_rename_context.kind = mira::InputKind::kMouseDown;
    click_rename_context.x = static_cast<mira::i32>(layer_context_gui.layout.context.x + 2.0F);
    click_rename_context.y = static_cast<mira::i32>(layer_context_gui.layout.context.y + 32.0F);
    mira::guievent(&layer_context_gui, {&click_rename_context, 1});
    MIRA_TEST(!layer_context_gui.context_open);
    MIRA_TEST(layer_context_gui.renaming_layer == 0);

    static mira::GuiState menu_gui;
    mira::guiinit(&menu_gui);
    mira::guilayout(&menu_gui, normal);
    mira::InputEvent open_layer_menu = {};
    open_layer_menu.kind = mira::InputKind::kMouseDown;
    open_layer_menu.x = 145;
    open_layer_menu.y = 5;
    mira::guievent(&menu_gui, {&open_layer_menu, 1});
    MIRA_TEST(menu_gui.active_menu == 3);
    mira::guilayout(&menu_gui, normal);
    const mira::Hit new_layer_hit = mira::guihit(menu_gui, 145, 20);
    MIRA_TEST(new_layer_hit.kind == mira::HitKind::kMenuAction);
    mira::InputEvent click_new_layer = {};
    click_new_layer.kind = mira::InputKind::kMouseDown;
    click_new_layer.x = 145;
    click_new_layer.y = 20;
    mira::guievent(&menu_gui, {&click_new_layer, 1});
    MIRA_TEST(menu_gui.layers.size() == 3);
    MIRA_TEST(menu_gui.active_menu == mira::kNoMenu);
    MIRA_TEST(menu_gui.renaming_layer == 0);
    MIRA_TEST(mira::layername(menu_gui.layers[0]) == "layer 3");

    static mira::GuiState edit_gui;
    mira::guiinit(&edit_gui);
    mira::guilayout(&edit_gui, normal);
    mira::InputEvent open_edit_menu = {};
    open_edit_menu.kind = mira::InputKind::kMouseDown;
    open_edit_menu.x = 105;
    open_edit_menu.y = 5;
    mira::guievent(&edit_gui, {&open_edit_menu, 1});
    MIRA_TEST(edit_gui.active_menu == 2);
    mira::guilayout(&edit_gui, normal);
    const mira::Hit undo_hit = mira::guihit(edit_gui, 105, 20);
    const mira::Hit redo_hit = mira::guihit(edit_gui, 105, 35);
    MIRA_TEST(undo_hit.kind == mira::HitKind::kMenuAction);
    MIRA_TEST(redo_hit.kind == mira::HitKind::kMenuAction);
    MIRA_TEST(mira::menuaction(edit_gui, undo_hit) == mira::MenuAction::kUndo);
    MIRA_TEST(mira::menuaction(edit_gui, redo_hit) == mira::MenuAction::kRedo);

    static mira::GuiState file_gui;
    mira::guiinit(&file_gui);
    MIRA_TEST(mira::layeradd(&file_gui, "Temp"));
    mira::guilayout(&file_gui, normal);
    mira::InputEvent open_file_menu = {};
    open_file_menu.kind = mira::InputKind::kMouseDown;
    open_file_menu.x = 55;
    open_file_menu.y = 5;
    mira::guievent(&file_gui, {&open_file_menu, 1});
    MIRA_TEST(file_gui.active_menu == 1);
    mira::guilayout(&file_gui, normal);
    const mira::Hit import_hit = mira::guihit(file_gui, 55, 34);
    MIRA_TEST(import_hit.kind == mira::HitKind::kMenuAction);
    MIRA_TEST(mira::menuaction(file_gui, import_hit) == mira::MenuAction::kFileImport);
    const mira::Hit export_hit = mira::guihit(file_gui, 55, 49);
    MIRA_TEST(export_hit.kind == mira::HitKind::kMenuAction);
    MIRA_TEST(mira::menuaction(file_gui, export_hit) == mira::MenuAction::kFileExport);
    const mira::Hit new_file_hit = mira::guihit(file_gui, 55, 20);
    MIRA_TEST(mira::menuaction(file_gui, new_file_hit) == mira::MenuAction::kFileNew);
    mira::InputEvent click_file_new = {};
    click_file_new.kind = mira::InputKind::kMouseDown;
    click_file_new.x = 55;
    click_file_new.y = 20;
    mira::guievent(&file_gui, {&click_file_new, 1});
    MIRA_TEST(file_gui.new_dialog);
    MIRA_TEST(file_gui.layers.size() == 3);
    MIRA_TEST(file_gui.new_width == mira::kDefaultDocumentWidth);
    MIRA_TEST(file_gui.new_height == mira::kDefaultDocumentHeight);
    mira::guilayout(&file_gui, normal);
    const mira::Hit dialog_hit =
        mira::guihit(file_gui, static_cast<mira::i32>(file_gui.layout.dialog_width.x + 2.0F),
                     static_cast<mira::i32>(file_gui.layout.dialog_width.y + 2.0F));
    MIRA_TEST(dialog_hit.kind == mira::HitKind::kDialogField);
    MIRA_TEST(dialog_hit.priority > 120);
    mira::InputEvent width_6 = {};
    width_6.kind = mira::InputKind::kText;
    width_6.dx = '6';
    mira::InputEvent width_4 = width_6;
    width_4.dx = '4';
    mira::InputEvent next_field = {};
    next_field.kind = mira::InputKind::kKeyDown;
    next_field.button = static_cast<mira::u8>(mira::Key::kTab);
    mira::InputEvent height_4 = width_6;
    height_4.dx = '4';
    mira::InputEvent height_8 = width_6;
    height_8.dx = '8';
    mira::InputEvent accept_new = {};
    accept_new.kind = mira::InputKind::kKeyDown;
    accept_new.button = static_cast<mira::u8>(mira::Key::kEnter);
    const mira::InputEvent new_events[] = {width_6, width_4, next_field, height_4, height_8,
                                           accept_new};
    mira::guievent(&file_gui, new_events);
    MIRA_TEST(!file_gui.new_dialog);
    MIRA_TEST(file_gui.document.width == 64);
    MIRA_TEST(file_gui.document.height == 48);
    MIRA_TEST(file_gui.layers.size() == 2);
    MIRA_TEST(mira::layername(file_gui.layers[0]) == "ink");
    MIRA_TEST(file_gui.clear_slots.size() == mira::kMaxLayers);
    MIRA_TEST(file_gui.recreate_layers);
    MIRA_TEST(file_gui.active_menu == mira::kNoMenu);

    static mira::GuiState export_gui;
    mira::guiinit(&export_gui);
    mira::guilayout(&export_gui, normal);
    mira::InputEvent open_export_menu = {};
    open_export_menu.kind = mira::InputKind::kMouseDown;
    open_export_menu.x = 55;
    open_export_menu.y = 5;
    mira::guievent(&export_gui, {&open_export_menu, 1});
    mira::guilayout(&export_gui, normal);
    mira::InputEvent click_export = {};
    click_export.kind = mira::InputKind::kMouseDown;
    click_export.x = 55;
    click_export.y = 49;
    mira::guievent(&export_gui, {&click_export, 1});
    MIRA_TEST(export_gui.actions.size() == 1);
    MIRA_TEST(export_gui.actions[0].kind == mira::GuiActionKind::kExportPng);
    MIRA_TEST(export_gui.active_menu == mira::kNoMenu);

    static mira::GuiState import_gui;
    mira::guiinit(&import_gui);
    mira::guilayout(&import_gui, normal);
    mira::InputEvent open_import_menu = {};
    open_import_menu.kind = mira::InputKind::kMouseDown;
    open_import_menu.x = 55;
    open_import_menu.y = 5;
    mira::guievent(&import_gui, {&open_import_menu, 1});
    mira::guilayout(&import_gui, normal);
    mira::InputEvent click_import = {};
    click_import.kind = mira::InputKind::kMouseDown;
    click_import.x = 55;
    click_import.y = 34;
    mira::guievent(&import_gui, {&click_import, 1});
    MIRA_TEST(import_gui.actions.size() == 1);
    MIRA_TEST(import_gui.actions[0].kind == mira::GuiActionKind::kOpenImagePicker);
    MIRA_TEST(import_gui.active_menu == mira::kNoMenu);

    static mira::GuiState image_gui;
    mira::guiinit(&image_gui);
    const mira::u8 image_index = mira::layerimage(&image_gui, "Reference", 96);
    MIRA_TEST(image_index == 1);
    MIRA_TEST(image_gui.layers.size() == 3);
    MIRA_TEST(mira::layername(image_gui.layers[1]) == "Reference");
    MIRA_TEST(image_gui.layers[1].kind == mira::LayerKind::kImage);
    MIRA_TEST(image_gui.layers[1].opacity_u8 == 96);
    MIRA_TEST(mira::layerlocked(image_gui.layers[1]));
    MIRA_TEST(image_gui.layers[1].layer_slot == 1);
    MIRA_TEST(mira::layername(image_gui.layers[2]) == "background");
    static mira::GuiState default_image_gui;
    mira::guiinit(&default_image_gui);
    const mira::u8 default_image_index = mira::layerimage(&default_image_gui, "Import");
    MIRA_TEST(default_image_index == 1);
    MIRA_TEST(default_image_gui.layers[1].opacity_u8 == 255);

    const mira::Hit tool_hit =
        mira::guihit(gui, static_cast<mira::i32>(gui.layout.tools.x + 1.0F),
                     static_cast<mira::i32>(gui.layout.tools.y + 1.0F));
    MIRA_TEST(tool_hit.kind == mira::HitKind::kTool);
    MIRA_TEST(tool_hit.index == 0);
    const mira::Hit brush_hit =
        mira::guihit(gui, static_cast<mira::i32>(gui.layout.tips.x + 1.0F),
                     static_cast<mira::i32>(gui.layout.tips.y + 1.0F));
    MIRA_TEST(brush_hit.kind == mira::HitKind::kToolbar);
    const mira::Hit empty_tool_hit =
        mira::guihit(gui, static_cast<mira::i32>(gui.layout.tips.x + 29.0F),
                     static_cast<mira::i32>(gui.layout.tips.y + 1.0F));
    MIRA_TEST(empty_tool_hit.kind == mira::HitKind::kToolbar);

    mira::InputEvent inactive_brush = {};
    inactive_brush.kind = mira::InputKind::kMouseDown;
    inactive_brush.x = static_cast<mira::i32>(gui.layout.tips.x + 8.0F);
    inactive_brush.y = static_cast<mira::i32>(gui.layout.tips.y + 4.0F);
    mira::guievent(&gui, {&inactive_brush, 1});
    MIRA_TEST(gui.cursize == 3);
    MIRA_TEST(!gui.brush_open);
    mira::InputEvent close_brush = {};
    close_brush.kind = mira::InputKind::kKeyDown;
    close_brush.button = static_cast<mira::u8>(mira::Key::kEscape);
    mira::guievent(&gui, {&close_brush, 1});
    MIRA_TEST(!gui.brush_open);

    static mira::GuiState reactive_gui;
    mira::guiinit(&reactive_gui);
    mira::guilayout(&reactive_gui, normal);
    mira::InputEvent reactive_brush = {};
    reactive_brush.kind = mira::InputKind::kMouseDown;
    reactive_brush.x = static_cast<mira::i32>(reactive_gui.layout.tools.x + 8.0F);
    reactive_brush.y = static_cast<mira::i32>(reactive_gui.layout.tools.y + 28.0F);
    mira::guievent(&reactive_gui, {&reactive_brush, 1});
    mira::guilayout(&reactive_gui, normal);
    const mira::Hit brush_tip_hit =
        mira::guihit(reactive_gui, static_cast<mira::i32>(reactive_gui.layout.tips.x + 1.0F),
                     static_cast<mira::i32>(reactive_gui.layout.tips.y + 1.0F));
    const mira::Hit brush_button_hit =
        mira::guihit(reactive_gui,
                     static_cast<mira::i32>(reactive_gui.layout.brush_button.x + 1.0F),
                     static_cast<mira::i32>(reactive_gui.layout.brush_button.y + 1.0F));
    MIRA_TEST(brush_tip_hit.kind == mira::HitKind::kTip);
    MIRA_TEST(brush_button_hit.kind == mira::HitKind::kBrushButton);
    mira::InputEvent open_brush = {};
    open_brush.kind = mira::InputKind::kMouseDown;
    open_brush.x = static_cast<mira::i32>(reactive_gui.layout.brush_button.x + 8.0F);
    open_brush.y = static_cast<mira::i32>(reactive_gui.layout.brush_button.y + 4.0F);
    mira::guievent(&reactive_gui, {&open_brush, 1});
    MIRA_TEST(reactive_gui.brush_open);
    mira::guilayout(&reactive_gui, normal);
    const mira::Hit tip_row_hit =
        mira::guihit(reactive_gui,
                     static_cast<mira::i32>(reactive_gui.layout.brush_panel.x + 14.0F),
                     static_cast<mira::i32>(reactive_gui.layout.brush_panel.y + 166.0F));
    MIRA_TEST(tip_row_hit.kind == mira::HitKind::kTip);
    MIRA_TEST(tip_row_hit.index == 2);
    const mira::Hit coverage_hit =
        mira::guihit(reactive_gui,
                     static_cast<mira::i32>(reactive_gui.layout.brush_panel.x + 50.0F),
                     static_cast<mira::i32>(reactive_gui.layout.brush_panel.y + 70.0F));
    MIRA_TEST(coverage_hit.kind == mira::HitKind::kCoverage);
    const mira::Hit brush_title_hit =
        mira::guihit(reactive_gui,
                     static_cast<mira::i32>(reactive_gui.layout.brush_panel.x + 8.0F),
                     static_cast<mira::i32>(reactive_gui.layout.brush_panel.y + 5.0F));
    MIRA_TEST(brush_title_hit.kind == mira::HitKind::kBrushTitle);
    const mira::Hit brush_close_hit =
        mira::guihit(reactive_gui,
                     static_cast<mira::i32>(reactive_gui.layout.brush_panel.x +
                                            reactive_gui.layout.brush_panel.width - 10.0F),
                     static_cast<mira::i32>(reactive_gui.layout.brush_panel.y + 8.0F));
    MIRA_TEST(brush_close_hit.kind == mira::HitKind::kBrushClose);
    const mira::f32 brush_x_before = reactive_gui.layout.brush_panel.x;
    const mira::f32 brush_y_before = reactive_gui.layout.brush_panel.y;
    mira::InputEvent drag_brush_down = {};
    drag_brush_down.kind = mira::InputKind::kMouseDown;
    drag_brush_down.x = static_cast<mira::i32>(brush_x_before + 8.0F);
    drag_brush_down.y = static_cast<mira::i32>(brush_y_before + 5.0F);
    mira::guievent(&reactive_gui, {&drag_brush_down, 1});
    MIRA_TEST(reactive_gui.moving_brush);
    mira::InputEvent drag_brush_move = {};
    drag_brush_move.kind = mira::InputKind::kMouseMove;
    drag_brush_move.buttons = 1;
    drag_brush_move.x = static_cast<mira::i32>(brush_x_before + 32.0F);
    drag_brush_move.y = static_cast<mira::i32>(brush_y_before + 17.0F);
    mira::guievent(&reactive_gui, {&drag_brush_move, 1});
    MIRA_TEST(close(reactive_gui.brush_x, brush_x_before + 24.0F));
    const mira::f32 expected_brush_y =
        std::min(brush_y_before + 12.0F,
                 reactive_gui.layout.window.height - reactive_gui.layout.brush_panel.height);
    MIRA_TEST(close(reactive_gui.brush_y, expected_brush_y));
    mira::InputEvent drag_brush_up = drag_brush_move;
    drag_brush_up.kind = mira::InputKind::kMouseUp;
    mira::guievent(&reactive_gui, {&drag_brush_up, 1});
    MIRA_TEST(!reactive_gui.moving_brush);
    mira::InputEvent outside_brush = {};
    outside_brush.kind = mira::InputKind::kMouseDown;
    outside_brush.x = static_cast<mira::i32>(reactive_gui.layout.layers.x + 8.0F);
    outside_brush.y = static_cast<mira::i32>(reactive_gui.layout.layers.y + 8.0F);
    mira::guievent(&reactive_gui, {&outside_brush, 1});
    MIRA_TEST(reactive_gui.brush_open);

    mira::InputEvent reactive_magic = reactive_brush;
    reactive_magic.y = static_cast<mira::i32>(reactive_gui.layout.tools.y + 76.0F);
    mira::guievent(&reactive_gui, {&reactive_magic, 1});
    mira::guilayout(&reactive_gui, normal);
    const mira::Hit magic_tip_hit =
        mira::guihit(reactive_gui, static_cast<mira::i32>(reactive_gui.layout.tips.x + 1.0F),
                     static_cast<mira::i32>(reactive_gui.layout.tips.y + 1.0F));
    const mira::Hit magic_brush_hit =
        mira::guihit(reactive_gui, static_cast<mira::i32>(reactive_gui.layout.tips.x + 29.0F),
                     static_cast<mira::i32>(reactive_gui.layout.tips.y + 1.0F));
    MIRA_TEST(magic_tip_hit.kind == mira::HitKind::kToolbar);
    MIRA_TEST(magic_brush_hit.kind == mira::HitKind::kToolbar);

    mira::InputEvent reactive_zoom = reactive_brush;
    reactive_zoom.y = static_cast<mira::i32>(reactive_gui.layout.tools.y + 124.0F);
    mira::guievent(&reactive_gui, {&reactive_zoom, 1});
    mira::guilayout(&reactive_gui, normal);
    const mira::Hit zoom_tip_hit =
        mira::guihit(reactive_gui, static_cast<mira::i32>(reactive_gui.layout.tips.x + 1.0F),
                     static_cast<mira::i32>(reactive_gui.layout.tips.y + 1.0F));
    const mira::Hit zoom_brush_hit =
        mira::guihit(reactive_gui, static_cast<mira::i32>(reactive_gui.layout.tips.x + 29.0F),
                     static_cast<mira::i32>(reactive_gui.layout.tips.y + 1.0F));
    MIRA_TEST(zoom_tip_hit.kind == mira::HitKind::kToolbar);
    MIRA_TEST(zoom_brush_hit.kind == mira::HitKind::kToolbar);
    const mira::Hit viewport_hit =
        mira::guihit(gui, static_cast<mira::i32>(gui.layout.viewport.x + 24.0F),
                     static_cast<mira::i32>(gui.layout.viewport.y + 24.0F));
    MIRA_TEST(viewport_hit.kind == mira::HitKind::kViewport);
    const mira::Hit visibility_hit =
        mira::guihit(gui, static_cast<mira::i32>(gui.layout.layerrows.x + 7.0F),
                     static_cast<mira::i32>(gui.layout.layerrows.y + 9.0F));
    MIRA_TEST(visibility_hit.kind == mira::HitKind::kLayerVisibility);
    MIRA_TEST(visibility_hit.priority > 80);

    mira::InputEvent toggle_visibility = {};
    toggle_visibility.kind = mira::InputKind::kMouseDown;
    toggle_visibility.x = static_cast<mira::i32>(gui.layout.layerrows.x + 7.0F);
    toggle_visibility.y = static_cast<mira::i32>(gui.layout.layerrows.y + 9.0F);
    mira::guievent(&gui, {&toggle_visibility, 1});
    MIRA_TEST(!mira::layervisible(gui.layers[0]));

    mira::InputEvent select_background = {};
    select_background.kind = mira::InputKind::kMouseDown;
    select_background.x = static_cast<mira::i32>(gui.layout.layerrows.x + 20.0F);
    select_background.y = static_cast<mira::i32>(gui.layout.layerrows.y + 36.0F);
    mira::guievent(&gui, {&select_background, 1});
    MIRA_TEST(gui.curlayer == 1);
    MIRA_TEST(mira::layerselected(gui.layers[1]));
    MIRA_TEST(!mira::layerselected(gui.layers[0]));

    const mira::Hit background_lock_hit =
        mira::guihit(gui, static_cast<mira::i32>(gui.layout.layerrows.x + 21.0F),
                     static_cast<mira::i32>(gui.layout.layerrows.y + 43.0F));
    MIRA_TEST(background_lock_hit.kind == mira::HitKind::kLayerLock);
    mira::InputEvent unlock_background = {};
    unlock_background.kind = mira::InputKind::kMouseDown;
    unlock_background.x = background_lock_hit.rect.x + 1;
    unlock_background.y = background_lock_hit.rect.y + 1;
    mira::guievent(&gui, {&unlock_background, 1});
    MIRA_TEST(mira::layerlocked(gui.layers[1]));
    MIRA_TEST(!mira::layerdel(&gui));
    MIRA_TEST(gui.layers.size() == 2);

    MIRA_TEST(mira::layeradd(&gui, "Sketch"));
    MIRA_TEST(gui.layers.size() == 3);
    MIRA_TEST(gui.curlayer == 1);
    MIRA_TEST(mira::layername(gui.layers[1]) == "Sketch");
    MIRA_TEST(gui.layers[1].layer_slot == 1);
    MIRA_TEST(gui.clear_slots.size() == 1);
    MIRA_TEST(gui.clear_slots[0] == 1);
    MIRA_TEST(mira::layerrename(&gui, 1, "Line 2"));
    MIRA_TEST(mira::layername(gui.layers[1]) == "Line 2");
    MIRA_TEST(mira::layeredit(&gui));
    MIRA_TEST(gui.renaming_layer == 1);
    mira::InputEvent rename_text = {};
    rename_text.kind = mira::InputKind::kText;
    rename_text.dx = 'A';
    mira::InputEvent rename_enter = {};
    rename_enter.kind = mira::InputKind::kKeyDown;
    rename_enter.button = static_cast<mira::u8>(mira::Key::kEnter);
    const mira::InputEvent rename_events[] = {rename_text, rename_enter};
    mira::guievent(&gui, rename_events);
    MIRA_TEST(mira::layername(gui.layers[1]) == "A");
    MIRA_TEST(gui.renaming_layer == mira::kNoLayer);
    mira::guilayout(&gui, normal);

    const mira::Hit opacity_hit =
        mira::guihit(gui, static_cast<mira::i32>(gui.layout.layerrows.x + 40.0F),
                     static_cast<mira::i32>(gui.layout.layerrows.y + 62.0F));
    MIRA_TEST(opacity_hit.kind == mira::HitKind::kLayerOpacity);
    mira::InputEvent opacity_click = {};
    opacity_click.kind = mira::InputKind::kMouseDown;
    opacity_click.x = static_cast<mira::i32>(gui.layout.layerrows.x + 50.0F);
    opacity_click.y = static_cast<mira::i32>(gui.layout.layerrows.y + 62.0F);
    mira::guievent(&gui, {&opacity_click, 1});
    MIRA_TEST(gui.layers[1].opacity_u8 < 255);

    mira::InputEvent select_line = {};
    select_line.kind = mira::InputKind::kMouseDown;
    select_line.x = static_cast<mira::i32>(gui.layout.tools.x + 8.0F);
    select_line.y = static_cast<mira::i32>(gui.layout.tools.y + 52.0F);
    mira::guievent(&gui, {&select_line, 1});
    MIRA_TEST(gui.curtool == 2);
    MIRA_TEST(gui.tools[2].selected == 1);
    MIRA_TEST(gui.tools[0].selected == 0);

    static mira::DrawList list;
    mira::guiframe(&gui, normal, {}, &list);
    MIRA_TEST(list.rects.size() >= 5);
    MIRA_TEST(list.glyphs.size() > 0);
    MIRA_TEST(list.icons.size() == 19);
    MIRA_TEST(list.upload_bytes() == list.rects.byte_size() + list.glyphs.byte_size() +
                                          list.icons.byte_size() +
                                          list.preview_stamps.byte_size());
    MIRA_TEST(list.overflow_count() == 0);
    MIRA_TEST(list.rects.size() <= 76);
    MIRA_TEST(list.glyphs.size() <= 64);

    static mira::GuiState cursor_gui;
    mira::guiinit(&cursor_gui);
    cursor_gui.view_initialized = true;
    cursor_gui.view.x = 0.0F;
    cursor_gui.view.y = 0.0F;
    cursor_gui.view.zoom = 8.0F;
    mira::guilayout(&cursor_gui, normal);
    mira::InputEvent cursor_move = {};
    cursor_move.kind = mira::InputKind::kMouseMove;
    cursor_move.x = static_cast<mira::i32>(cursor_gui.layout.viewport.x + (10.0F * 8.0F) + 1.0F);
    cursor_move.y = static_cast<mira::i32>(cursor_gui.layout.viewport.y + (12.0F * 8.0F) + 1.0F);
    static mira::DrawList cursor_list;
    mira::guiframe(&cursor_gui, normal, {&cursor_move, 1}, &cursor_list);
    MIRA_TEST(cursor_list.preview_stamps.size() == 1);
    MIRA_TEST(close(cursor_list.preview_stamps[0].x, 10.0F));
    MIRA_TEST(close(cursor_list.preview_stamps[0].y, 12.0F));
    MIRA_TEST(close(cursor_list.preview_stamps[0].diameter, 1.0F));
    MIRA_TEST(close(cursor_list.preview_stamps[0].tip, 0.0F));

    mira::InputEvent select_tip = {};
    select_tip.kind = mira::InputKind::kMouseDown;
    select_tip.x = static_cast<mira::i32>(gui.layout.tips.x + 8.0F);
    select_tip.y = static_cast<mira::i32>(gui.layout.tips.y + 148.0F);
    mira::guievent(&gui, {&select_tip, 1});
    MIRA_TEST(gui.curtip == 6);
    MIRA_TEST(gui.tips[6].selected == 1);
    MIRA_TEST(gui.tips[3].selected == 0);
    MIRA_TEST(gui.cursize == 3);

    mira::InputEvent open_brush_panel = {};
    open_brush_panel.kind = mira::InputKind::kMouseDown;
    open_brush_panel.x = static_cast<mira::i32>(gui.layout.brush_button.x + 8.0F);
    open_brush_panel.y = static_cast<mira::i32>(gui.layout.brush_button.y + 4.0F);
    mira::guievent(&gui, {&open_brush_panel, 1});
    MIRA_TEST(gui.brush_open);
    mira::guilayout(&gui, normal);
    mira::InputEvent select_size = {};
    select_size.kind = mira::InputKind::kMouseDown;
    select_size.x = static_cast<mira::i32>(gui.layout.brush_panel.x + 118.0F);
    select_size.y = static_cast<mira::i32>(gui.layout.brush_panel.y + 40.0F);
    mira::guievent(&gui, {&select_size, 1});
    MIRA_TEST(gui.cursize == 6);
    MIRA_TEST(gui.sizes[6].selected == 1);
    MIRA_TEST(gui.sizes[3].selected == 0);

    mira::InputEvent select_coverage = {};
    select_coverage.kind = mira::InputKind::kMouseDown;
    select_coverage.x = static_cast<mira::i32>(gui.layout.brush_panel.x + 50.0F);
    select_coverage.y = static_cast<mira::i32>(gui.layout.brush_panel.y + 70.0F);
    mira::guievent(&gui, {&select_coverage, 1});
    MIRA_TEST(gui.curcoverage == 2);
    MIRA_TEST(gui.brush_open);
    MIRA_TEST(gui.coverages[2].selected == 1);
    MIRA_TEST(gui.coverages[0].selected == 0);

    mira::InputEvent select_brush = {};
    select_brush.kind = mira::InputKind::kMouseDown;
    select_brush.x = static_cast<mira::i32>(gui.layout.tools.x + 8.0F);
    select_brush.y = static_cast<mira::i32>(gui.layout.tools.y + 28.0F);
    mira::guievent(&gui, {&select_brush, 1});
    MIRA_TEST(gui.curtool == 1);

    mira::InputEvent select_background_again = {};
    select_background_again.kind = mira::InputKind::kMouseDown;
    select_background_again.x = static_cast<mira::i32>(gui.layout.layerrows.x + 40.0F);
    select_background_again.y = static_cast<mira::i32>(gui.layout.layerrows.y + 80.0F);
    mira::guievent(&gui, {&select_background_again, 1});
    MIRA_TEST(gui.curlayer == 2);
    mira::InputEvent locked_background_down = {};
    locked_background_down.kind = mira::InputKind::kMouseDown;
    locked_background_down.x = static_cast<mira::i32>(gui.layout.viewport.x + 30.0F);
    locked_background_down.y = static_cast<mira::i32>(gui.layout.viewport.y + 30.0F);
    mira::guievent(&gui, {&locked_background_down, 1});
    MIRA_TEST(!gui.painting);
    MIRA_TEST(gui.paint_delta.empty());

    mira::InputEvent select_ink = {};
    select_ink.kind = mira::InputKind::kMouseDown;
    select_ink.x = static_cast<mira::i32>(gui.layout.layerrows.x + 40.0F);
    select_ink.y = static_cast<mira::i32>(gui.layout.layerrows.y + 8.0F);
    mira::guievent(&gui, {&select_ink, 1});
    MIRA_TEST(gui.curlayer == 0);

    static mira::GuiState outside_gui;
    mira::guiinit(&outside_gui);
    mira::guilayout(&outside_gui, normal);
    outside_gui.view.x = -20.0F;
    outside_gui.view.y = -20.0F;
    mira::guilayout(&outside_gui, normal);
    mira::InputEvent outside_document_down = {};
    outside_document_down.kind = mira::InputKind::kMouseDown;
    outside_document_down.x = static_cast<mira::i32>(outside_gui.layout.viewport.x + 4.0F);
    outside_document_down.y = static_cast<mira::i32>(outside_gui.layout.viewport.y + 4.0F);
    mira::guievent(&outside_gui, {&outside_document_down, 1});
    MIRA_TEST(!outside_gui.painting);
    MIRA_TEST(outside_gui.paint_delta.empty());

    static mira::GuiState edge_stroke_gui;
    mira::guiinit(&edge_stroke_gui);
    mira::guilayout(&edge_stroke_gui, normal);
    edge_stroke_gui.view.x = -8.0F;
    edge_stroke_gui.view.y = 0.0F;
    edge_stroke_gui.view.zoom = 1.0F;
    mira::guilayout(&edge_stroke_gui, normal);
    mira::InputEvent edge_stroke_down = {};
    edge_stroke_down.kind = mira::InputKind::kMouseDown;
    edge_stroke_down.x = static_cast<mira::i32>(edge_stroke_gui.layout.document.x + 2.0F);
    edge_stroke_down.y = static_cast<mira::i32>(edge_stroke_gui.layout.document.y + 8.0F);
    mira::InputEvent edge_stroke_move = edge_stroke_down;
    edge_stroke_move.kind = mira::InputKind::kMouseMove;
    edge_stroke_move.buttons = 1;
    edge_stroke_move.x = static_cast<mira::i32>(edge_stroke_gui.layout.document.x - 2.0F);
    mira::InputEvent edge_stroke_up = edge_stroke_move;
    edge_stroke_up.kind = mira::InputKind::kMouseUp;
    const mira::InputEvent edge_stroke_events[] = {edge_stroke_down, edge_stroke_move,
                                                   edge_stroke_up};
    mira::guievent(&edge_stroke_gui, edge_stroke_events);
    MIRA_TEST(!edge_stroke_gui.painting);
    MIRA_TEST(edge_stroke_gui.strokes.size() == 1);
    MIRA_TEST(edge_stroke_gui.stroke_cursor == 1);
    MIRA_TEST(edge_stroke_gui.paint_delta.size() == 3);
    MIRA_TEST(edge_stroke_gui.history_stamps.size() == 3);
    MIRA_TEST(close(edge_stroke_gui.paint_delta[0].x, 2.0F));
    MIRA_TEST(close(edge_stroke_gui.paint_delta[2].x, 0.0F));

    gui.view.x = 0.0F;
    gui.view.y = 0.0F;
    mira::guilayout(&gui, normal);

    const mira::i32 paint_y = static_cast<mira::i32>(gui.layout.document.y + 30.0F);
    mira::InputEvent paint_down = {};
    paint_down.kind = mira::InputKind::kMouseDown;
    paint_down.x = static_cast<mira::i32>(gui.layout.document.x + 40.0F);
    paint_down.y = paint_y;
    mira::InputEvent paint_move = {};
    paint_move.kind = mira::InputKind::kMouseMove;
    paint_move.buttons = 1;
    paint_move.x = static_cast<mira::i32>(gui.layout.document.x + 50.0F);
    paint_move.y = paint_y;
    mira::InputEvent paint_up = paint_move;
    paint_up.kind = mira::InputKind::kMouseUp;
    const mira::InputEvent paint_events[] = {paint_down, paint_move, paint_up};
    mira::guievent(&gui, paint_events);
    MIRA_TEST(gui.paint_delta.size() == 21);
    MIRA_TEST(close(gui.paint_delta[0].x, 80.0F));
    MIRA_TEST(close(gui.paint_delta[0].y, 60.0F));
    MIRA_TEST(close(gui.paint_delta[20].x, 100.0F));
    MIRA_TEST(close(gui.paint_delta[20].y, 60.0F));
    MIRA_TEST(close(gui.paint_delta[0].diameter, 7.0F));
    MIRA_TEST(close(gui.paint_delta[0].tip, 6.0F));
    MIRA_TEST(close(gui.paint_delta[0].tone,
                    static_cast<mira::f32>(mira::tone_value(mira::Tone::kBlack))));
    MIRA_TEST(close(gui.paint_delta[0].layer_slot, 0.0F));
    MIRA_TEST(close(gui.paint_delta[0].coverage, 2.0F));
    MIRA_TEST(gui.strokes.size() == 1);
    MIRA_TEST(gui.stroke_cursor == 1);
    MIRA_TEST(gui.history_stamps.size() == 21);
    MIRA_TEST(gui.strokes[0].first_stamp == 0);
    MIRA_TEST(gui.strokes[0].stamp_count == 21);

    mira::InputEvent undo_stroke = {};
    undo_stroke.kind = mira::InputKind::kKeyDown;
    undo_stroke.button = static_cast<mira::u8>(mira::Key::kUndo);
    static mira::DrawList undo_list;
    mira::guiframe(&gui, normal, {&undo_stroke, 1}, &undo_list);
    MIRA_TEST(gui.stroke_cursor == 0);
    MIRA_TEST(gui.clear_slots.size() == 1);
    MIRA_TEST(gui.paint_delta.empty());

    mira::InputEvent redo_stroke = {};
    redo_stroke.kind = mira::InputKind::kKeyDown;
    redo_stroke.button = static_cast<mira::u8>(mira::Key::kRedo);
    static mira::DrawList redo_list;
    mira::guiframe(&gui, normal, {&redo_stroke, 1}, &redo_list);
    MIRA_TEST(gui.stroke_cursor == 1);
    MIRA_TEST(gui.clear_slots.size() == 1);
    MIRA_TEST(gui.paint_delta.size() == 21);
    MIRA_TEST(close(gui.paint_delta[0].coverage, 2.0F));

    static mira::GuiState dropped_up_gui;
    mira::guiinit(&dropped_up_gui);
    mira::guilayout(&dropped_up_gui, normal);
    dropped_up_gui.view.x = 0.0F;
    dropped_up_gui.view.y = 0.0F;
    mira::guilayout(&dropped_up_gui, normal);
    mira::InputEvent dropped_down = {};
    dropped_down.kind = mira::InputKind::kMouseDown;
    dropped_down.x = static_cast<mira::i32>(dropped_up_gui.layout.document.x + 8.0F);
    dropped_down.y = static_cast<mira::i32>(dropped_up_gui.layout.document.y + 8.0F);
    mira::InputEvent dropped_move = dropped_down;
    dropped_move.kind = mira::InputKind::kMouseMove;
    dropped_move.buttons = 0;
    dropped_move.x = static_cast<mira::i32>(dropped_up_gui.layout.document.x + 10.0F);
    const mira::InputEvent dropped_events[] = {dropped_down, dropped_move};
    mira::guievent(&dropped_up_gui, dropped_events);
    MIRA_TEST(!dropped_up_gui.painting);
    MIRA_TEST(dropped_up_gui.strokes.size() == 1);

    static mira::GuiState overflow_paint_gui;
    mira::guiinit(&overflow_paint_gui);
    mira::guilayout(&overflow_paint_gui, normal);
    overflow_paint_gui.view.x = 0.0F;
    overflow_paint_gui.view.y = 0.0F;
    mira::guilayout(&overflow_paint_gui, normal);
    mira::PaintStamp dummy_stamp = {};
    for (mira::usize index = 0; index + 1U < overflow_paint_gui.paint_delta.capacity();
         ++index) {
        MIRA_TEST(overflow_paint_gui.paint_delta.push(dummy_stamp));
    }
    mira::InputEvent overflow_down = {};
    overflow_down.kind = mira::InputKind::kMouseDown;
    overflow_down.x = static_cast<mira::i32>(overflow_paint_gui.layout.document.x + 8.0F);
    overflow_down.y = static_cast<mira::i32>(overflow_paint_gui.layout.document.y + 8.0F);
    mira::InputEvent overflow_move = overflow_down;
    overflow_move.kind = mira::InputKind::kMouseMove;
    overflow_move.buttons = 1;
    overflow_move.x = static_cast<mira::i32>(overflow_paint_gui.layout.document.x + 12.0F);
    const mira::InputEvent overflow_events[] = {overflow_down, overflow_move};
    mira::guievent(&overflow_paint_gui, overflow_events);
    MIRA_TEST(overflow_paint_gui.paint_delta.size() ==
              overflow_paint_gui.paint_delta.capacity() - 1U);
    MIRA_TEST(overflow_paint_gui.paint_delta.overflowed);
    MIRA_TEST(overflow_paint_gui.history_stamps.empty());
    MIRA_TEST(overflow_paint_gui.strokes.empty());
    MIRA_TEST(!overflow_paint_gui.painting);

    static mira::GuiState overflow_action_gui;
    mira::guiinit(&overflow_action_gui);
    mira::guilayout(&overflow_action_gui, normal);
    overflow_action_gui.view.x = 0.0F;
    overflow_action_gui.view.y = 0.0F;
    mira::guilayout(&overflow_action_gui, normal);
    mira::Stroke dummy_action = {};
    for (mira::usize index = 0; index < overflow_action_gui.strokes.capacity(); ++index) {
        MIRA_TEST(overflow_action_gui.strokes.push(dummy_action));
    }
    overflow_action_gui.stroke_cursor = static_cast<mira::u32>(overflow_action_gui.strokes.size());
    mira::InputEvent overflow_action_down = {};
    overflow_action_down.kind = mira::InputKind::kMouseDown;
    overflow_action_down.x = static_cast<mira::i32>(overflow_action_gui.layout.document.x + 8.0F);
    overflow_action_down.y = static_cast<mira::i32>(overflow_action_gui.layout.document.y + 8.0F);
    mira::guievent(&overflow_action_gui, {&overflow_action_down, 1});
    MIRA_TEST(overflow_action_gui.paint_delta.empty());
    MIRA_TEST(overflow_action_gui.history_stamps.empty());
    MIRA_TEST(overflow_action_gui.strokes.overflowed);
    MIRA_TEST(!overflow_action_gui.painting);

    static mira::GuiState zoom_paint_gui;
    mira::guiinit(&zoom_paint_gui);
    mira::guilayout(&zoom_paint_gui, normal);
    zoom_paint_gui.view.x = 0.0F;
    zoom_paint_gui.view.y = 0.0F;
    zoom_paint_gui.view.zoom = 2.0F;
    mira::guilayout(&zoom_paint_gui, normal);
    mira::InputEvent zoom_select_brush = {};
    zoom_select_brush.kind = mira::InputKind::kMouseDown;
    zoom_select_brush.x = static_cast<mira::i32>(zoom_paint_gui.layout.tools.x + 8.0F);
    zoom_select_brush.y = static_cast<mira::i32>(zoom_paint_gui.layout.tools.y + 28.0F);
    mira::guievent(&zoom_paint_gui, {&zoom_select_brush, 1});
    mira::guilayout(&zoom_paint_gui, normal);
    mira::InputEvent zoom_open_brush = {};
    zoom_open_brush.kind = mira::InputKind::kMouseDown;
    zoom_open_brush.x = static_cast<mira::i32>(zoom_paint_gui.layout.brush_button.x + 8.0F);
    zoom_open_brush.y = static_cast<mira::i32>(zoom_paint_gui.layout.brush_button.y + 4.0F);
    mira::guievent(&zoom_paint_gui, {&zoom_open_brush, 1});
    mira::guilayout(&zoom_paint_gui, normal);
    mira::InputEvent zoom_select_size = {};
    zoom_select_size.kind = mira::InputKind::kMouseDown;
    zoom_select_size.x = static_cast<mira::i32>(zoom_paint_gui.layout.brush_panel.x + 118.0F);
    zoom_select_size.y = static_cast<mira::i32>(zoom_paint_gui.layout.brush_panel.y + 40.0F);
    mira::guievent(&zoom_paint_gui, {&zoom_select_size, 1});
    mira::InputEvent zoom_close_brush = {};
    zoom_close_brush.kind = mira::InputKind::kKeyDown;
    zoom_close_brush.button = static_cast<mira::u8>(mira::Key::kEscape);
    mira::guievent(&zoom_paint_gui, {&zoom_close_brush, 1});
    mira::guilayout(&zoom_paint_gui, normal);
    mira::InputEvent zoom_paint_down = {};
    zoom_paint_down.kind = mira::InputKind::kMouseDown;
    zoom_paint_down.x = static_cast<mira::i32>(zoom_paint_gui.layout.viewport.x +
                                               ((40.0F - zoom_paint_gui.view.x) * 2.0F));
    zoom_paint_down.y = static_cast<mira::i32>(zoom_paint_gui.layout.viewport.y +
                                               ((30.0F - zoom_paint_gui.view.y) * 2.0F));
    mira::InputEvent zoom_paint_move = {};
    zoom_paint_move.kind = mira::InputKind::kMouseMove;
    zoom_paint_move.buttons = 1;
    zoom_paint_move.x = static_cast<mira::i32>(zoom_paint_gui.layout.viewport.x +
                                               ((45.0F - zoom_paint_gui.view.x) * 2.0F));
    zoom_paint_move.y = zoom_paint_down.y;
    mira::InputEvent zoom_paint_up = zoom_paint_move;
    zoom_paint_up.kind = mira::InputKind::kMouseUp;
    const mira::InputEvent zoom_paint_events[] = {zoom_paint_down, zoom_paint_move, zoom_paint_up};
    mira::guievent(&zoom_paint_gui, zoom_paint_events);
    MIRA_TEST(zoom_paint_gui.paint_delta.size() == 6);
    MIRA_TEST(close(zoom_paint_gui.paint_delta[0].x, 40.0F));
    MIRA_TEST(close(zoom_paint_gui.paint_delta[5].x, 45.0F));
    MIRA_TEST(close(zoom_paint_gui.paint_delta[0].diameter, 7.0F));

    static mira::GuiState line_gui;
    mira::guiinit(&line_gui);
    mira::guilayout(&line_gui, normal);
    line_gui.view.x = 0.0F;
    line_gui.view.y = 0.0F;
    line_gui.view.zoom = 1.0F;
    mira::guilayout(&line_gui, normal);
    mira::InputEvent line_tool = {};
    line_tool.kind = mira::InputKind::kMouseDown;
    line_tool.x = static_cast<mira::i32>(line_gui.layout.tools.x + 8.0F);
    line_tool.y = static_cast<mira::i32>(line_gui.layout.tools.y + 52.0F);
    mira::guievent(&line_gui, {&line_tool, 1});

    static mira::GuiState line_preview_gui;
    mira::guiinit(&line_preview_gui);
    mira::guilayout(&line_preview_gui, normal);
    line_preview_gui.view.x = 0.0F;
    line_preview_gui.view.y = 0.0F;
    line_preview_gui.view.zoom = 1.0F;
    mira::guilayout(&line_preview_gui, normal);
    mira::InputEvent line_preview_tool = {};
    line_preview_tool.kind = mira::InputKind::kMouseDown;
    line_preview_tool.x = static_cast<mira::i32>(line_preview_gui.layout.tools.x + 8.0F);
    line_preview_tool.y = static_cast<mira::i32>(line_preview_gui.layout.tools.y + 52.0F);
    mira::guievent(&line_preview_gui, {&line_preview_tool, 1});
    line_preview_gui.curtip = 6;
    line_preview_gui.curcoverage = 2;
    static mira::DrawList line_base_draws;
    mira::guiframe(&line_preview_gui, normal, {}, &line_base_draws);
    mira::InputEvent line_preview_down = {};
    line_preview_down.kind = mira::InputKind::kMouseDown;
    line_preview_down.x = static_cast<mira::i32>(line_preview_gui.layout.document.x + 10.0F);
    line_preview_down.y = static_cast<mira::i32>(line_preview_gui.layout.document.y + 20.0F);
    mira::InputEvent line_preview_move = line_preview_down;
    line_preview_move.kind = mira::InputKind::kMouseMove;
    line_preview_move.buttons = 1;
    line_preview_move.x = static_cast<mira::i32>(line_preview_gui.layout.document.x + 15.0F);
    const mira::InputEvent line_preview_events[] = {line_preview_down, line_preview_move};
    static mira::DrawList line_preview_draws;
    mira::guiframe(&line_preview_gui, normal, line_preview_events, &line_preview_draws);
    const mira::usize line_guide_start = line_base_draws.preview_stamps.size();
    MIRA_TEST(line_preview_gui.painting);
    MIRA_TEST(line_preview_gui.paint_delta.empty());
    MIRA_TEST(line_preview_gui.draft_active);
    MIRA_TEST(line_preview_gui.draft_stamps.size() == 6);
    MIRA_TEST(line_preview_draws.preview_stamps.size() == line_guide_start + 6);
    MIRA_TEST(close(line_preview_draws.preview_stamps[line_guide_start].x, 10.0F));
    MIRA_TEST(close(line_preview_draws.preview_stamps[line_guide_start + 5].x, 15.0F));
    MIRA_TEST(close(line_preview_draws.preview_stamps[line_guide_start].diameter, 4.0F));
    MIRA_TEST(close(line_preview_draws.preview_stamps[line_guide_start].tip, 6.0F));
    MIRA_TEST(close(line_preview_draws.preview_stamps[line_guide_start].coverage, 2.0F));
    for (mira::usize stamp = 0; stamp < line_preview_gui.draft_stamps.size(); ++stamp) {
        MIRA_TEST(same_stamp(line_preview_draws.preview_stamps[line_guide_start + stamp],
                             line_preview_gui.draft_stamps[stamp]));
    }
    mira::InputEvent line_preview_up = line_preview_move;
    line_preview_up.kind = mira::InputKind::kMouseUp;
    mira::guievent(&line_preview_gui, {&line_preview_up, 1});
    MIRA_TEST(line_preview_gui.paint_delta.size() == 6);
    MIRA_TEST(!line_preview_gui.draft_active);
    MIRA_TEST(line_preview_gui.draft_stamps.empty());
    for (mira::usize stamp = 0; stamp < 6; ++stamp) {
        MIRA_TEST(same_stamp(line_preview_draws.preview_stamps[line_guide_start + stamp],
                             line_preview_gui.paint_delta[stamp]));
    }

    static mira::GuiState edge_preview_gui;
    mira::guiinit(&edge_preview_gui);
    mira::docnew(&edge_preview_gui, 20, 20);
    mira::guilayout(&edge_preview_gui, normal);
    edge_preview_gui.view.x = 0.0F;
    edge_preview_gui.view.y = 0.0F;
    edge_preview_gui.view.zoom = 1.0F;
    mira::guilayout(&edge_preview_gui, normal);
    mira::InputEvent edge_line_tool = {};
    edge_line_tool.kind = mira::InputKind::kMouseDown;
    edge_line_tool.x = static_cast<mira::i32>(edge_preview_gui.layout.tools.x + 8.0F);
    edge_line_tool.y = static_cast<mira::i32>(edge_preview_gui.layout.tools.y + 52.0F);
    mira::guievent(&edge_preview_gui, {&edge_line_tool, 1});
    edge_preview_gui.curtip = 6;
    edge_preview_gui.curcoverage = 2;
    mira::InputEvent edge_preview_down = {};
    edge_preview_down.kind = mira::InputKind::kMouseDown;
    edge_preview_down.x = static_cast<mira::i32>(edge_preview_gui.layout.document.x + 18.0F);
    edge_preview_down.y = static_cast<mira::i32>(edge_preview_gui.layout.document.y + 10.0F);
    mira::InputEvent edge_preview_move = edge_preview_down;
    edge_preview_move.kind = mira::InputKind::kMouseMove;
    edge_preview_move.buttons = 1;
    edge_preview_move.x = static_cast<mira::i32>(edge_preview_gui.layout.document.x + 24.0F);
    edge_preview_move.y = static_cast<mira::i32>(edge_preview_gui.layout.document.y + 16.0F);
    const mira::InputEvent edge_preview_events[] = {edge_preview_down, edge_preview_move};
    static mira::DrawList edge_preview_draws;
    mira::guiframe(&edge_preview_gui, normal, edge_preview_events, &edge_preview_draws);
    mira::InputEvent edge_preview_up = edge_preview_move;
    edge_preview_up.kind = mira::InputKind::kMouseUp;
    mira::guievent(&edge_preview_gui, {&edge_preview_up, 1});
    MIRA_TEST(edge_preview_draws.preview_stamps.size() == edge_preview_gui.paint_delta.size());
    for (mira::usize stamp = 0; stamp < edge_preview_gui.paint_delta.size(); ++stamp) {
        MIRA_TEST(same_stamp(edge_preview_draws.preview_stamps[stamp],
                             edge_preview_gui.paint_delta[stamp]));
    }

    mira::InputEvent line_down = {};
    line_down.kind = mira::InputKind::kMouseDown;
    line_down.x = static_cast<mira::i32>(line_gui.layout.document.x + 10.0F);
    line_down.y = static_cast<mira::i32>(line_gui.layout.document.y + 20.0F);
    mira::InputEvent line_up = line_down;
    line_up.kind = mira::InputKind::kMouseUp;
    line_up.x = static_cast<mira::i32>(line_gui.layout.document.x + 15.0F);
    const mira::InputEvent line_events[] = {line_down, line_up};
    mira::guievent(&line_gui, line_events);
    MIRA_TEST(line_gui.paint_delta.size() == 6);
    MIRA_TEST(close(line_gui.paint_delta[0].x, 10.0F));
    MIRA_TEST(close(line_gui.paint_delta[5].x, 15.0F));
    MIRA_TEST(line_gui.strokes.size() == 1);

    static mira::GuiState rect_gui;
    mira::guiinit(&rect_gui);
    mira::guilayout(&rect_gui, normal);
    rect_gui.view.x = 0.0F;
    rect_gui.view.y = 0.0F;
    rect_gui.view.zoom = 1.0F;
    mira::guilayout(&rect_gui, normal);
    mira::InputEvent rect_tool = {};
    rect_tool.kind = mira::InputKind::kMouseDown;
    rect_tool.x = static_cast<mira::i32>(rect_gui.layout.tools.x + 8.0F);
    rect_tool.y = static_cast<mira::i32>(rect_gui.layout.tools.y + 100.0F);
    mira::guievent(&rect_gui, {&rect_tool, 1});

    static mira::GuiState rect_preview_gui;
    mira::guiinit(&rect_preview_gui);
    mira::guilayout(&rect_preview_gui, normal);
    rect_preview_gui.view.x = 0.0F;
    rect_preview_gui.view.y = 0.0F;
    rect_preview_gui.view.zoom = 1.0F;
    mira::guilayout(&rect_preview_gui, normal);
    mira::InputEvent rect_preview_tool = {};
    rect_preview_tool.kind = mira::InputKind::kMouseDown;
    rect_preview_tool.x = static_cast<mira::i32>(rect_preview_gui.layout.tools.x + 8.0F);
    rect_preview_tool.y = static_cast<mira::i32>(rect_preview_gui.layout.tools.y + 100.0F);
    mira::guievent(&rect_preview_gui, {&rect_preview_tool, 1});
    rect_preview_gui.curtip = 1;
    rect_preview_gui.curcoverage = 7;
    static mira::DrawList rect_base_draws;
    mira::guiframe(&rect_preview_gui, normal, {}, &rect_base_draws);
    mira::InputEvent rect_preview_down = {};
    rect_preview_down.kind = mira::InputKind::kMouseDown;
    rect_preview_down.x = static_cast<mira::i32>(rect_preview_gui.layout.document.x + 20.0F);
    rect_preview_down.y = static_cast<mira::i32>(rect_preview_gui.layout.document.y + 20.0F);
    mira::InputEvent rect_preview_move = rect_preview_down;
    rect_preview_move.kind = mira::InputKind::kMouseMove;
    rect_preview_move.buttons = 1;
    rect_preview_move.x = static_cast<mira::i32>(rect_preview_gui.layout.document.x + 22.0F);
    rect_preview_move.y = static_cast<mira::i32>(rect_preview_gui.layout.document.y + 22.0F);
    const mira::InputEvent rect_preview_events[] = {rect_preview_down, rect_preview_move};
    static mira::DrawList rect_preview_draws;
    mira::guiframe(&rect_preview_gui, normal, rect_preview_events, &rect_preview_draws);
    const mira::usize rect_guide_start = rect_base_draws.preview_stamps.size();
    MIRA_TEST(rect_preview_gui.painting);
    MIRA_TEST(rect_preview_gui.paint_delta.empty());
    MIRA_TEST(rect_preview_gui.draft_active);
    MIRA_TEST(rect_preview_gui.draft_stamps.size() == 8);
    MIRA_TEST(rect_preview_draws.preview_stamps.size() == rect_guide_start + 8);
    MIRA_TEST(close(rect_preview_draws.preview_stamps[rect_guide_start].x, 20.0F));
    MIRA_TEST(close(rect_preview_draws.preview_stamps[rect_guide_start + 7].x, 22.0F));
    MIRA_TEST(close(rect_preview_draws.preview_stamps[rect_guide_start].diameter, 4.0F));
    MIRA_TEST(close(rect_preview_draws.preview_stamps[rect_guide_start].tip, 1.0F));
    MIRA_TEST(close(rect_preview_draws.preview_stamps[rect_guide_start].coverage, 7.0F));
    for (mira::usize stamp = 0; stamp < rect_preview_gui.draft_stamps.size(); ++stamp) {
        MIRA_TEST(same_stamp(rect_preview_draws.preview_stamps[rect_guide_start + stamp],
                             rect_preview_gui.draft_stamps[stamp]));
    }
    mira::InputEvent rect_preview_up = rect_preview_move;
    rect_preview_up.kind = mira::InputKind::kMouseUp;
    mira::guievent(&rect_preview_gui, {&rect_preview_up, 1});
    MIRA_TEST(rect_preview_gui.paint_delta.size() == 8);
    MIRA_TEST(!rect_preview_gui.draft_active);
    MIRA_TEST(rect_preview_gui.draft_stamps.empty());
    for (mira::usize stamp = 0; stamp < 8; ++stamp) {
        MIRA_TEST(same_stamp(rect_preview_draws.preview_stamps[rect_guide_start + stamp],
                             rect_preview_gui.paint_delta[stamp]));
    }

    static mira::GuiState rect_reverse_gui;
    mira::guiinit(&rect_reverse_gui);
    mira::guilayout(&rect_reverse_gui, normal);
    rect_reverse_gui.view.x = 0.0F;
    rect_reverse_gui.view.y = 0.0F;
    rect_reverse_gui.view.zoom = 1.0F;
    mira::guilayout(&rect_reverse_gui, normal);
    mira::InputEvent rect_reverse_tool = {};
    rect_reverse_tool.kind = mira::InputKind::kMouseDown;
    rect_reverse_tool.x = static_cast<mira::i32>(rect_reverse_gui.layout.tools.x + 8.0F);
    rect_reverse_tool.y = static_cast<mira::i32>(rect_reverse_gui.layout.tools.y + 100.0F);
    mira::guievent(&rect_reverse_gui, {&rect_reverse_tool, 1});
    rect_reverse_gui.curtip = 6;
    rect_reverse_gui.curcoverage = 2;
    mira::InputEvent rect_reverse_down = {};
    rect_reverse_down.kind = mira::InputKind::kMouseDown;
    rect_reverse_down.x = static_cast<mira::i32>(rect_reverse_gui.layout.document.x + 24.0F);
    rect_reverse_down.y = static_cast<mira::i32>(rect_reverse_gui.layout.document.y + 25.0F);
    mira::InputEvent rect_reverse_move = rect_reverse_down;
    rect_reverse_move.kind = mira::InputKind::kMouseMove;
    rect_reverse_move.buttons = 1;
    rect_reverse_move.x = static_cast<mira::i32>(rect_reverse_gui.layout.document.x + 20.0F);
    rect_reverse_move.y = static_cast<mira::i32>(rect_reverse_gui.layout.document.y + 21.0F);
    const mira::InputEvent rect_reverse_events[] = {rect_reverse_down, rect_reverse_move};
    static mira::DrawList rect_reverse_draws;
    mira::guiframe(&rect_reverse_gui, normal, rect_reverse_events, &rect_reverse_draws);
    mira::InputEvent rect_reverse_up = rect_reverse_move;
    rect_reverse_up.kind = mira::InputKind::kMouseUp;
    mira::guievent(&rect_reverse_gui, {&rect_reverse_up, 1});
    MIRA_TEST(rect_reverse_draws.preview_stamps.size() == rect_reverse_gui.paint_delta.size());
    MIRA_TEST(rect_reverse_gui.paint_delta.size() == 16);
    for (mira::usize stamp = 0; stamp < rect_reverse_gui.paint_delta.size(); ++stamp) {
        MIRA_TEST(same_stamp(rect_reverse_draws.preview_stamps[stamp],
                             rect_reverse_gui.paint_delta[stamp]));
    }

    mira::InputEvent rect_down = {};
    rect_down.kind = mira::InputKind::kMouseDown;
    rect_down.x = static_cast<mira::i32>(rect_gui.layout.document.x + 20.0F);
    rect_down.y = static_cast<mira::i32>(rect_gui.layout.document.y + 20.0F);
    mira::InputEvent rect_up = rect_down;
    rect_up.kind = mira::InputKind::kMouseUp;
    rect_up.x = static_cast<mira::i32>(rect_gui.layout.document.x + 22.0F);
    rect_up.y = static_cast<mira::i32>(rect_gui.layout.document.y + 22.0F);
    const mira::InputEvent rect_events[] = {rect_down, rect_up};
    mira::guievent(&rect_gui, rect_events);
    MIRA_TEST(rect_gui.paint_delta.size() >= 8);
    MIRA_TEST(rect_gui.strokes.size() == 1);

    static mira::GuiState magic_gui;
    mira::guiinit(&magic_gui);
    mira::guilayout(&magic_gui, normal);
    magic_gui.view.x = 0.0F;
    magic_gui.view.y = 0.0F;
    mira::guilayout(&magic_gui, normal);
    mira::InputEvent magic_tool = {};
    magic_tool.kind = mira::InputKind::kMouseDown;
    magic_tool.x = static_cast<mira::i32>(magic_gui.layout.tools.x + 8.0F);
    magic_tool.y = static_cast<mira::i32>(magic_gui.layout.tools.y + 76.0F);
    mira::guievent(&magic_gui, {&magic_tool, 1});
    mira::InputEvent magic_down = {};
    magic_down.kind = mira::InputKind::kMouseDown;
    magic_down.x = static_cast<mira::i32>(magic_gui.layout.document.x + 30.0F);
    magic_down.y = static_cast<mira::i32>(magic_gui.layout.document.y + 30.0F);
    mira::guievent(&magic_gui, {&magic_down, 1});
    MIRA_TEST(magic_gui.paint_delta.size() == 1);
    MIRA_TEST(close(magic_gui.paint_delta[0].diameter, 8.0F));
    MIRA_TEST(close(magic_gui.paint_delta[0].tone,
                    static_cast<mira::f32>(mira::tone_value(mira::Tone::kWhite))));

    static mira::GuiState zoom_tool_gui;
    mira::guiinit(&zoom_tool_gui);
    mira::guilayout(&zoom_tool_gui, normal);
    const mira::f32 zoom_before = zoom_tool_gui.view.zoom;
    mira::InputEvent zoom_tool = {};
    zoom_tool.kind = mira::InputKind::kMouseDown;
    zoom_tool.x = static_cast<mira::i32>(zoom_tool_gui.layout.tools.x + 8.0F);
    zoom_tool.y = static_cast<mira::i32>(zoom_tool_gui.layout.tools.y + 124.0F);
    mira::guievent(&zoom_tool_gui, {&zoom_tool, 1});
    mira::InputEvent zoom_click = {};
    zoom_click.kind = mira::InputKind::kMouseDown;
    zoom_click.x = static_cast<mira::i32>(zoom_tool_gui.layout.viewport.x + 80.0F);
    zoom_click.y = static_cast<mira::i32>(zoom_tool_gui.layout.viewport.y + 60.0F);
    mira::guievent(&zoom_tool_gui, {&zoom_click, 1});
    MIRA_TEST(close(zoom_tool_gui.view.zoom, zoom_before * 1.125F));
    MIRA_TEST(zoom_tool_gui.paint_delta.empty());

    static mira::DrawList paint_list;
    mira::guiframe(&gui, normal, {}, &paint_list);
    MIRA_TEST(gui.paint_delta.empty());
    MIRA_TEST(paint_list.icons.size() >= 19);

    static mira::DrawList tiny_list;
    static mira::GuiState tiny_gui;
    mira::guiframe(&tiny_gui, mira::screen_for(1, 1), {}, &tiny_list);
    MIRA_TEST(tiny_list.rects.size() >= 1);
    MIRA_TEST(tiny_list.rects[0].x1 >= tiny_list.rects[0].x0);
    MIRA_TEST(tiny_list.rects[0].y1 >= tiny_list.rects[0].y0);

    const mira::DrawView view = mira::view(list);
    MIRA_TEST(view.rects.size() == list.rects.size());
    MIRA_TEST(view.glyphs.size() == list.glyphs.size());
    MIRA_TEST(view.icons.size() == list.icons.size());
    MIRA_TEST(view.preview_stamps.size() == list.preview_stamps.size());

    list.clear();
    std::array<char, mira::kMaxGlyphs + 100> long_text = {};
    long_text.fill('x');
    MIRA_TEST(!mira::add_text(&list, std::string_view(long_text.data(), long_text.size()), 0.0F,
                              0.0F, mira::Tone::kWhite));
    MIRA_TEST(list.glyphs.overflowed);
    MIRA_TEST(list.overflow_count() == 1);
    return 0;
}
