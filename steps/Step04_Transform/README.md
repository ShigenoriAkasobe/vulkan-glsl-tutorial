# Step04_Transform

## このステップで学ぶこと

- `UniformBuffer` (UBO) を使って CPU→GPU に定数データを渡す
- `VkBuffer` + `VkDeviceMemory` の確保と `vkMapMemory` による更新
- GLSL の `layout(set=0,binding=0)` と Vulkan 側 Descriptor の対応

## GPU パイプライン上のどの段階を扱うか

- Vertex シェーダーでの座標変換（Model/View/Projection）
- CPU が毎フレーム更新する“定数データ”の流れ

## Vulkan オブジェクト対応

- Uniform: `VkBuffer` / `VkDeviceMemory`
- Descriptor: `VkDescriptorSetLayout` / `VkDescriptorPool` / `VkDescriptorSet`
- Pipeline: `VkPipelineLayout` に UBO 用の set layout を接続

## なぜこの設定が必要なのか（設計意図）

- 変換行列は頂点ごとに変わらないので、毎頂点属性で渡すより UBO が自然です
- Vulkan は UBO を “バッファ + DescriptorSet” 経由でバインドする設計です

## 実行

```bash
./build/steps/Step04_Transform/Step04_Transform
```
