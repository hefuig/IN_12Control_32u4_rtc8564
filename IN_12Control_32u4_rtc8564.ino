// ------------------------------------------------------------
// -- IN-12b ニキシー管表示ユニット制御プログラム
// -- main module
// --
// -- Author. 楠 昌浩(Masahiro Kusunoki)
// -- http://mkusunoki.net
// -- Release 20130903 Rev 001
// ------------------------------------------------------------
#include <interrupt.h>
#include <SPI.h>
#include <Wire.h>
#include <EEPROM.h>
#include "rtc8564.h"

// ------------------------------------------------------------
// -- ポート番号定義
// ------------------------------------------------------------
#define HC595RCLK 17
#define K155ID1_A 8
#define K155ID1_B 9
#define K155ID1_C 10
#define K155ID1_D 11
#define COMMA 7
#define LED 13
#define GP01 23

// ------------------------------------------------------------
// -- グローバル変数
// ------------------------------------------------------------
RTC8564 rtc;
boolean led = false;

// 割り込み処理用変数
unsigned int interruptCount = 0;
boolean interruptOVF = false;

// IN-12 数字セグメント、カンマ
unsigned char IN12Num[8] = { 
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
unsigned char IN12Com[8] = { 
  0, 0, 0, 0, 0, 0, 0, 0 };
unsigned char IN12Digit = 0;
unsigned char IN12BrightDaytime = 9;
unsigned char IN12BrightNight = 3;

// USB Device address
unsigned char udaddrOld = 0;

// シリアルデータ
boolean serial0StringComplete = false;
char serial0String[24];
int serial0StringIdx = 0;

// 時計表示秒数
unsigned int displayDateIntervalValue = 600;  // 年月日表示する秒数間隔
unsigned int displayDateIntervalCount = 0;
unsigned int displayDateSecondsValue = 10;    // 年月日表示秒数
unsigned char dispTimeFormat = 0;             // 時分秒表示フォーマット
unsigned char hourDaytime = 7;
unsigned char hourNight = 22;
unsigned char hour12_24format = 1;  // 0 = 12, 1 = 24 hour display

// ユーザデータ表示秒数
unsigned int userDataTimerValue = 30;         // ユーザーデータを表示する時間
unsigned int userDataTimerCount = 0;

// ------------------------------------------------------------
// -- TIMER0 割り込み処理(1ms毎)
// ------------------------------------------------------------
ISR(TIMER0_COMPA_vect) {
  unsigned char bright = 5;

  // K155ID1 の A～D を設定
  digitalWrite(K155ID1_A, bitRead(IN12Num[IN12Digit], 0));
  digitalWrite(K155ID1_B, bitRead(IN12Num[IN12Digit], 1));
  digitalWrite(K155ID1_C, bitRead(IN12Num[IN12Digit], 2));
  digitalWrite(K155ID1_D, bitRead(IN12Num[IN12Digit], 3));

  // カンマの設定
  if(IN12Com[IN12Digit] != 0) {
    digitalWrite(COMMA, HIGH);
  } 
  else {
    digitalWrite(COMMA, LOW);
  }

  // IN-12 のアノード ON
  digitalWrite(HC595RCLK, LOW);
  SPI.transfer(bit(IN12Digit));
  digitalWrite(HC595RCLK, HIGH);

  if(hourNight > hourDaytime) {
    if(rtc.bcdToBin(rtc.Hours) < hourDaytime || rtc.bcdToBin(rtc.Hours) >= hourNight) {
      bright = IN12BrightNight;
    } 
    else {
      bright = IN12BrightDaytime;
    }
  } 
  else {
    if(rtc.bcdToBin(rtc.Hours) >= hourNight && rtc.bcdToBin(rtc.Hours) < hourDaytime) {
      bright = IN12BrightNight;
    } 
    else {
      bright = IN12BrightDaytime;
    }
  }
  switch(bright) {
  case 1: 
    delayMicroseconds(50); 
    break;
  case 2: 
    delayMicroseconds(150); 
    break;
  case 3: 
    delayMicroseconds(250); 
    break;
  case 4: 
    delayMicroseconds(350); 
    break;
  case 5: 
    delayMicroseconds(450); 
    break;
  case 6: 
    delayMicroseconds(550); 
    break;
  case 7: 
    delayMicroseconds(650); 
    break;
  case 8: 
    delayMicroseconds(750); 
    break;
  case 9: 
    delayMicroseconds(850); 
    break;
  default: 
    delayMicroseconds(850);
  }
  // IN-12 のアノード OFF
  digitalWrite(HC595RCLK, LOW);
  SPI.transfer(0x00);
  digitalWrite(HC595RCLK, HIGH);

  switch(bright) {
  case 1: 
    delayMicroseconds(750); 
    break;
  case 2: 
    delayMicroseconds(650); 
    break;
  case 3: 
    delayMicroseconds(550); 
    break;
  case 4: 
    delayMicroseconds(450); 
    break;
  case 5: 
    delayMicroseconds(350); 
    break;
  case 6: 
    delayMicroseconds(250); 
    break;
  case 7: 
    delayMicroseconds(150); 
    break;
  case 8: 
    delayMicroseconds(50); 
    break;
  case 9: 
    //delayMicroseconds(800); 
    break;
  default: 
  break;
    //delayMicroseconds(800);
  }
  
  IN12Digit++;
  if(IN12Digit > 7) {
    IN12Digit = 0;
  }    
  interruptCount++;

  // 割り込みマスクをセット
  TIFR0 |= (1<<OCF0A);
  
  //
}

// ------------------------------------------------------------
// -- 初期設定
// ------------------------------------------------------------
void setup() {

  randomSeed(analogRead(0));
  delay(500);
  // LEDポート, GPIO
  pinMode(LED, OUTPUT);
  pinMode(GP01, INPUT_PULLUP);
  // 74HC595 
  pinMode(HC595RCLK, OUTPUT);
  digitalWrite(HC595RCLK, LOW);

  // K155ID1
  pinMode(K155ID1_A, OUTPUT);
  pinMode(K155ID1_B, OUTPUT);
  pinMode(K155ID1_C, OUTPUT);
  pinMode(K155ID1_D, OUTPUT);

  // IN12 カンマ
  pinMode(COMMA, OUTPUT);

  // i2c 
  Wire.begin();
  delay(100);

  // RTC-8564 リアルタイムクロック
  rtc.begin();

  // シリアルポート1
  Serial1.begin(9600);

  // SPI
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV4);
  SPI.setDataMode(SPI_MODE0);
  SPI.setBitOrder(MSBFIRST);

  readEEPROM();

  // タイマー、カウンタ 0 初期化
  // Disbale interrupt
  TIMSK0 &= ~(1<<OCIE0B);  
  TIMSK0 &= ~(1<<OCIE0A);  
  TIMSK0 &= ~(1<<TOIE0);  
  // 比較一致ﾀｲﾏ/ｶｳﾝﾀ解除(CTC)動作
  TCCR0B &= ~(1<<WGM02);
  TCCR0A |= (1<<WGM01);
  TCCR0A &= ~(1<<WGM00);
  // clkI/O/64 (64分周)
  TCCR0B &= ~(1<<CS02);
  TCCR0B |= (1<<CS01);
  TCCR0B |= (1<<CS00);
  TCNT0 = 0;
  // 16MHz / 64 = 4us. 4us * 250 = 1.0ms
  OCR0A =  249;
  // set interrupt mask
  TIFR0 |= (1<<OCF0A);
  // Enable interrupt
  TIMSK0 |= (1<<OCIE0A);
}

