#include "mira/draw/draw.hpp"
#include "mira/gui/gui.hpp"
#include "test_support.hpp"

#include <array>
#include <string_view>
#include <type_traits>

auto main() -> int {
    static_assert(sizeof(mira::RectDraw) == 32);
    static_assert(sizeof(mira::GlyphDraw) == 32);
    static_assert(sizeof(mira::IconDraw) == 32);
    static_assert(sizeof(mira::Layer) == 24);
    static_assert(sizeof(mira::Tool) == 16);
    static_assert(sizeof(mira::InputEvent) == 12);
    static_assert(sizeof(mira::HitRecord) == 24);
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

    mira::DrawList packing;
    MIRA_TEST(
        mira::add_rect(&packing, {.x = 1.0F, .y = 2.0F, .width = 3.0F, .height = 4.0F},
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
    MIRA_TEST(mira::add_icon(&packing, mira::Icon::kPen, 7.0F, 8.0F, mira::Tone::kBlack));
    MIRA_TEST(packing.icons[0].x == 7.0F);
    MIRA_TEST(packing.icons[0].y == 8.0F);
    MIRA_TEST(packing.icons[0].code == 0.0F);
    MIRA_TEST(packing.icons[0].tone == mira::tone_value(mira::Tone::kBlack));

    mira::GuiState gui;
    mira::init_gui(&gui);
    MIRA_TEST(gui.tools.size() == 7);
    MIRA_TEST(mira::tool_name(gui.tools[0]) == "Pen");
    MIRA_TEST(mira::tool_name(gui.tools[1]) == "Brush");
    MIRA_TEST(mira::tool_name(gui.tools[6]) == "Erase");
    MIRA_TEST(gui.tools[0].selected == 1);
    MIRA_TEST(mira::selected_tool(gui) == &gui.tools[0]);
    MIRA_TEST(gui.layers.size() == 3);
    MIRA_TEST(mira::layer_name(gui.layers[0]) == "Ink");
    MIRA_TEST(mira::layer_name(gui.layers[1]) == "Reference");
    MIRA_TEST(mira::layer_name(gui.layers[2]) == "Paper");
    MIRA_TEST(gui.layers[0].selected == 1);
    MIRA_TEST(gui.layers[1].opacity_u8 == 112);
    MIRA_TEST(mira::selected_layer(gui) == &gui.layers[0]);

    mira::layout_gui(&gui, normal);
    MIRA_TEST(gui.layout.menu_bar.x == 0.0F);
    MIRA_TEST(gui.layout.menu_bar.y == 0.0F);
    MIRA_TEST(gui.layout.menu_bar.width == 560.0F);
    MIRA_TEST(gui.layout.menu_bar.height == 14.0F);
    MIRA_TEST(gui.layout.toolbar.x == 0.0F);
    MIRA_TEST(gui.layout.toolbar.width == 28.0F);
    MIRA_TEST(gui.layout.sidebar.x == 448.0F);
    MIRA_TEST(gui.layout.sidebar.width == 112.0F);
    MIRA_TEST(gui.layout.canvas.x == 28.0F);
    MIRA_TEST(gui.layout.canvas.y == 14.0F);
    MIRA_TEST(gui.layout.canvas.width == 420.0F);

    const mira::HitRecord menu_hit = mira::hit_at(gui, 8, 5);
    MIRA_TEST(menu_hit.kind == mira::HitKind::kMenu);
    MIRA_TEST(menu_hit.index == 0);
    const mira::HitRecord tool_hit = mira::hit_at(gui, 8, 24);
    MIRA_TEST(tool_hit.kind == mira::HitKind::kTool);
    MIRA_TEST(tool_hit.index == 0);
    const mira::HitRecord canvas_hit = mira::hit_at(gui, 80, 40);
    MIRA_TEST(canvas_hit.kind == mira::HitKind::kCanvas);
    const mira::HitRecord visibility_hit =
        mira::hit_at(gui, static_cast<mira::i32>(gui.layout.layer_list.x + 6.0F),
                     static_cast<mira::i32>(gui.layout.layer_list.y + 7.0F));
    MIRA_TEST(visibility_hit.kind == mira::HitKind::kLayerVisibility);
    MIRA_TEST(visibility_hit.priority > 80);

    mira::InputEvent toggle_visibility = {};
    toggle_visibility.kind = mira::InputKind::kMouseDown;
    toggle_visibility.x = static_cast<mira::i32>(gui.layout.layer_list.x + 6.0F);
    toggle_visibility.y = static_cast<mira::i32>(gui.layout.layer_list.y + 7.0F);
    mira::reduce_gui(&gui, {&toggle_visibility, 1});
    MIRA_TEST(gui.layers[0].visible == 0);

    mira::InputEvent select_reference = {};
    select_reference.kind = mira::InputKind::kMouseDown;
    select_reference.x = static_cast<mira::i32>(gui.layout.layer_list.x + 20.0F);
    select_reference.y = static_cast<mira::i32>(gui.layout.layer_list.y + 28.0F);
    mira::reduce_gui(&gui, {&select_reference, 1});
    MIRA_TEST(gui.selected_layer == 1);
    MIRA_TEST(gui.layers[1].selected == 1);
    MIRA_TEST(gui.layers[0].selected == 0);

    mira::InputEvent select_line = {};
    select_line.kind = mira::InputKind::kMouseDown;
    select_line.x = static_cast<mira::i32>(gui.layout.tool_list.x + 8.0F);
    select_line.y = static_cast<mira::i32>(gui.layout.tool_list.y + 40.0F);
    mira::reduce_gui(&gui, {&select_line, 1});
    MIRA_TEST(gui.selected_tool == 2);
    MIRA_TEST(gui.tools[2].selected == 1);
    MIRA_TEST(gui.tools[0].selected == 0);

    mira::DrawList list;
    mira::build_gui_frame(&gui, normal, {}, &list);
    MIRA_TEST(list.rects.size() >= 5);
    MIRA_TEST(list.glyphs.size() > 0);
    MIRA_TEST(list.icons.size() == 7);
    MIRA_TEST(list.upload_bytes() ==
              list.rects.byte_size() + list.glyphs.byte_size() + list.icons.byte_size());
    MIRA_TEST(list.overflow_count() == 0);
    MIRA_TEST(list.rects.size() <= 64);
    MIRA_TEST(list.glyphs.size() <= 64);

    mira::DrawList tiny_list;
    mira::GuiState tiny_gui;
    mira::build_gui_frame(&tiny_gui, mira::screen_for(1, 1), {}, &tiny_list);
    MIRA_TEST(tiny_list.rects.size() >= 1);
    MIRA_TEST(tiny_list.rects[0].x1 >= tiny_list.rects[0].x0);
    MIRA_TEST(tiny_list.rects[0].y1 >= tiny_list.rects[0].y0);

    const mira::DrawView view = mira::view(list);
    MIRA_TEST(view.rects.size() == list.rects.size());
    MIRA_TEST(view.glyphs.size() == list.glyphs.size());
    MIRA_TEST(view.icons.size() == list.icons.size());

    list.clear();
    std::array<char, mira::kMaxGlyphs + 100> long_text = {};
    long_text.fill('x');
    MIRA_TEST(!mira::add_text(&list, std::string_view(long_text.data(), long_text.size()), 0.0F,
                              0.0F, mira::Tone::kWhite));
    MIRA_TEST(list.glyphs.overflowed);
    MIRA_TEST(list.overflow_count() == 1);
    return 0;
}
