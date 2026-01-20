# vulkan-glsl-tutorial

Vulkan 1.3 + GLSL（glslangValidator）で、GPUパイプラインを段階的に学ぶための **最小構成チュートリアル** です。

- OS: Windows 11
- IDE: Visual Studio 2026 Community（CMake プロジェクト）
- Compiler: MSVC（C++20）
- Window: GLFW（Win32 backend）
- API: Vulkan 1.3（Vulkan SDK for Windows）
- Shader: GLSL → SPIR-V（`glslangValidator`）
- Debug/Profiling: Validation Layer / NVIDIA Nsight Systems / NVIDIA Nsight Compute

## まず動かす（最短手順）

このリポジトリは、`CMakePresets.json` に定義されたプリセットを使って **x64 + Visual Studio 2026** で構成・ビルドできます。

### 1) 対話不要でビルドする（PowerShell）

リポジトリ直下で実行します。

```powershell
cmake --preset msvc-x64 --fresh
cmake --build --preset msvc-x64-debug --config Debug
```

### 2) 実行する

生成された exe は次に出力されます（例）：

- `build/msvc-x64/steps/Step00_ClearScreen/Debug/Step00_ClearScreen.exe`

まずは `Step00_ClearScreen` を起動して、ウィンドウが開きクリア色が表示されれば OK です。

> 補足：各 Step は起動時にシェーダー (`compiled_shaders/`) を参照します。ビルド後に実行して、シェーダー関連のエラーが出ないことも合わせて確認してください。

## ディレクトリ構成

```
./
  CMakeLists.txt
  cmake/                 # CMake 補助モジュール（依存取得、シェーダーコンパイル、設定）
  third_party/           # 方針ドキュメント（依存は FetchContent で取得）
  steps/
    Step00_ClearScreen/
    Step01_MinimalTriangle/
    Step02_VertexColor/
    Step03_Texture/
    Step04_Transform/
    Step05_LightingBasic/
  docs/
```

## 前提環境（Windows）

1. **Vulkan SDK for Windows** をインストール（Vulkan 1.3 以上）。以下を確認してください：
   - 環境変数 `VULKAN_SDK` が設定されている
   - `glslangValidator.exe` が利用できる（Vulkan SDK に同梱）
2. **Visual Studio 2026 Community** をインストールし、次を有効化してください：
   - C++ によるデスクトップ開発
   - CMake Tools for Windows
   - MSVC ツールセット（x64）

GLFW は CMake の `FetchContent` により自動取得します（手動インストール不要）。

## ビルド（CMake + MSVC）

リポジトリ直下で実行します。

> 注意：このリポジトリは Visual Studio の CMake 統合で扱いやすいように `CMakePresets.json` を提供しています。
> まずは上記「最短手順」のプリセット経由でのビルドを推奨します。

### 推奨：x64 Native Tools で Ninja ビルド

1) 「x64 Native Tools Command Prompt for VS 2026」を起動して、

```bat
cmake -S . -B build-ninja-x64 -G Ninja
cmake --build build-ninja-x64
```

### Visual Studio から（CMake プロジェクトとして）

- Visual Studio でフォルダを開く
- 上部の構成（Configure Preset）で `msvc-x64` を選ぶ
- 構成が完了したら、そのまま `StepXX_...` を実行できます

## GLSL → SPIR-V 生成フロー

- 各 Step は `steps/<StepName>/shaders/` に GLSL を配置します。
- CMake が `glslangValidator -V` を実行して `.spv` を生成します。
- 生成物はビルドツリー内の `compiled_shaders/` に出力されます。
  - 一部の Visual Studio 同梱 CMake では `POST_BUILD` でのディレクトリコピーが不安定なため、
    実行時は `compiled_shaders/` 側も探索する実装になっています（Step01/Step02）。

手動コンパイル例：

```powershell
glslangValidator -V steps/Step01_MinimalTriangle/shaders/triangle.vert -o triangle.vert.spv
```

## Validation Layer

- Validation の有効/無効は CMake オプション `VGT_ENABLE_VALIDATION` で切り替えます（既定: ON）。
- 有効時は `VK_LAYER_KHRONOS_validation` を有効化し（可能なら Debug Utils Messenger も設定します）。

Layer が見つからない旨のエラーが出る場合は、Validation Layer を含む構成で Vulkan SDK を入れ直してください。

## Nsight（簡易メモ）

### Nsight Systems

- Nsight Systems から実行ファイルを起動してキャプチャします。
- 最初は `Debug`（Validation の警告が役立つ）→ 次に `Release`（計測向け）がおすすめです。

### Nsight Compute

- シェーダー負荷があるステップ（後半）から始めて、シェーダー実行の詳細を確認します。
- まずは小さいキャプチャで進めてください。Validation はタイミングへ影響することがあります。

## ステップ

- `Step00_ClearScreen`: スワップチェーン画像のクリア（シェーダーなし）
- `Step01_MinimalTriangle`: シェーダー + Graphics Pipeline を導入して三角形を描画
- `Step02_VertexColor`: 頂点バッファ + 頂点入力属性（`location`）
- `Step03_Texture`: （スケルトン）Descriptor によるテクスチャサンプリング
- `Step04_Transform`: （スケルトン）UniformBuffer による MVP
- `Step05_LightingBasic`: （スケルトン）Lambert 拡散反射の基本