// ------------------------------------------------------------
// -- メイン処理
// ------------------------------------------------------------
void loop() {
  int i = 0;
  int j = 0;

  // 1秒毎の処理
  if(interruptCount > 999) {
    // ユーザーデータ表示期間ではない = 時計モード
    if(userDataTimerCount == 0) {
      rtc.get();
      if(displayDateIntervalCount <= displayDateIntervalValue) {
        // 時分秒表示
        for(int i = 0; i < 8; i++) {
          IN12Com[i] = 0;
        }
        if(dispTimeFormat == 0) {
          IN12Num[0] = 0x0F;
          if(hour12_24format == 1) {
            IN12Num[1] = rtc.hourHigh;
            IN12Num[2] = rtc.hourLow;
          } 
          else {
            IN12Num[1] = rtc.hourHigh12;
            IN12Num[2] = rtc.hourLow12;
          }
          IN12Num[3] = rtc.minutesHigh;
          IN12Num[4] = rtc.minutesLow;
          IN12Num[5] = rtc.secondsHigh;
          IN12Num[6] = rtc.secondsLow;
          IN12Num[7] = 0x0F;
        }
        if(dispTimeFormat == 1) {
          if(hour12_24format == 1) {
            IN12Num[0] = rtc.hourHigh;
            IN12Num[1] = rtc.hourLow;
          } 
          else {
            IN12Num[0] = rtc.hourHigh12;
            IN12Num[1] = rtc.hourLow12;
          }
          IN12Num[2] = 0x0F;
          IN12Num[3] = rtc.minutesHigh;
          IN12Num[4] = rtc.minutesLow;
          IN12Num[5] = 0x0F;
          IN12Num[6] = rtc.secondsHigh;
          IN12Num[7] = rtc.secondsLow;
          IN12Com[2] = 1;
          IN12Com[5] = 1;
        } 
      } 
      else {
        // 年月日表示
        for(int i = 0; i < 8; i++) {
          IN12Com[i] = 0;
        }
        IN12Num[0] = 2;
        IN12Num[1] = 0;
        IN12Num[2] = rtc.yearHigh;
        IN12Num[3] = rtc.yearLow;
        IN12Num[4] = rtc.monthHigh;
        IN12Num[5] = rtc.monthLow;
        IN12Num[6] = rtc.dayHigh;
        IN12Num[7] = rtc.dayLow;
        if(displayDateIntervalCount >= ( displayDateIntervalValue + displayDateSecondsValue)) {
          displayDateIntervalCount = 0;
        }
      }
      displayDateIntervalCount++;
    }
    // ユーザーデータ表示中
    if(userDataTimerCount > 0) {
      userDataTimerCount--;
    }
    // LED 点滅
    if(led == false) {
      led = true;
      digitalWrite(LED, HIGH);
    } 
    else {
      led = false;
      digitalWrite(LED,LOW);
    }
    interruptCount = 0;
  }    

  // USB ポートのアドレスが変った
  if(udaddrOld != UDADDR) {
    udaddrOld = UDADDR;
    if(UDADDR == 0) {
      Serial.end();
    } 
    else {
      Serial.begin(9600);
    }
  }

  // シリアル0 受信処理
  if(Serial) {
    while(Serial.available()) {
      char ch = (char)Serial.read();
      serial0String[serial0StringIdx] = ch;
      Serial.write(ch);
      if(serial0String[serial0StringIdx] == '\r') {
        Serial.write('\n');
        serial0String[serial0StringIdx] = 0;
        serial0StringComplete = true;
      }
      serial0StringIdx++;
      if(serial0StringIdx > 23) {
        serial0StringIdx = 23;
      }
      // CR コードだけの入力は無視
      if(serial0StringComplete == true && serial0StringIdx == 1) {
        serial0StringIdx = 0;
        serial0StringComplete = false;
      }
    }
  }

  // シリアル1 受信処理
  while(Serial1.available()) {
    char ch = (char)Serial1.read();
    serial0String[serial0StringIdx] = ch;
    Serial1.write(ch);
    if(serial0String[serial0StringIdx] == '\r') {
      Serial1.write('\n');
      serial0String[serial0StringIdx] = 0;
      serial0StringComplete = true;
    }
    serial0StringIdx++;
    if(serial0StringIdx > 23) {
      serial0StringIdx = 23;
    }
    // CR コードだけの入力は無視
    if(serial0StringComplete == true && serial0StringIdx == 1) {
      serial0StringIdx = 0;
      serial0StringComplete = false;
    }
  }

  // 受信データ処理
  if(serial0StringComplete == true) {
    // ニキシー表示データチェック
    boolean isUserData = true;
    for(i = 0; i < (serial0StringIdx - 1); i++) {
      if(serial0String[i] == ' ') continue; 
      if(serial0String[i] == '.') continue;
      if((serial0String[i] >= '0') && (serial0String[i] <= '9')) continue;
      isUserData = false;
    }
    // ユーザーデータを表示
    if(isUserData == true) {
      for(i = 0; i < 8; i++) {
        IN12Num[i] = 0x0F;
        IN12Com[i] = 0;
      }
      j = 7;
      for(i = (serial0StringIdx - 1); i >= 0; i--) {
        if(serial0String[i] >= '0' && serial0String[i] <= '9') {
          if(j >= 0) {
            IN12Num[j--] = serial0String[i] - 0x30;
          }
        } 
        else if(serial0String[i] == ' ') {
          if(j >= 0) {
            IN12Num[j--] = 0x0F;
          }
        }
      }
      j = 8;
      boolean previusCharValid = false;
      for(i = (serial0StringIdx - 1); i >= 0; i--) {
        if((serial0String[i] >= '0' && serial0String[i] <= '9') || serial0String[i] == ' ') {
          previusCharValid = true;
          if(j > 0) {
            j--;
          }
          continue;
        }
        if(serial0String[i] == '.' && previusCharValid == true) {
          IN12Com[j] = 1;
          previusCharValid = false;
        }
      } 
      userDataTimerCount = userDataTimerValue;
    } 
    else {
      // コマンド処理
      if(strncmp(serial0String, "help", 4) == 0) {
        serialPrintln("set time YYMMDD hhmmss");
        serialPrintln("set timeformat [0 or 1]");
        serialPrintln("set dateinterval 99999");
        serialPrintln("set datesec 99999");
        serialPrintln("set udatasec 99999");
        serialPrintln("set bright daytime [1 to 9]");
        serialPrintln("set bright night [1 to 9]");
        serialPrintln("set hour daytime [0 to 23]");
        serialPrintln("set hour night [0 to 23]");
        serialPrintln("set 12/24 format [0 or 1]");
        serialPrintln("cathod -> Cathod poisoning");
        serialPrintln("sekai  -> STEINS;GATE like effect");
        serialPrintln("save   -> write EEPROM");
        serialPrintln("show   -> show all setting");
        serialPrintln("time   -> change to clock mode");
        serialPrintln("");
        serialPrintln("99999: numeric number 0 to 65535");
        serialPrintln("---");
      } 
      if(strncmp(serial0String, "set timeformat ", 15) == 0) {
        unsigned int i = atoi(&serial0String[15]);
        if(i == 0 || i == 1) {
          dispTimeFormat = i;
          serialPrint("value = ");
          serialPrintln(dispTimeFormat);
        }
      } 
      if(strncmp(serial0String, "set time ", 9) == 0) {
        unsigned char year = ((serial0String[9] & 0x0F)<<4);
        year |= serial0String[10] & 0x0F;
        unsigned char month = ((serial0String[11] & 0x0F)<<4);
        month |= serial0String[12] & 0x0F;
        unsigned char days = ((serial0String[13] & 0x0F)<<4);
        days |= serial0String[14] & 0x0F;
        unsigned char hour = ((serial0String[16] & 0x0F)<<4);
        hour |= serial0String[17] & 0x0F;
        unsigned char minutes = ((serial0String[18] & 0x0F)<<4);
        minutes |= serial0String[19] & 0x0F;
        unsigned char seconds = ((serial0String[20] & 0x0F)<<4);
        seconds |= serial0String[21] & 0x0F;
        rtc.set(year, month, 1, days, hour, minutes, seconds);
      }
      if(strncmp(serial0String, "set dateinterval ", 17) == 0) {
        displayDateIntervalValue = atoi(&serial0String[17]);
        serialPrint("value = ");
        serialPrintln(displayDateIntervalValue);
      }
      if(strncmp(serial0String, "set datesec ", 12) == 0) {
        displayDateSecondsValue = atoi(&serial0String[12]);
        serialPrint("value = ");
        serialPrintln(displayDateSecondsValue);
      }
      if(strncmp(serial0String, "set udatasec ", 13) == 0) {
        userDataTimerValue = atoi(&serial0String[13]);
        serialPrint("value = ");
        serialPrintln(userDataTimerValue);
      }
      if(strncmp(serial0String, "cathod", 6) == 0) {
        cathodePoisoning();
        serialPrintln("Done.");
      }
      if(strncmp(serial0String, "sekai", 5) == 0) {
        steins();
        //serialPrintln("Done.");
      }
      if(strncmp(serial0String, "set bright daytime ", 19) == 0) {
        unsigned int i = atoi(&serial0String[19]);
        if(i > 0 && i < 10) {
          IN12BrightDaytime = i;
          serialPrint("value = ");
          serialPrintln(IN12BrightDaytime);
        }
      }
      if(strncmp(serial0String, "set bright night ", 17) == 0) {
        unsigned int i = atoi(&serial0String[17]);
        if(i > 0 && i < 10) {
          IN12BrightNight = i;
          serialPrint("value = ");
          serialPrintln(IN12BrightNight);
        }
      }
      if(strncmp(serial0String, "set hour daytime ", 17) == 0) {
        unsigned int i = atoi(&serial0String[17]);
        if(i >= 0 && i < 24) {
          hourDaytime = i;
          serialPrint("value = ");
          serialPrintln(hourDaytime);
        }
      }
      if(strncmp(serial0String, "set hour night ", 15) == 0) {
        unsigned int i = atoi(&serial0String[15]);
        if(i >= 0 && i < 24) {
          hourNight = i;
          serialPrint("value = ");
          serialPrintln(hourNight);
        }
      }
      if(strncmp(serial0String, "set 12/24 format ", 17) == 0) {
        unsigned int i = atoi(&serial0String[17]);
        if(i == 0 || i == 1) {
          hour12_24format = i;
          serialPrint("value = ");
          serialPrintln(hour12_24format);
        }
      } 
      if(strncmp(serial0String, "save", 4) == 0) {
        saveEEPROM();
        serialPrintln("EEPROM write complete");
      }
      if(strncmp(serial0String, "time", 4) == 0) {
        userDataTimerCount = 0;
      }
      if(strncmp(serial0String, "show", 4) == 0) {
        serialPrint("set timeformat: ");
        serialPrintln(dispTimeFormat);
        serialPrint("set dateinterval: ");
        serialPrintln(displayDateIntervalValue);
        serialPrint("set datesec: ");
        serialPrintln(displayDateSecondsValue);
        serialPrint("set udatasec: ");
        serialPrintln(userDataTimerValue);
        serialPrint("set bright daytime: ");
        serialPrintln(IN12BrightDaytime);
        serialPrint("set bright night: ");
        serialPrintln(IN12BrightNight);
        serialPrint("set hour daytime: ");
        serialPrintln(hourDaytime);
        serialPrint("set hour night: ");
        serialPrintln(hourNight);
        serialPrint("set 12/24 format: ");
        serialPrintln(hour12_24format);
        serialPrintln("---");
      }
    }
    serial0StringIdx = 0;
    serial0StringComplete = false;
  }
  // STEINS;GATE 風エフェクト表示
  // 0.5秒毎にPORTF0をチェック
//  if(interruptCount == 499 && steinsOVF == false) {
//    steinsOVF = true;
//    if(digitalRead(GP01) == LOW) {
//      steins();
//    }
//  }
}

