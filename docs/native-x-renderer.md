# NativeXRenderer in Mad Riders (work in progress)

Branch `native-x-renderer`. [NativeXRenderer](https://github.com/ElGuslml/NativeXRenderer) draws the game's
Direct3D calls on the host (Vulkan, DiligentCore) instead of emulating the Xenos GPU. It is an optional second
renderer here: the ReXGlue SDK stays v0.10.0 and its Xenos path stays the default and the fallback.

## Build
- `thirdparty/native_x_renderer`: submodule (pinned commit), with DiligentCore inside it.
- `patches/rexglue-sdk/`: NativeXRenderer's eight SDK changes (draw observer, sync-only command processor,
  queries, gamma, fractional scale, renderer thread signals, RequestRanges, swap count), taken from
  ElGuslml/rexglue-sdk's `recomp` branch (changes 26-33), plus change 19 (lazy resolves: the texture cache's
  `TryLoadTextureDataFromResolveCache`, which NativeXRenderer overrides; its D3D12 part is off by default). They
  apply cleanly to v0.10.0.
- CMake: `-DMADRIDERS_NATIVE_X_RENDERER=ON -DREXSDK_DIR=<patched SDK source tree>`.
- CI: `.github/workflows/build-windows-nxr.yml` (artifact `mad_riders-nxr-win-amd64`).

## The executable
XDK 2.0.21076 (newer than every game NativeXRenderer has mapped: 2099, 2417, 5426, 7978). The D3D library is at
`0x82E5B000`-`0x82E84000`.

| Entry point | Address | Evidence |
|---|---|---|
| DrawIndexedVertices | `82E6EBB8` | DRAW_INDX with index data; flush reads masks +0/+8 and constants +1920/+6016; reads index buffer +12812 |
| DrawVertices | `82E6E7A0` | same flush, auto-index DRAW_INDX |
| DrawVerticesUP | `82E6E2E0` | writes vertex fetch constant 0x48BA, (count, stride) arguments |
| SetPixelShader / SetVertexShader | `82E5D518` / `82E5D720` | write +13068 / +13072 |
| SetIndices | `82E66398` | writes +12812 |
| SetDepthStencilSurface | `82E66E70` | writes +12832 and the RB_DEPTH_INFO shadow |
| Swap | `82E6FFE0` | calls VdSwap (hooked by the FPS overlay, so not by NativeXRenderer) |
| Present | `82E70748` | engine call once a frame; sync `82E6FB20`, then Swap |

Device layout: XDK 7978's up to the register shadow (confirmed in the draw's flush), the current-object block
7978's + 368 (index buffer, targets, depth, shaders confirmed by their setters; the rest unverified).

## Still to map
SetRenderTarget, Clear/ClearF, Resolve, BeginTiling/EndTiling, SetPredication, SetViewport/SetScissorRect,
SetVertexDeclaration, SetStreamSource, SetTexture, the constant setters, buffer Lock/Unlock, TextureUnlock,
KickSegment/NewSegment, SetShaderGPRAllocation, queries, BeginVertices/EndVertices; the vertex declaration
offset; the resource objects' layout.

## Testing (needs the player's PC)
1. `mad_riders.exe --native_x_renderer_trace=true`: play a race; F9 dumps a frame into the log.
2. `mad_riders.exe --native_x_renderer_verify=true`: F10 verifies a frame; the report lands in the log about
   10 frames later.
Send the log. The renderer itself (`--native_x_renderer=true`) is not ready.
