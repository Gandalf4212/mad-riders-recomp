# Issue for ElGuslml/NativeXRenderer (to post upstream)

**Title:** Draws split into several DRAW_INDX packets in one call (> 65532 indices); XDK 2.0.21076 map (Mad Riders)

---

Hi, thanks for NativeXRenderer. I integrated it into a ReXGlue recompilation of **Mad Riders** (XBLA, title
58411240, **XDK 2.0.21076**, newer than the versions in `xdk-d3d-reference.md`). It is now the game's default renderer:
the observer is at 100 % in races and in the garage, and the game runs at about 3x the frame rate of the Xenos path.
Two things that may be useful to the library.

## 1. One D3D call, several draw packets: only the last one is drawn

The library splits `DrawIndexedVertices` with more than 65532 indices into several `DRAW_INDX` packets inside the same
call (each of at most 65532 indices, `VGT_DMA_BASE` advanced). NativeXRenderer takes one draw packet per call: the
packet scan (`ScanDraw` in `state.cpp`) keeps the last `VGT_DRAW_INITIATOR` / `VGT_DMA_BASE` / `VGT_DMA_SIZE` written,
so the renderer draws only the last piece.

- **Symptom:** in the garage the rider's body (one draw of 71322 indices, triangle list) showed only its last 5790
  indices: helmet, gloves and boots. In races the lower-detail model fits in one packet and draws correctly.
- **Observer evidence:** the call was executed twice (`executions per call: ... 2x:2`), and the only mismatches
  were `VGT_DMA_BASE` (cpu `0x19546000`, gpu `0x19565ff8`), `VGT_DMA_SIZE` (cpu `0x4000fffc`, gpu `0x4000169e`) and
  `VGT_DRAW_INITIATOR` (cpu `0xfffc0004`, gpu `0x169e0004`).
- **Game-side workaround (works):** the game's `DrawIndexedVertices` hook issues a list draw (point, line,
  triangle, rect or quad list) of more than 65532 indices as several calls of at most 65532, which the library
  then emits as one packet each. Mad Riders' `src/native_x_renderer_mad_riders.cpp` has it.
- **Suggestion:** handle every draw packet a call writes, in the observer and in the renderer (one host draw per
  packet, or one draw over the whole index range when the pieces are contiguous). This would make it work without
  a per-game hook, including for strips, which the workaround leaves alone.

## 2. XDK 2.0.21076 map (Mad Riders)

Device layout: XDK 7978's (Guitar Hero: Metallica) up to the end of the register shadow, with these differences.

**Device fields that moved:**

| Field | Offset | How it moved |
|---|---|---|
| Fence counter | `+11036` | 7978 + 128 |
| Write-back block | `+11024` | 7978 + 128 |
| Flag bytes | `+11068` | 7978 + 128 |
| Vertex declaration | `+12240` | 7978 + 416 |
| BeginVertices' saved end (data +13848, dwords +13856) | `+13844` | 7978 + 400 |

The current-object block is 7978's + 368: index buffer `+12812`, targets `+12816`, depth `+12832`, streams `+12836`,
textures `+12920`, viewport `+13024`, scissor `+13052`, pixel/vertex shader `+13068`/`+13072`, predication `+13084`,
tile count/rects `+13124`/`+13128`. Resource objects as 5426/7978.

**Entry points:**

| Call | Address |
|---|---|
| DrawIndexedVertices | `82E6EBB8` |
| DrawVertices | `82E6E7A0` |
| BeginVertices | `82E6E2E0` |
| EndVertices | `82E6E2D0`, a real function that the engine calls |
| DrawVerticesUP | none in this library |
| SetRenderTarget | `82E66AE0` (engine alias `82E67148`) |
| SetDepthStencilSurface | `82E66E70` |
| Clear / ClearF | `82E6E210` / `82E6E0D8` |
| Resolve | `82E72558` (body `82E71400`) |
| SetPredication | `82E70860` |
| SetViewport / SetScissorRect | `82E669B0` / `82E660F8` |
| SetPixelShader / SetVertexShader | `82E5D518` / `82E5D720` |
| SetVertexDeclaration | `82E5D938` |
| SetStreamSource | `82E661F0` |
| SetIndices | `82E66398` |
| SetTexture | `82E69DB8` |
| Set{Vertex,Pixel}ShaderConstantF | `82E5D170` / `82E5D248` (dirty mask in r7, as 7978) |
| VB Lock/Unlock | `82E63AB0` / `82E62998` |
| IB Lock/Unlock | `82E63B00` / `82E62A58` |
| TextureUnlock | `82E67E30` |
| FlushCpuRange | `82E74B28` |
| KickSegment / NewSegment | `82E73FB8` / `82E72ED8` |
| SetShaderGPRAllocation | `82E5DAC0` |
| QueryIssue / QueryGetData | `82E6B090` / `82E6B340` |
| Swap | `82E6FFE0`; reported as Present: the library's Present `82E70748` is not used by the engine |

BeginTiling/EndTiling exist (likely `82E76A30`/`82E77198`) but the engine never calls them.

**Other notes:**
- **The scene is drawn at 1280x672, not the output's 1280x720.** So D101 took every scene draw for an offscreen one
  and made it wait for its pipeline: players with an empty pipeline storage saw long hitches. Mad Riders carries a
  small patch adding `Profile::scene_width/scene_height` (draws into a target of that size count as scene draws);
  something like it upstream would be welcome (`patches/native_x_renderer/0001-scene-size-and-scene-antialiasing.patch`).
- **Antialiasing:** the scene's final copy is 1280x672 and is drawn into the 1280x720 frame before the HUD, and every
  resolve has the same return addresses. The same patch adds `Profile::scene_resolve_before_output`: the newest
  scene-size copy from `scene_resolve_caller` is antialiased at the first draw into an output-size target.
- Every Resolve goes through one engine wrapper (return address `0x82761F30`, outer caller `0x82762690` for all of
  them), so `scene_resolve_caller` cannot pick the scene copy by return address in this game.
- The evidence for each entry is in Mad Riders' `docs/native-x-renderer.md`, which I can share.
