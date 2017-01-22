# mirb for GR-CITRUS
mirbをGR-CITRUSに移植してみました。

## 使い方
* mirb4grフォルダにあるcitrus_sketch.binをGR-CITRUSに書き込んでください。
* ターミナルソフトからGR-CITRUSに接続してください。通信速度は115200bpsです。
* OSやターミナルソフトによっては、起動メッセージが表示されないことがあります。エンターキーを押してプロンプトを表示してください。

## ビルド方法
### ビルド環境
GNURX_v14.03が必要です。
Windowsの方は、[GR-CITRUSに組み込むmrubyをビルドする方法 for Windows](http://qiita.com/takjn/items/42fa8ad0c61a8840a9c2)や、macOSの方は、[GR-CITRUSに組み込むmrubyをビルドする方法 for macOS](http://qiita.com/takjn/items/0ef3d46107ac8faaf621)、Linuxの場合は[こちら](http://japan.renesasrulz.com/gr_user_forum_japanese/f/gr-citrus/3447/x64-ubuntu)などを参考にセットアップしてください。

### リポジトリの取得
```
git clone https://github.com/takjn/mirb4gr.git
cd mirb4gr
git submodule init
git submodule update
```

### RX630用libmruby.aのビルド
build_configをmrubyに上書きコピーしてください。build_config内の`BIN_PATH`を環境に合わせて適切に設定してください。
その後、mrubyをmakeしてください。

```
cp build_config.rb mruby/build_config.rb
cd mruby
make
```

上書きコピーをせずにbuild_configを`MRUBY_CONFIG`環境変数で指定することもできます。
{path_to}には、mirb4grフォルダまでのパスを指定してください。
```
export MRUBY_CONFIG=/{path_to}/mirb4gr/build_config.rb
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
             mruby-arduino - Arduino API
             mruby-print - standard print/puts/p for mruby-arduino environments
             mruby-gr-citrus - Extention libraries for GR-CITRUS
================================================
```

### citrus_sketch.binのmake
- mirb4grフォルダに戻り、makefileの``GNU_PATH``を適切に修正してからmakeしてください。
- 出来上がったcitrus_sketch.binをGR-CITRUSに書き込んでください。

## Sample
手動でLEDをOn、Offします。
```
> extend Arduino      # load "mruby-arduino" module
 => main
> digitalWrite(61, 1) # LED On  (61 = Built-in LED on GR-CITRUS)
 => nil
> digitalWrite(61, 0) # LED Off
 => nil
```

メソッドを定義してLEDを点滅させます。
```
> extend Arduino
 => main
> def blink
*       10.times do
*               digitalWrite(61, 1)
*               delay 1000
*               digitalWrite(61, 0)
*               delay 1000
*       end
* end
 => :blink
> blink
```

クラスを定義してLEDを点滅させます。
```
> class GR_CITRUS
*       include Arduino
*       include PinsArduino
*       def run
*               10.times do
*                       digitalWrite(PIN_LED0, HIGH)
*                       delay 1000
*                       digitalWrite(PIN_LED0, LOW)
*                       delay 1000
*               end
*       end
* end
 => :run
> GR_CITRUS.new.run
```

## 実装の方向性
mirb自体は、本家mrubyのmirbを忠実に移植したいと考えています。他のボードへの移植性を高めるため、Gadget RenesasやGR-CITRUS向けの機能拡張は、mrbgemsで行います。
