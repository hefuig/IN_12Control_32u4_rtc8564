# IN-12 ニキシー管8桁表示ユニット制御プログラム

ロシア製ニキシー管 IN-12 の表示プログラムです。
Strawberry Linux の Da Vinci ボード(ATmega32u4搭載)を使用しニキシー管の表示を行ないます。

[IN-12b 開発時の Blog 記事一覧](http://mkusunoki.net/?s=IN-12)


* RTC-8564l RTC から日時の表示を行ないます。
* シリアルポートで受信したデータをニキシー管に表示します。
* データ表示後一定時間経過すると時計表示に戻ります。
* 電源投入時は内蔵 EEPROM より以前の状態を復元します。
* 開発環境は arduino-1.0.5


楠 昌浩(Masahiro Kusunoki)

http://mkusunoki.net

masahiro.kusunoki @ gmail.com
Twitter: ngc6589
Facebook: ngc6589

