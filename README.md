# mirb for GR-CITRUS
["Interactive Arduino Shell"](https://github.com/bovi/ias/)をGR-CITRUSに移植してみました。

とりあえず、動くことは確認できた、というレベルです。

## 使い方
- citrus_sketch.binをGR-CITRUSに書き込む

## ビルド方法
### リポジトリの取得
- git clone https://github.com/takjn/gr-mirb.git
- git submodule init
- git submodule update

### citrus_sketch.binのmake
- makefileの``GNU_PATH``を適切に修正してmake
- 出来上がったcitrus_sketch.binをGR-CITRUSに書き込む

## TODO
- カーソルやバックスペースなどのキーを正しく処理する
- mruby-arduinoを組み込む
- code blockに対応する
- 本家mrubyのmirbをベースに書き直す
