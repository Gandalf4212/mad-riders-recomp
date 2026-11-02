// NativeXRenderer hooks and profile for Mad Riders (XDK 2.0.21076).
//
// Work in progress (docs/native-x-renderer.md): the entry points below are the ones mapped so far, with
// the evidence in that document. Everything is inactive unless one of NativeXRenderer's modes is turned on
// (--native_x_renderer_trace, --native_x_renderer_verify, --native_x_renderer); the Xenos path draws.
//
// Layout: the device structure matches XDK 7978's (Guitar Hero: Metallica) up to the end of the register
// shadow (six dirty masks from +0, command buffer +48, VS/PS float constants +1920/+6016, shadow groups from
// +10368: read in the draw's flush); the block of current objects is 7978's + 368 (index buffer +12812,
// render targets +12816, depth +12832, shaders +13068/+13072: read and written by the setters below). Fields
// marked "unverified" follow from that shift and still need the observer (F10) to confirm them.
#include <native_x_renderer/hooks.h>

#include <native_x_renderer/native_x_renderer.h>

// Draws (PM4 scan: DRAW_INDX with an index buffer / auto-index, and the UP draw that writes vertex fetch 0x48BA)
NATIVE_X_RENDERER_HOOK(82E6EBB8, DrawIndexedVertices)
NATIVE_X_RENDERER_HOOK(82E6E7A0, DrawVertices)
NATIVE_X_RENDERER_HOOK(82E6E2E0, DrawVerticesUP)
// Setters (the device field each writes)
NATIVE_X_RENDERER_HOOK(82E5D518, SetPixelShader)          // +13068
NATIVE_X_RENDERER_HOOK(82E5D720, SetVertexShader)         // +13072
NATIVE_X_RENDERER_HOOK(82E66398, SetIndices)              // +12812
NATIVE_X_RENDERER_HOOK(82E66E70, SetDepthStencilSurface)  // +12832, RB_DEPTH_INFO shadow
// Frame end. The engine calls Swap (82E6FFE0 -> VdSwap) itself once a frame (from 8275FC00); the library's
// own Present wrapper (82E70748) never ran in a race (first trace: no frame summaries). Swap is reported as
// Present, the call that ends a frame. Written out by hand: the FPS overlay counts frames here too.
#include "perf_overlay.h"
extern "C" void __imp__sub_82E6FFE0(PPCContext& __restrict ctx, uint8_t* base);
REX_HOOK_RAW(sub_82E6FFE0) {
  recomp_ui::perf_on_frame();
  if (!::native_x_renderer::detail::g_hooks_active) [[likely]] {
    __imp__sub_82E6FFE0(ctx, base);
    return;
  }
  ::native_x_renderer::detail::Enter(::native_x_renderer::Call::kPresent, ctx, base);
  __imp__sub_82E6FFE0(ctx, base);
  ::native_x_renderer::detail::Leave(::native_x_renderer::Call::kPresent, ctx, base);
}

namespace mad_riders {

const native_x_renderer::Profile& NativeXRendererProfile() {
  static const native_x_renderer::Profile profile = [] {
    native_x_renderer::Profile p;
    p.game = "Mad Riders (58411240)";
    p.xdk_version = "2.0.21076";
    p.output_width = 1280;
    p.output_height = 720;
    native_x_renderer::DeviceLayout& d = p.device;
    d.vs_constants_dirty = 0;       // the draw's flush: ld +0 -> constants at +1920 (0x4000)
    d.ps_constants_dirty = 8;       // ld +8 -> +6016 (0x4400)
    d.fetch_constants_dirty = 24;   // unverified (7978)
    d.fetch_dirty_slot0_bit = 31;   // unverified (7978)
    d.command_buffer_write = 48;
    d.fetch_constants = 1152;       // unverified (7978)
    d.vs_float_constants = 1920;
    d.ps_float_constants = 6016;
    d.bool_loop_constants = 10112;  // unverified (7978)
    d.register_groups = {{
        {10368, 0x2000, 16},
        {10444, 0x2100, 21},
        {10528, 0x2180, 5},
        {10548, 0x2200, 12},
        {10596, 0x2280, 21},
        {10680, 0x2300, 38},
        {10832, 0x2380, 8},
    }};
    d.fence_counter = 10908;        // unverified (7978)
    d.write_back_block = 10896;     // unverified (7978)
    d.flags = 10940;                // unverified (7978)
    d.index_buffer = 12812;
    d.render_targets = 12816;
    d.depth_stencil = 12832;
    d.vertex_buffers = 12836;       // unverified (7978 + 368)
    d.textures = 12920;             // unverified (7978 + 368)
    d.viewport = 13024;             // unverified (7978 + 368)
    d.pixel_shader = 13068;
    d.vertex_shader = 13072;
    d.predication_mask = 13084;     // unverified (7978 + 368)
    d.tile_count = 13124;           // unverified (7978 + 368)
    d.tile_rects = 13128;           // unverified (7978 + 368)
    native_x_renderer::ObjectLayout& o = p.objects;  // unverified (5426/7978 objects)
    o.texture_fetch = 28;
    o.surface_info = 24;
    o.surface_color_depth_info = 28;
    o.surface_size = 36;
    o.surface_width_minus_one = {18, 14};
    o.surface_height_minus_one = {3, 15};
    o.buffer_address = 24;
    o.buffer_size = 28;
    return p;
  }();
  return profile;
}

}  // namespace mad_riders
