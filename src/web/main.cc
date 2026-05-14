#include "mira/web/web.hpp"
#include <emscripten/emscripten.h>

namespace {

mira::Web app;

void frame(void *user_data) { static_cast<mira::Web *>(user_data)->frame(); }

} // namespace

auto main() -> int {
    if (!app.init()) {
        return 1;
    }

    emscripten_set_main_loop_arg(&frame, &app, 0, false);
    return 0;
}
