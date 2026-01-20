# Step05_LightingBasic

## このステップで学ぶこと

- 法線ベクトルの扱い（今回は簡略化してシェーダー内で固定法線）
- Lambert 拡散反射の最小実装：$\max(0, n \cdot l)$
- UBO に複数の値（行列 + ライト方向 + 色）をまとめて渡す

補足: UBO のレイアウトは `std140` を意識する必要があります。`vec3` は 16byte アラインメントになるため、このステップでは C++/GLSL ともに `vec4` 相当で扱い、余計なパディング事故を避けています。

## GPU パイプライン上のどの段階を扱うか

- Vertex: 変換（MVP） + 法線の供給
- Fragment: 照明計算（Lambert）

## Vulkan オブジェクト対応

- Step04 と同じく UBO + Descriptor
- 追加は「UBO の中身（データ構造）」と「シェーダー側の計算」だけ

## 実行

```bash
./build/steps/Step05_LightingBasic/Step05_LightingBasic
```
