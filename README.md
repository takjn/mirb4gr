# mirb for GR-CITRUS
![mirg4gr](https://github.com/takjn/mirb4gr/raw/standalone/images/photo1.jpg)

mirbをGR-CITRUSに移植してみました。
OLEDとUSBキーボードをGR-CITRUSにつなげることで、スタンドアロンで動作します。

## 必要な部品
必須のものは、GR-CITRUSとSSD1306、抵抗2本(10k〜20k)、USBキーボードです。
USBコネクタはキーボードを接続するためと電源をとるために利用します。OTGケーブルを改造したりすることでも代用ができます。

|部品 |個数 |備考 |
|:----|:----|:----|
|GR-CITRUS (FULL) |1 |[秋月電子通商](http://akizukidenshi.com/catalog/g/gK-10281/)で購入可能 |
|SSD1306 I2C OLEDモジュール |1 |[Aitendo](http://www.aitendo.com/product/14958)やAmazon.co.jpなどで購入可能 |
|金属皮膜抵抗 15kΩ|2| |
|USBキーボード |1 | |
|USBコネクタDIP化キット（Aメス） |2 |[秋月電子通商](http://akizukidenshi.com/catalog/g/gK-07429/)で購入可能 |
|電源用マイクロUSBコネクタDIP化キット |1 |[秋月電子通商](http://akizukidenshi.com/catalog/g/gK-10972/)で購入可能 |
|マイクロUSBケーブル |2 |スマホで一般的に使われているケーブル |
|ブレッドボード、ジャンパーワイヤ |1 | |

## 回路図
![overview](https://github.com/takjn/mirb4gr/raw/standalone/images/photo2.jpg)
![breadboard](https://github.com/takjn/mirb4gr/raw/standalone/images/mirb4gr_breadboard.png)

## 注意事項
* コネクタ類はハンダ付けが必要です。
* 安全性は考慮していません。くれぐれもコネクタを逆に刺したり部品をショートさせたりしないように注意してください。
* SSD1306は購入するお店によってVDDとGNDが逆になっているものがあります。部品にかかれている文字をよく確認してから接続してください。
* 5Vと3.3Vを間違えないように接続してください。SSD1306は3.3Vにつないでください。USBコネクタは5Vにつないでください。
* 起動メッセージは電源を入れてすぐに表示されます。万が一表示されない場合、回路に間違いが無いかを確認してください。
* 動かないUSBキーボードがあるようです。キーボードが動かない場合、抜き差しをしてみたり別のキーボードで試してみてください。
* キーボードの認識までは、通常、起動後に3秒程の時間がかかります。
* 電源容量が不足している場合、認識しないこともあります。サーボなど消費電力が大きい部品をつなぐ場合、キーボードとは別の電源を用意するなどの工夫をしてください。
* キーボードはUSキーボード配列となります。

## 使い方
* GR-CITRUSを単体でPCにつなぎ、リセットスイッチを押してUSBドライブとして認識させてください。
* mirb4grフォルダにあるcitrus_sketch.binをGR-CITRUSに書き込んでください。
* GR-CITRUSをPCから外してブレッドボードに取り付け、USBキーボードなどの部品をつないでください。
* プラスとマイナスの繋ぎ間違いやショートが無いことを確認してください。
* 電源用マイクロUSBケーブルを電源につないでください。
* 起動メッセージが表示されることを確認してください。

USBシリアル変換モジュールなどを持っている方は、シリアル通信で接続することもできます。
* GR-CITRUSのSerial1（0ピン、１ピン）とUSBシリアル変換モジュールを接続してください。
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
