# Step02_VertexColor

## このステップで学ぶこと

- `layout(location = N)` による Attribute / Varying の指定
- Vertex → Fragment のデータ受け渡し (varying)
- ラスタライザによる **補間**（頂点カラーが面内で滑らかに変化する）

## GPU パイプライン上のどの段階を扱うか

- Vertex シェーダーの出力（varying）
- Rasterizer の補間
- Fragment シェーダーの入力

## Vulkan オブジェクト対応

- Pipeline: `VkPipeline`（Step01 と同じ）
- 今回は頂点バッファをまだ使わず、`gl_VertexIndex` で位置と色を与えます

## なぜこの設定が必要なのか（設計意図）

- Vulkan/GLSL は入力/出力が「location」で明示されます
- `location` は、C++ 側の頂点属性や Fragment 出力との対応関係を“静的に”決めるための鍵です

## 実行

```bash
./build/steps/Step02_VertexColor/Step02_VertexColor
```
