# Develop with VSCode
- Visual Studioで、MSVC Compilerやcmakeなどの開発環境を入れる。
- `C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin`へのパスを通す。
  - コントロールパネルで、ユーザーの`Path`変数に追加する。
- これができれば、`$ cmake --build --preset msvc-x64-debug --config Debug`でのビルドが可能になる。
- また、VulkanのSDKへのパスを`.vscode\c_cpp_properties.json`に記載しておけば、依存関係解決も可能になる。
- おそらく、これでVSCodeのモダンな環境でシェーダープログラミングが行える。
