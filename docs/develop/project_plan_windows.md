あなたはプロフェッショナルなグラフィックスプログラマです。
私は Vulkan と GLSL を用いて、GPU パイプラインを体系的に学ぶための
「最小構成の Vulkan + GLSL チュートリアルプロジェクト」を GitHub 用に作りたいです。

# 開発環境：
- OS: Windows 11
- IDE: Visual Studio 2026 Community
- Compiler: MSVC (C++20 以上)
- GPU: NVIDIA RTX 系
- Driver: 最新の NVIDIA Driver / NVIDIA App
- API: Vulkan 1.3 (Vulkan SDK for Windows)
- Shader: GLSL → SPIR-V (glslangValidator 使用)
- Window System: GLFW (Win32 backend)
- Build System: CMake
- Debug / Profiling:
  - Validation Layer
  - NVIDIA Nsight Systems
  - NVIDIA Nsight Compute

# 目的：
- エンジンや Unity / Unreal は使わない
- Vulkan の最小構成 + GLSL を直接扱う
- GPU パイプライン構造（CPU → Vertex → Raster → Fragment → Framebuffer）を段階的に理解する
- Windows 環境特有の初期化・サーフェス・デバッグ構成も含めて理解する

# 設計方針：
- 各ステップは 1 つの概念だけを追加する
- 各ステップは独立したフォルダとして配置
- 各ステップに README.md を必ず含め、以下を記述する：
  - このステップで何を学ぶか
  - GPU パイプライン上のどの段階を扱っているか
  - Vulkan オブジェクトの対応関係（Instance / Device / Swapchain / Pipeline / DescriptorSet / Buffer 等）
  - オブジェクトの依存関係とライフタイム
  - なぜこの設定が必要なのか（設計意図）
  - Windows 固有の注意点（VK_KHR_win32_surface 等）

# ステップ構成：

## Step00_ClearScreen
- GLFW でウィンドウ作成（Win32 backend）
- Vulkan Instance / PhysicalDevice / LogicalDevice
- VK_KHR_surface / VK_KHR_win32_surface / Swapchain
- CommandBuffer 記録
- ClearColor のみ（シェーダーなし）
- Validation Layer 有効化

## Step01_MinimalTriangle
- 最小の頂点シェーダーとフラグメントシェーダー
- 固定三角形・固定色
- VkPipeline の基本構成
- RenderPass / Framebuffer の構造理解

## Step02_VertexColor
- 頂点カラー属性
- varying による補間
- location 修飾子の理解
- VertexInputState の詳細

## Step03_Texture
- UV 座標
- テクスチャ読み込み（stb_image）
- Image / ImageView / Sampler
- DescriptorSet によるバインディング

## Step04_Transform
- MVP 行列
- UniformBuffer
- 座標空間変換（Model / View / Projection）

## Step05_LightingBasic
- 法線
- Lambert 拡散反射
- 視線ベクトル
- フラグメント段でのライティング処理

# 制約：
- C++20
- Vulkan-Hpp は使わず C API で書く
- 依存は以下のみ：
  - Vulkan SDK (Windows)
  - GLFW
  - stb_image (ヘッダオンリー)
- 外部レンダリングエンジン / フレームワークなし
- 学習目的のため可読性を最優先する
- 抽象化しすぎず、Vulkan オブジェクト生成はすべて明示的に書く

# 出力してほしい内容：
1. プロジェクト全体のディレクトリ構成
2. 各 Step の main.cpp（Visual Studio + CMake でそのままビルド可能な完全な雛形）
3. 各 Step の GLSL シェーダー
4. 各 Step の README.md（学習解説つき）
5. Windows + Visual Studio 用のビルド方法（CMake / MSVC）
6. GLSL → SPIR-V のビルド手順（glslangValidator）
7. Validation Layer / Nsight を使ったデバッグ実行方法

# 重要：
- 教育目的なので抽象化しすぎない
- Vulkan オブジェクト生成・破棄順を省略しない
- 依存関係・ライフタイム・同期構造を必ず説明する
- なぜその設定が必要なのかを README で構造的に解説する
- GPU パイプライン上で「今どこを扱っているか」を常に明示する

# 以下に注意：
1.	cullMode = VK_CULL_MODE_NONE（または頂点順序を反時計回りに統一）
2.	Pipeline の必須フィールド（depthClampEnable, rasterizerDiscardEnable, depthBiasEnable）を明示的に VK_FALSE に設定
3.	vkQueueWaitIdle(presentQueue) による semaphore 再利用の安全化
4.	完全な ReadSpirvWithFallback（exe 相対パス対応）
5.	エラーハンドリング（PrintVkResult, ShowFatal）
6.	VGT_PAUSE_ON_EXIT 対応