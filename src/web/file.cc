#include "mira/web/web.hpp"

#include <emscripten/emscripten.h>

#include <algorithm>
#include <array>
#include <limits>
#include <string_view>
#include <vector>

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

EM_JS(void, mira_download_png, (int width, int height, const unsigned char *rgba, int byte_count), {
    const w = Math.max(1, width | 0);
    const h = Math.max(1, height | 0);
    const expected = w * h * 4;
    if (!rgba || byte_count < expected) {
        return;
    }
    const canvas = document.createElement("canvas");
    canvas.width = w;
    canvas.height = h;
    const ctx = canvas.getContext("2d");
    const copy = new Uint8ClampedArray(HEAPU8.subarray(rgba, rgba + expected));
    ctx.putImageData(new ImageData(copy, w, h), 0, 0);
    canvas.toBlob(function(blob) {
        if (!blob) {
            return;
        }
        const url = URL.createObjectURL(blob);
        const link = document.createElement("a");
        link.href = url;
        link.download = "mira.png";
        document.body.appendChild(link);
        link.click();
        link.remove();
        setTimeout(function() {
            URL.revokeObjectURL(url);
        }, 0);
    }, "image/png");
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
namespace {

[[nodiscard]] auto align_to(u32 value, u32 alignment) -> u32 {
    return ((value + alignment - 1U) / alignment) * alignment;
}

[[nodiscard]] auto bayer4(u32 x, u32 y) -> f32 {
    constexpr std::array<f32, 16> cells = {
        0.0F, 8.0F,  2.0F,  10.0F,
        12.0F, 4.0F, 14.0F, 6.0F,
        3.0F, 11.0F, 1.0F,  9.0F,
        15.0F, 7.0F, 13.0F, 5.0F,
    };
    return (cells[((y & 3U) * 4U) + (x & 3U)] + 0.5F) / 16.0F;
}

[[nodiscard]] auto sample_layer(const u8 *layers, usize layer_index, usize layer_bytes,
                                u32 bytes_per_row, u32 x, u32 y) -> f32 {
    const usize offset = (layer_index * layer_bytes) + (static_cast<usize>(y) * bytes_per_row) + x;
    return static_cast<f32>(layers[offset]) / 255.0F;
}

[[nodiscard]] auto mix(f32 a, f32 b, f32 t) -> f32 { return a + ((b - a) * t); }

} // namespace

void Web::install_file_import() { g_web_app = this; }

void Web::open_image_picker() { mira_open_image_picker(gui_.document.width, gui_.document.height); }

void Web::export_png() {
    const u32 document_width = static_cast<u32>(std::max(1, gui_.document.width));
    const u32 document_height = static_cast<u32>(std::max(1, gui_.document.height));
    const usize layer_count = gui_.layers.size();
    if (layer_count == 0 || device_ == nullptr || queue_ == nullptr || instance_ == nullptr ||
        layer_texture_ == nullptr) {
        return;
    }

    const u32 bytes_per_row = align_to(document_width, 256U);
    const usize layer_bytes = static_cast<usize>(bytes_per_row) * document_height;
    const usize read_size = layer_bytes * layer_count;

    wgpu::BufferDescriptor buffer_descriptor = {};
    buffer_descriptor.size = read_size;
    buffer_descriptor.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead;
    wgpu::Buffer readback = device_.CreateBuffer(&buffer_descriptor);
    if (readback == nullptr) {
        return;
    }

    wgpu::CommandEncoder encoder = device_.CreateCommandEncoder();
    if (encoder == nullptr) {
        return;
    }
    for (usize index = 0; index < layer_count; ++index) {
        const Layer &layer = gui_.layers[index];
        wgpu::TexelCopyTextureInfo source = {};
        source.texture = layer_texture_;
        source.mipLevel = 0;
        source.origin = {0, 0, static_cast<u32>(layer.texture_slot)};
        source.aspect = wgpu::TextureAspect::All;

        wgpu::TexelCopyBufferLayout layout = {};
        layout.offset = layer_bytes * index;
        layout.bytesPerRow = bytes_per_row;
        layout.rowsPerImage = document_height;

        wgpu::TexelCopyBufferInfo destination = {};
        destination.buffer = readback;
        destination.layout = layout;

        wgpu::Extent3D extent = {document_width, document_height, 1};
        encoder.CopyTextureToBuffer(&source, &destination, &extent);
    }

    wgpu::CommandBuffer commands = encoder.Finish();
    if (commands == nullptr) {
        return;
    }
    queue_.Submit(1, &commands);

    instance_.WaitAny(
        readback.MapAsync(
            wgpu::MapMode::Read, 0, read_size, wgpu::CallbackMode::WaitAnyOnly,
            [&](wgpu::MapAsyncStatus status, wgpu::StringView) {
                if (status != wgpu::MapAsyncStatus::Success) {
                    return;
                }
                const auto *mapped = static_cast<const u8 *>(readback.GetConstMappedRange());
                if (mapped == nullptr) {
                    readback.Unmap();
                    return;
                }

                std::vector<u8> rgba(static_cast<usize>(document_width) * document_height * 4U);
                for (u32 y = 0; y < document_height; ++y) {
                    for (u32 x = 0; x < document_width; ++x) {
                        f32 luma = 0.0F;
                        for (usize step = 0; step < layer_count; ++step) {
                            const usize layer_index = layer_count - 1U - step;
                            const Layer &layer = gui_.layers[layer_index];
                            if (!layervisible(layer)) {
                                continue;
                            }
                            const f32 opacity = static_cast<f32>(layer.opacity_u8) / 255.0F;
                            if (layer.kind == LayerKind::kBackground) {
                                luma = mix(luma, 1.0F, opacity);
                            } else if (layer.kind == LayerKind::kImage) {
                                const f32 image =
                                    sample_layer(mapped, layer_index, layer_bytes, bytes_per_row, x, y);
                                luma = mix(luma, image, opacity);
                            } else {
                                const f32 ink =
                                    sample_layer(mapped, layer_index, layer_bytes, bytes_per_row, x, y);
                                luma = mix(luma, 0.0F, ink * opacity);
                            }
                        }
                        const u8 value =
                            std::clamp(luma, 0.0F, 1.0F) >= bayer4(x, y) ? 255U : 0U;
                        const usize out = ((static_cast<usize>(y) * document_width) + x) * 4U;
                        rgba[out + 0U] = value;
                        rgba[out + 1U] = value;
                        rgba[out + 2U] = value;
                        rgba[out + 3U] = 255U;
                    }
                }
                mira_download_png(static_cast<int>(document_width), static_cast<int>(document_height),
                                  rgba.data(), static_cast<int>(rgba.size()));
                readback.Unmap();
            }),
        std::numeric_limits<u64>::max());
}

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
