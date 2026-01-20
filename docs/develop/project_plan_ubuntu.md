あなたはプロフェッショナルなグラフィックスプログラマです。
私は Vulkan と GLSL を用いて、GPU パイプラインを体系的に学ぶための「最小構成の Vulkan + GLSL チュートリアルプロジェクト」を GitHub 用に作りたいです。

# 開発環境：
- OS: Ubuntu-24.04
- Compiler: clang++ or g++ (C++20)
- GPU: NVIDIA RTX 2080Ti
- API: Vulkan 1.3
- Shader: GLSL → SPIR-V (glslangValidator 使用)

# 目的：
- エンジンや Unity / Unreal は使わない
- Vulkan の最小構成 + GLSL を直接扱う
- GPU パイプライン構造（CPU → Vertex → Raster → Fragment → Framebuffer）を段階的に理解する

# 設計方針：
- 各ステップは 1 つの概念だけを追加する
- 各ステップは独立したフォルダとして配置
- 各ステップに README.md を必ず含め、以下を記述する：
  - このステップで何を学ぶか
  - GPU パイプライン上のどの段階を扱っているか
  - Vulkan オブジェクトの対応関係（Pipeline / DescriptorSet / Buffer 等）
  - なぜこの設定が必要なのか（設計意図）

# ステップ構成：

## Step00_ClearScreen
- GLFW でウィンドウ作成
- Vulkan Instance / Device / Swapchain
- CommandBuffer 記録
- ClearColor のみ（シェーダーなし）

## Step01_MinimalTriangle
- 最小の頂点シェーダーとフラグメントシェーダー
- 固定三角形・固定色
- VkPipeline の基本構成

## Step02_VertexColor
- 頂点カラー属性
- varying による補間
- location 修飾子の理解

## Step03_Texture
- UV 座標
- テクスチャ読み込み
- Sampler + ImageView
- DescriptorSet によるバインディング

## Step04_Transform
- MVP 行列
- UniformBuffer
- 座標空間変換

## Step05_LightingBasic
- 法線
- Lambert 拡散反射
- 視線ベクトル

# 制約：
- C++20
- Vulkan-Hpp は使わず C API で書く
- 依存は以下のみ：
  - Vulkan SDK
  - GLFW
  - stb_image (ヘッダオンリー)
- 外部レンダリングエンジン / フレームワークなし
- 学習目的のため可読性を最優先する

# 出力してほしい内容：
1. プロジェクト全体のディレクトリ構成
2. 各 Step の main.cpp（完全な雛形）
3. 各 Step の GLSL シェーダー
4. 各 Step の README.md（学習解説つき）
5. ビルド方法（cmake）
6. GLSL → SPIR-V のビルド手順

# 重要：
- 教育目的なので抽象化しすぎない
- Vulkan オブジェクト生成は明示的に書く
- 省略せずに最低限必要な構造をすべて含める
- なぜその設定が必要なのかを README で解説する
