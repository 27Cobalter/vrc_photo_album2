# vrc_photo_album2
## これはなに
- VRCでアルバムを見るためにhlsで流す動画を生成するやつ
  - クソデカ動画読み込んで大丈夫かテストしてないのでそれ次第

## 必要なライブラリ
- boost
- g++ ([clang++12.0.1とopencv4.5だと文字入れがうまくいかなかった](https://github.com/opencv/opencv/issues/20854))
- opencv4
- openmp
- ttf-migu（デフォルトフォント　こばルームで使ってるやつは[いい感じに合成した](https://wiki.27coba.lt/technology/font)）
- ffmpeg (shellで実行できること)

## ビルド&実行
```sh
$ git submodule update --init
$ mkdir build && cd $_
$ cmake .. -G ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=1
$ ninja
$ ./bin/vrc_photo_album2
```

## 実行オプション
-  --input=/path/to/input_dir
-  --output=/path/to/output_dir
-  --font=/path/to/font_file
-  --filepref=prefix
