# mirb for GR-CITRUS
["Interactive Arduino Shell"](https://github.com/bovi/ias/)をGR-CITRUSに移植してみました。

とりあえず、動くことは確認できた、というレベルです。

## 使い方
mirb4grフォルダにあるcitrus_sketch.binをGR-CITRUSに書き込んでください。

## ビルド方法
### リポジトリの取得
```
git clone https://github.com/takjn/mirb4gr.git
git submodule init
git submodule update
```

### RX630用libmruby.aのビルド
build_configをmrubyに上書きコピーして、mrubyをmakeします。

```
cp build_config.rb mruby/build_config.rb
cd mruby
make
```

以下のようなメッセージが出力されていれば成功です。

```
================================================
      Config Name: RX630
 Output Directory: build/RX630
    Included Gems:
             mruby-compiler - mruby compiler library
================================================
```

### citrus_sketch.binのmake
- gr-mirbフォルダに戻り、makefileの``GNU_PATH``を適切に修正してからmakeしてください。
- 出来上がったcitrus_sketch.binをGR-CITRUSに書き込んでください。

## TODO
- line inputの関数を実装する
- カーソルキーを正しく処理する
- backspaceキーを正しく処理する
- GR-CITRUSのピン番号を定数として定義する
- code blockに対応する
- 本家mrubyのmirbをベースに書き直す