// コマンド処理の応答表示を PE2 のピン状態で振り分け
// 0 = Serial1, 1 = Serial
void serialPrintln(char *buf) {
  if(bit_is_clear(PINE, PINE2)) {
    Serial1.println(buf);
  } 
  else {
    Serial.println(buf);
  }
}

void serialPrint(char *buf) {
  if(bit_is_clear(PINE, PINE2)) {
    Serial1.print(buf);
  } 
  else {
    Serial.print(buf);
  }
}

void serialPrintln(unsigned int val) {
  if(bit_is_clear(PINE, PINE2)) {
    Serial1.println(val);
  } 
  else {
    Serial.println(val);
  }
}

void serialPrintln(unsigned char val) {
  if(bit_is_clear(PINE, PINE2)) {
    Serial1.println(val);
  } 
  else {
    Serial.println(val);
  }
}

// ------------------------------------------------------------
// -- EEPROM 書き込み処理
// ------------------------------------------------------------
void saveEEPROM() {
  unsigned char byteHigh;
  unsigned char byteLow;

  byteLow = displayDateIntervalValue & 0x0F;
  byteHigh = displayDateIntervalValue >> 4;
  EEPROM.write(0, byteLow);
  EEPROM.write(1, byteHigh);

  byteLow = displayDateSecondsValue & 0x0F;
  byteHigh = displayDateSecondsValue >> 4;
  EEPROM.write(2, byteLow);
  EEPROM.write(3, byteHigh);

  byteLow = userDataTimerValue & 0x0F;
  byteHigh = userDataTimerValue >> 4;
  EEPROM.write(4, byteLow);
  EEPROM.write(5, byteHigh);

  EEPROM.write(6, dispTimeFormat);
  EEPROM.write(7, IN12BrightDaytime);
  EEPROM.write(8, IN12BrightNight);
  EEPROM.write(9, hourDaytime);
  EEPROM.write(10, hourNight);
  EEPROM.write(11, hour12_24format);
}

