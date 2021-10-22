# vrc_photo_album2
## これはなに
- VRCでアルバムを見るためにhlsで流す動画を生成するやつ
  - クソデカ動画読み込んで大丈夫かテストしてないのでそれ次第

## 必要なライブラリ
- imagemagick
- opencv4

## ビルド&実行
```sh
$ git submodule update --init
$ mkdir build && cd $_
$ cmake .. -G ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=1
$ ninja
$ ./bin/vrc_photo_album2
```
