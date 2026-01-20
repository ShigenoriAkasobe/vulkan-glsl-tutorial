# Step03_Texture

## このステップで学ぶこと

- `VkImage` / `VkImageView` / `VkSampler` の基本
- 画像を GPU にアップロードする最小手順（Staging Buffer → Image）
- `DescriptorSet` による `sampler2D` バインド
- GLSL の `layout(set=0,binding=0)` と Vulkan 側の `VkDescriptorSetLayoutBinding` の対応

## GPU パイプライン上のどの段階を扱うか

- Fragment シェーダーでの **テクスチャサンプリング**
- 「CPU 側でリソースを用意して GPU に渡す」部分（Descriptor 経由）

## Vulkan オブジェクト対応

- Texture: `VkImage` + `VkDeviceMemory` + `VkImageView` + `VkSampler`
- Descriptor: `VkDescriptorSetLayout` / `VkDescriptorPool` / `VkDescriptorSet`
- Pipeline: `VkPipelineLayout` に DescriptorSetLayout を組み込む

## なぜこの設定が必要なのか（設計意図）

- Vulkan はシェーダーリソース（テクスチャ/UBO 等）を **DescriptorSet** で束ねます
- 画像は `VkImage` ですが、そのままではサンプリングできず `VkImageView` が必要です
- `VkSampler` はフィルタ/アドレスモード等を定義する“サンプリング状態”です

## 実行

```bash
./build/steps/Step03_Texture/Step03_Texture
```

この Step は外部画像を使わず、起動時にチェッカーボードを生成してアップロードします。
（stb_image を使ったファイル読み込みに置き換える拡張は `assets/README.md` を参照）
