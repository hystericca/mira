#include "mira/web/web.hpp"

#include <emscripten/emscripten.h>

#include <algorithm>
#include <string_view>

namespace {

mira::Web *g_web_app = nullptr;

} // namespace

// clang-format off
EM_JS(void, mira_open_image_picker, (int doc_width, int doc_height), {
    const targetW = Math.max(1, doc_width | 0);
    const targetH = Math.max(1, doc_height | 0);
    if (!Module.miraImageInput) {
        const input = document.createElement("input");
        input.type = "file";
        input.accept = "image/*";
        input.style.display = "none";
        document.body.appendChild(input);
        Module.miraImageInput = input;
    }
    const input = Module.miraImageInput;
    input.onchange = async function() {
        const file = input.files && input.files[0];
        if (!file) {
            return;
        }
        let bitmap = null;
        try {
            bitmap = await createImageBitmap(file);
            const canvas = typeof OffscreenCanvas !== 'undefined'
                ? new OffscreenCanvas(targetW, targetH)
                : document.createElement("canvas");
            canvas.width = targetW;
            canvas.height = targetH;
            const ctx = canvas.getContext("2d", { willReadFrequently: true });
            ctx.fillStyle = "#fff";
            ctx.fillRect(0, 0, targetW, targetH);
            ctx.imageSmoothingEnabled = true;
            ctx.imageSmoothingQuality = "high";
            const scale = Math.min(targetW / Math.max(1, bitmap.width),
                                   targetH / Math.max(1, bitmap.height));
            const w = Math.max(1, Math.floor(bitmap.width * scale));
            const h = Math.max(1, Math.floor(bitmap.height * scale));
            const x = Math.floor((targetW - w) * 0.5);
            const y = Math.floor((targetH - h) * 0.5);
            ctx.drawImage(bitmap, x, y, w, h);
            const rgba = ctx.getImageData(0, 0, targetW, targetH).data;
            const pixels = new Uint8Array(targetW * targetH);
            for (let p = 0, r = 0; p < pixels.length; ++p, r += 4) {
                const alpha = rgba[r + 3] / 255.0;
                const luma =
                    ((rgba[r] * 0.299) + (rgba[r + 1] * 0.587) + (rgba[r + 2] * 0.114)) * alpha +
                    (255.0 * (1.0 - alpha));
                pixels[p] = Math.max(0, Math.min(255, Math.round(luma)));
            }
            const data = _malloc(pixels.length);
            HEAPU8.set(pixels, data);
            const filename = file.name || "Image";
            const dot = filename.lastIndexOf(".");
            const stem = (dot > 0 ? filename.slice(0, dot) : filename) || "Image";
            const nameLen = lengthBytesUTF8(stem) + 1;
            const name = _malloc(nameLen);
            stringToUTF8(stem, name, nameLen);
            _mira_import_image_ready(targetW, targetH, data, pixels.length, name);
            _free(name);
            _free(data);
        } catch (error) {
        } finally {
            if (bitmap && bitmap.close) {
                bitmap.close();
            }
            input.value = "";
        }
    };
    input.value = "";
    input.click();
});
// clang-format on

extern "C" EMSCRIPTEN_KEEPALIVE void mira_import_image_ready(int width, int height,
                                                             const unsigned char *pixels,
                                                             int byte_count, const char *name) {
    if (g_web_app == nullptr || pixels == nullptr || byte_count <= 0) {
        return;
    }
    const std::string_view layer_name =
        name == nullptr ? std::string_view{} : std::string_view(name);
    g_web_app->import_image_ready(static_cast<mira::i32>(width), static_cast<mira::i32>(height),
                                  reinterpret_cast<const mira::u8 *>(pixels),
                                  static_cast<mira::usize>(byte_count), layer_name);
}

namespace mira {

void Web::install_file_import() { g_web_app = this; }

void Web::open_image_picker() { mira_open_image_picker(gui_.document.width, gui_.document.height); }

auto Web::upload_layer(u8 slot, const u8 *pixels, usize byte_count) -> b8 {
    const u32 document_width = static_cast<u32>(std::max(1, gui_.document.width));
    const u32 document_height = static_cast<u32>(std::max(1, gui_.document.height));
    const usize expected = static_cast<usize>(document_width) * static_cast<usize>(document_height);
    if (slot >= kMaxLayers || layer_texture_ == nullptr || pixels == nullptr ||
        byte_count < expected) {
        return false;
    }

    wgpu::TexelCopyTextureInfo destination = {};
    destination.texture = layer_texture_;
    destination.mipLevel = 0;
    destination.origin = {0, 0, static_cast<u32>(slot)};
    destination.aspect = wgpu::TextureAspect::All;

    wgpu::TexelCopyBufferLayout layout = {};
    layout.offset = 0;
    layout.bytesPerRow = document_width;
    layout.rowsPerImage = document_height;

    wgpu::Extent3D size = {document_width, document_height, 1};
    queue_.WriteTexture(&destination, pixels, expected, &layout, &size);
    return true;
}

void Web::import_image_ready(i32 width, i32 height, const u8 *pixels, usize byte_count,
                             std::string_view name) {
    if (width != gui_.document.width || height != gui_.document.height) {
        return;
    }
    const u8 index = layerimage(&gui_, name);
    if (index == kNoLayer || index >= gui_.layers.size()) {
        return;
    }
    if (!upload_layer(gui_.layers[index].texture_slot, pixels, byte_count)) {
        (void)layerdel(&gui_);
        return;
    }
    draw_dirty_ = true;
}

} // namespace mira
