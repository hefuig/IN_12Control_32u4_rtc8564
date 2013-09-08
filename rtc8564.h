// ------------------------------------------------------------
// -- IN-12b ニキシー管表示ユニット制御プログラム
// -- RTC-8564 RealTimeClock Module
// --
// -- Author. 楠 昌浩(Masahiro Kusunoki)
// -- http://mkusunoki.net
// -- Release 20130903 Rev 001
// ------------------------------------------------------------
#ifndef RTC8564_H
#define RTC8564_H

class RTC8564 {
  
  private:
  
  public:
  unsigned char Seconds;
  unsigned char Minutes;
  unsigned char Hours;
  unsigned char Hours12;
  unsigned char Days;
  unsigned char Weekday;
  unsigned char Month;
  unsigned char Years;
  unsigned char secondsHigh;
  unsigned char secondsLow;
  unsigned char minutesHigh;
  unsigned char minutesLow;
  unsigned char hourHigh;
  unsigned char hourLow;
  unsigned char hourHigh12;
  unsigned char hourLow12;
  unsigned char dayHigh;
  unsigned char dayLow;
  unsigned char monthHigh;
  unsigned char monthLow;
  unsigned char yearHigh;
  unsigned char yearLow;
  
  void begin();
  boolean isPowerFail();
  void initialize();
  void get();
  void set(unsigned char years, unsigned char month, unsigned char weekday, unsigned char days, unsigned char hours, unsigned char minutes, unsigned char seconds);
  unsigned char bcdToBin(unsigned char bcd);
  unsigned char binToBcd(unsigned char bin);
};

#endif

/*
  |   7 |   6 |   5 |   4 |   3 |   2 |   1 |   0 |
--+-----+-----+-----+-----+-----+-----+-----+-----+
00| TEST|   0 | STOP|   0 | TEST|   0 |   0 |   0 | Control1 STOP=0 timer running =1 timer stop
01|   0 |   x |   0 |TI/TP|  AF |  TF | AIE | TIE | Control2 Interrupt disable = 0
02|  VL |     10 Seconds  |       Seconds         | Seconds(BCD) VL=1 PowerFail
03|   x |     10 Minutes  |       Minutes         | Minutes(BCD)
04|   x |   x |  10 Hours |        Hours          | Hours(BCD)
05|   x |   x |  10 Days  |        Days           | Days(BCD)
06|   x |   x |   x |   x |   x |    Weekdays     | Weekdays 0=Sunday 1=Monday ....
07|   C |   x |   x |  10 |        Month          | Month(BCD) / Century 
08|           10Years     |        Years          | Years(BCD)
09|  AE |     10 Minutes  |       Minutes         | Minutes Alarm
0A|  AE |   x |  10 Hours |        Hours          | Hour Alarm
0B|  AE |   x |  10 Days  |        Days           | Day Alarm
0C|  AE |   x |   x |   x |   x |    Weekdays     | Weekday Alarm
0D|  FE |   x |   x |   x |   x |   x | FD1 | FD0 | CLKOUT Frquency
0E|  TE |   x |   x |   x |   x |   x | TD1 | TD0 | Timer control
0F| 128 |  64 |  32 |  16 |   8 |   4 |   2 |   1 | Timer (Binary)

*/
