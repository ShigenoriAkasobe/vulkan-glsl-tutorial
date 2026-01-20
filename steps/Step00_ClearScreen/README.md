# Step00_ClearScreen

## このステップで学ぶこと

- GLFW でウィンドウと `VkSurfaceKHR` を作る
- Vulkan の最小構成 (Instance / PhysicalDevice / Device / Queue)
- Swapchain を作って画面に出す流れ
- `VkCommandBuffer` に「RenderPass 開始 → Clear → 終了」だけを記録する
- GPU と CPU の同期 (Semaphore / Fence) の最小形

## GPU パイプライン上のどの段階を扱うか

- このステップは **Framebuffer / Present** 側に集中します
- Graphics Pipeline (Vertex/Fragment) はまだ作りません（シェーダー無し）

## Vulkan オブジェクト対応

- Window/Surface: `GLFWwindow`, `VkSurfaceKHR`
- Swapchain: `VkSwapchainKHR`, `VkImage`, `VkImageView`
- Render: `VkRenderPass`, `VkFramebuffer`
- Command: `VkCommandPool`, `VkCommandBuffer`
- Sync: `VkSemaphore`, `VkFence`

## なぜこの設定が必要なのか（設計意図）

- Swapchain は「画面に出すための画像の列」で、Acquire/Present の同期が必須です
- RenderPass は「どの画像をどう扱うか(Load/Store, Layout)」を宣言し、Clear もここで定義します
- CommandBuffer は GPU 実行のための「命令列」。最小でも RenderPass 開始/終了が必要です

## ビルド & 実行

ルート README の手順でビルド後、以下で実行します。

```bash
./build/steps/Step00_ClearScreen/Step00_ClearScreen
```