// ------------------------------------------------------------
// -- EEPROM 読み出し処理
// ------------------------------------------------------------
void readEEPROM() {
  unsigned char byteHigh;
  unsigned char byteLow;

  byteLow = EEPROM.read(0);
  byteHigh = EEPROM.read(1);
  displayDateIntervalValue = byteHigh << 4;
  displayDateIntervalValue |= byteLow;

  byteLow = EEPROM.read(2);
  byteHigh = EEPROM.read(3);
  displayDateSecondsValue = byteHigh << 4;
  displayDateSecondsValue |= byteLow;

  byteLow = EEPROM.read(4);
  byteHigh = EEPROM.read(5);
  userDataTimerValue = byteHigh << 4;
  userDataTimerValue |= byteLow;

  dispTimeFormat = EEPROM.read(6);
  if(dispTimeFormat > 1) {
    dispTimeFormat = 1;
  }
  IN12BrightDaytime = EEPROM.read(7);
  if(IN12BrightDaytime > 9) {
    IN12BrightDaytime = 5;
  }
  IN12BrightNight = EEPROM.read(8);
  if(IN12BrightNight > 9) {
    IN12BrightNight = 5;
  }
  hourDaytime = EEPROM.read(9);
  if(hourDaytime > 23) {
    hourDaytime = 7;
  }
  hourNight = EEPROM.read(10);
  if(hourNight > 23) {
    hourNight = 22;
  }
  hour12_24format = EEPROM.read(11);
  if(hour12_24format > 1) {
    hour12_24format = 1;
  }
}

