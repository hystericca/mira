#include "mira/web/web.hpp"
#include <emscripten/html5.h>

namespace {

mira::Web app;

EM_BOOL frame(double, void *user_data) {
    static_cast<mira::Web *>(user_data)->frame();
    return EM_TRUE;
}

} // namespace

auto main() -> int {
    if (!app.init()) {
        return 1;
    }

    emscripten_request_animation_frame_loop(&frame, &app);
    return 0;
}
