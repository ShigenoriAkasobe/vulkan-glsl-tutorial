# assets

この Step はデフォルトで「手続き生成のチェッカーボード」を使います。

stb_image を使って外部画像を読みたい場合は、たとえば `texture.png` をここに置き、
`main.cpp` のテクスチャ生成部分を

- stb_image で RGBA8 にデコード
- staging buffer にコピー

へ置き換えてください。
