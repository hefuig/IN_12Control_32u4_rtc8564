// ------------------------------------------------------------
// -- IN-12b ニキシー管表示ユニット制御プログラム
// -- RTC-8564 RealTimeClock Module
// --
// -- Author. 楠 昌浩(Masahiro Kusunoki)
// -- http://mkusunoki.net
// -- Release 20130903 Rev 001
// ------------------------------------------------------------
#include <Arduino.h>
#include <Wire.h>
#include "rtc8564.h"

#define I2CRTC8564 0x51

// ------------------------------------------------------------
//   Check VF bit
// ------------------------------------------------------------
void RTC8564::begin() {

  if(isPowerFail() == true) {
    initialize();
  }
}

// ------------------------------------------------------------
//   Check VL bit
// ------------------------------------------------------------
boolean  RTC8564::isPowerFail() {
  unsigned char seconds = 0x80;

  Wire.beginTransmission(I2CRTC8564);
  Wire.write(0x02);
  Wire.endTransmission();
  Wire.requestFrom(I2CRTC8564, 1);
  if(Wire.available() > 0) {
    seconds = Wire.read();
  }
  return(bitRead(seconds, 7)); 
}

// ------------------------------------------------------------
//   RTC8564モジュール初期化
// ------------------------------------------------------------
void RTC8564::initialize() {

  Wire.beginTransmission(I2CRTC8564);
  Wire.write(0x00);  // Set Address 0
  Wire.write(0x20);  // Address 0 STOP=1
  Wire.write(0x00);  // Address 1 
  Wire.write(0x00);  // Address 2 Seconds
  Wire.write(0x41);  // Address 3 Minutes
  Wire.write(0x17);  // Address 4 Hours
  Wire.write(0x08);  // Address 5 Days
  Wire.write(0x04);  // Address 6 Weekdays
  Wire.write(0x08);  // Address 7 Month
  Wire.write(0x10);  // Address 8 Years
  Wire.write(0x00);  // Address 9 Alarm Seconds
  Wire.write(0x00);  // Address A Alarm
  Wire.write(0x00);  // Address B Alarm
  Wire.write(0x00);  // Address C Alarm
  Wire.write(0x00);  // Address D CLOCKOUT
  Wire.write(0x00);  // Address E Timer
  Wire.write(0x00);  // Address F Timer
  Wire.write(0x00);  // Address 0 STOP=0
  Wire.endTransmission();
}

// ------------------------------------------------------------
//   日時取得
// ------------------------------------------------------------
void RTC8564::get() {
  int i;
  unsigned char ch[7];

  Wire.beginTransmission(I2CRTC8564);
  Wire.write(2);    // Set addoress 2
  Wire.endTransmission();
  Wire.requestFrom(I2CRTC8564, 7);
  while(Wire.available() > 0) {
    for(i = 0; i < 7; i++) {
      ch[i] = Wire.read();
    }
  }
  Seconds = ch[0] & 0x7F;
  Minutes = ch[1] & 0x7F;
  Hours = ch[2] & 0x3F;
  Days =  ch[3] & 0x3F;
  Weekday = ch[4] & 0x07;
  Month = ch[5] & 0x1F;
  Years = ch[6];
  secondsHigh = Seconds >> 4;
  secondsLow = Seconds & 0x0F;
  minutesHigh = Minutes >> 4;
  minutesLow = Minutes & 0x0F;
  hourHigh = Hours >> 4;
  hourLow = Hours & 0x0F;
  dayHigh = Days >> 4;
  dayLow = Days & 0x0F;
  monthHigh = Month >> 4;
  monthLow = Month & 0x0F;
  yearHigh = Years >> 4;
  yearLow = Years & 0x0F;
}

// ------------------------------------------------------------
//   日時設定
// ------------------------------------------------------------
void RTC8564::set(unsigned char years, unsigned char month, unsigned char weekday, unsigned char days, unsigned char hours, unsigned char minutes, unsigned char seconds) {
  int i;
  unsigned char ch[7];

  ch[0] = seconds;
  ch[1] = minutes;
  ch[2] = hours;
  ch[3] = days;
  ch[4] = weekday;
  ch[5] = month;
  ch[6] = years;
  // stop RTC
  Wire.beginTransmission(I2CRTC8564);
  Wire.write(0x00);    // Set addoress 0
  Wire.write(0x20);    // STOP=1
  Wire.endTransmission();
  // write value
  Wire.beginTransmission(I2CRTC8564);
  Wire.write(0x02);    // Set addoress 2
  for(i = 0; i < 7; i++) {
    Wire.write(ch[i]);
  }
  Wire.endTransmission();
  // start RTC
  Wire.beginTransmission(I2CRTC8564);
  Wire.write(0x00);    // Set addoress 0
  Wire.write(0x00);    // STOP=0
  Wire.endTransmission();
}

// ------------------------------------------------------------
//   BCD からバイナリ変換
// ------------------------------------------------------------
unsigned char RTC8564::bcdToBin(unsigned char bcd) {
  return(bcd - (6 * (bcd >> 4)));
}

// ------------------------------------------------------------
//   バイナリから BCD 変換
// ------------------------------------------------------------
unsigned char RTC8564::binToBcd(unsigned char bin) {
  return(bin + (6 * (bin / 10)));
}

