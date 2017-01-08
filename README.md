# mirb for GR-CITRUS
mirbをGR-CITRUSに移植してみました。

とりあえず、動くことは確認できた、というレベルです。
今後独自拡張を入れていく可能性があります。

## 使い方
* mirb4grフォルダにあるcitrus_sketch.binをGR-CITRUSに書き込んでください。
* ターミナルソフトからGR-CITRUSに接続してください。通信速度は115200bpsです。

## ビルド方法
### ビルド環境
GNURX_v14.03が必要です。
[GR-CITRUSに組み込むmrubyをビルドする方法 for Windows](http://qiita.com/takjn/items/42fa8ad0c61a8840a9c2)や、Linuxの場合は[こちら](http://japan.renesasrulz.com/gr_user_forum_japanese/f/gr-citrus/3447/x64-ubuntu)などを参考にセットアップしてください。

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

## Sample
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
