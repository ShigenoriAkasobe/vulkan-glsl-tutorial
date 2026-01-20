# Step01_MinimalTriangle

## このステップで学ぶこと

- 最小の Vertex/Fragment シェーダー (GLSL)
- GLSL → SPIR-V → `VkShaderModule`
- Graphics Pipeline の基本構成 (`VkPipelineLayout`, `VkPipeline`)
- `vkCmdBindPipeline` + `vkCmdDraw` の最小ループ

## GPU パイプライン上のどの段階を扱うか

- Vertex → Raster → Fragment → Framebuffer の最小経路
- まだ Vertex Buffer は使いません（`gl_VertexIndex` で位置を決める）

## Vulkan オブジェクト対応

- Shader: `VkShaderModule`
- Pipeline: `VkPipelineLayout`, `VkPipeline`
- Render: `VkRenderPass`, `VkFramebuffer`
- Command: `VkCommandBuffer` 内で `vkCmdBindPipeline` / `vkCmdDraw`

## なぜこの設定が必要なのか（設計意図）

- Vulkan は「固定機能 + シェーダー」をまとめた状態を `VkPipeline` として事前に作成します
- `VkPipelineLayout` は DescriptorSet や PushConstant の“レイアウト宣言”。この Step では空でも必須です
- `gl_VertexIndex` を使うことで、まずは **パイプラインの流れ**に集中できます

## ビルド & 実行

```bash
./build/steps/Step01_MinimalTriangle/Step01_MinimalTriangle
```

シェーダーはビルド時にコンパイルされ、実行ファイル隣の `shaders/` に配置されます。
