# vulkan-glsl-tutorial

Vulkan 1.3 と GLSL を「最小構成から段階的に」学ぶためのチュートリアルプロジェクトです。

各ステップは **1 つの概念だけ**を追加し、`steps/StepXX_*` として独立したフォルダに置きます。

## 必要環境 (Ubuntu 24.04)

- CMake 3.20+
- C++20 対応コンパイラ (`clang++` or `g++`)
- Vulkan SDK (Vulkan 1.3)
- GLFW
- `glslangValidator` (GLSL → SPIR-V)

インストール例 (apt):

```bash
sudo apt update
sudo apt install -y cmake ninja-build pkg-config \
	g++ clang \
	libglfw3-dev \
	libvulkan-dev vulkan-tools vulkan-validationlayers \
	glslang-tools
```

※ Vulkan のヘッダ/ライブラリは Vulkan SDK 由来の環境もあります。手元の環境に合わせてください。

## ビルド

アウト・オブ・ソースビルドを前提にしています。

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

## 実行

実行ファイルは、基本的に「実行ファイルと同じ場所にある `shaders/`」を参照します（ビルド時に自動コピーされます）。
そのため通常はどのディレクトリから実行しても動きます。

例:

```bash
./build/steps/Step00_ClearScreen/Step00_ClearScreen
./build/steps/Step01_MinimalTriangle/Step01_MinimalTriangle
```

## SSH 経由で実行する場合 (Windows + Xming)

このチュートリアルは GLFW でウィンドウを開くため、SSH セッションが GUI（X11/Wayland）に接続できないと何も表示されません。

特に `echo $DISPLAY` が `43.12.56.29:0.0` のように **クライアントの IP:0.0 になっている場合**は、SSH の X11 フォワーディングではなく「TCP で直接 X サーバへ接続」しようとしており、Xming 側の認証（"No protocol specified"）で弾かれることが多いです。

おすすめは **SSH の X11 フォワーディングを使う**方法です。

1) Windows 側で Xming を起動
- XLaunch の設定で "Disable access control"（または Xming を `-ac`）にすると詰まりにくいです（※セキュリティ的には弱くなるので、基本は X11 フォワーディングとセットで使用してください）。

2) SSH を X11 フォワーディング付きで接続
- OpenSSH の例: `ssh -X user@server`（うまくいかない場合は `-Y`）
- PuTTY の例: Connection → SSH → X11 → "Enable X11 forwarding" を ON、"X display location" を `localhost:0.0`

補足（Windows の OpenSSH の場合）: クライアント側で `DISPLAY` が未設定だと `X11 forwarding requested but DISPLAY not set` になります。PowerShell なら `set` の代わりに `$env:DISPLAY="localhost:0.0"` を実行してから `ssh -X ...` を試してください。

3) SSH 先で確認
- `echo $DISPLAY` が `localhost:10.0` のような値になっていれば OK（`43.x.x.x:0.0` のような値のままだと非推奨）

4) まずは X11 の疎通テスト
- SSH 先で `sudo apt install -y x11-apps` を入れて `xeyes` や `xclock` が出るか確認

5) Vulkan アプリを実行
- 例: `./build/steps/Step00_ClearScreen/Step00_ClearScreen`

### よくあるエラー: `X connection to <ip>:0.0 broken`

これは **X サーバ（Xming）が接続を切った**ときに出るメッセージで、原因としては次が多いです。

- `DISPLAY=<クライアントIP>:0.0` の “直結” になっている（SSH X11 フォワーディングではない）
	- このモードは認証/アクセス制御や Windows Firewall の影響を受けやすく、不安定になりがちです。
- Xming 側のアクセス制御（X0.hosts や `-ac` の設定）と接続元 IP が一致していない
- VPN/NAT などで経路が不安定、TCP/6000 が途中で遮断される

**推奨**: `echo $DISPLAY` が `localhost:10.0` のようになるように、SSH の X11 フォワーディング（`ssh -X` / PuTTY の "Enable X11 forwarding"）を使用してください。

補足: X11 フォワーディング経由の Vulkan 表示は環境によって遅い/不安定になることがあります。その場合は RDP/VNC などで GUI セッションに入って実行する方が確実です。

さらに補足: Windows の Xming に対する X11 転送は、Vulkan の Swapchain Present（表示）が成立しない／接続が切れる（`X connection ... broken`）ケースがあります。`xeyes` が動いても Vulkan が動くとは限らないため、表示を確実にしたい場合は **リモートの GUI セッション上で実行**してください。

### 出力確認のコツ

`./app 2>&1 | cat` のようにパイプすると、終了コードが `cat` 側のものになり、失敗に気づきにくいことがあります。終了コードも確認したい場合は `set -o pipefail` を使うか、パイプ無しで実行してください。

## シェーダー (GLSL → SPIR-V)

各 Step の `shaders/*.vert` / `shaders/*.frag` は、ビルド時に `glslangValidator` で `.spv` へコンパイルされ、実行ファイル隣の `shaders/` にコピーされます。

`glslangValidator` が見つからない場合は CMake が警告を出し、シェーダーの自動生成はスキップされます。

## ステップ一覧

- `Step00_ClearScreen`: Clear のみ (シェーダー無し)
- `Step01_MinimalTriangle`: 最小の Graphics Pipeline + 三角形
- `Step02_VertexColor`: varying 補間と `location`
- `Step03_Texture`: 画像サンプリング (DescriptorSet)
- `Step04_Transform`: MVP 行列 (UniformBuffer)
- `Step05_LightingBasic`: Lambert 拡散反射