// ------------------------------------------------------------
// -- カソードポイズニング防止(全セグメント点灯)
// ------------------------------------------------------------
void cathodePoisoning() {
  unsigned char a[17] = { 
    0,1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,6     };

  userDataTimerCount = userDataTimerValue;
  for(int i = 0; i < 8; i++) {
    IN12Com[i] = 1;
  }
  for(int j = 0; j < 5; j++) {
    for(int i = 0; i < 10; i++) {
      IN12Num[0] = a[i];
      IN12Num[1] = a[i+1];
      IN12Num[2] = a[i+2];
      IN12Num[3] = a[i+3];
      IN12Num[4] = a[i+4];
      IN12Num[5] = a[i+5];
      IN12Num[6] = a[i+6];
      IN12Num[7] = a[i+7];
      delay(800);
    }
  }
  for(int i = 0; i < 8; i++) {
    IN12Num[i] = 0;
    IN12Com[i] = 0;
  }
  userDataTimerCount = 0;
} 


// ------------------------------------------------------------
// -- STEINS;GATE ダイバージェンスメーター風エフェクト
// ------------------------------------------------------------
void steins() {
  long hendouRitsu[19] = {
    571024,
    571015,
    523299,
    456903,
    409420,
    37187,
    409431,
    456914,
    523307,
    571046,
    334581,
    1130426,
    1129848,
    1130205,
    1130212,
    1130238,
    2615074,
    3014368,
    4389117     
  };
  char hendouRitsuStr[10];
  unsigned char charValid[8] = { 
    1, 1, 0, 0, 0, 0, 0, 0
  };
  int charValidIdx;
  int randNumber;
  char randNumber2;
  boolean loopComplete = false;
  int i;
  int j;

  // カンマクリア
  for(i = 0; i < 8; i++) {
    IN12Com[i] = 0;
  }
  // 時計表示からユーザーデータ表示に切り替え
  userDataTimerCount = userDataTimerValue;

  // 変動率をピックアップして7桁の文字列変換
  randNumber = (int)random(20);
  sprintf(hendouRitsuStr, "%07ld", hendouRitsu[randNumber]);

  // 2桁目はカンマ、スペース固定
  IN12Num[1] = 0x0F;
  IN12Com[1] = 1;

  // 1, 3～8桁目をランダムに表示し1桁目の表示を確定させる
  randNumber = (int)random(400);
  for(i = 0; i < randNumber; i++) {
    for(j = 0; j < 8; j++) {
      randNumber2 = (char)random(10);
      IN12Num[j] = randNumber2;
      IN12Num[1] = 0x0F;
    }
    //delay(2);
  }
  IN12Num[0] = hendouRitsuStr[0] - 0x30;

  // 3～8桁目をランダム表示。
  do {
    charValidIdx = (int)random(2,8);
    // 表示確定している桁はスキップ
    if(charValid[charValidIdx] == 1) {
      continue;
    }

    randNumber = random(200);
    for(i = 0; i < randNumber; i++) {
      for(j = 2; j < 8; j++) {
        randNumber2 = random(10);
        if(charValid[j] == 0) {
          IN12Num[j] = randNumber2;
        }
      }
      //delay(2);
    }
    IN12Num[charValidIdx] = hendouRitsuStr[charValidIdx - 1] - 0x30;
    charValid[charValidIdx] = 1;
    // 全桁表示完了したかチェック
    loopComplete = true;
    for(i = 0; i < 8; i++) {
      if(charValid[i] == 0) {
        loopComplete = false;
      }
    }
  } while(loopComplete == false);
  userDataTimerCount = userDataTimerValue;
} 


