/*
 * ============================================================================
 * 实验二：DS1307 实时时钟 + TM1637 数码管的使用
 * 北京交通大学 电子信息工程学院 大学生创新活动中心
 * ----------------------------------------------------------------------------
 * 接线：
 *   DS1307  VCC -> 5V      GND -> GND
 *           SDA -> A4      SCL -> A5      （UNO 的硬件 I2C 引脚，不要接错）
 *   TM1637  VCC -> 5V      GND -> GND
 *           CLK -> D10     DIO -> D9       （指导书要求 CLK 接 PIN10、DIO 接 PIN9）
 *
 * 功能：
 *   1. 数码管显示“组号：分钟”，中间冒号点亮 —— 例：组号 05、当前 23 分 显示 05:23
 *   2. 串口输出数据前，先输出本组组号，格式如：[组号 05] 2026/11/23 19:23:45
 *
 * 说明：
 *   - 首次运行若 DS1307 未启动，会用本程序编译时刻（__DATE__ / __TIME__）给它对时；
 *   - RTC 模块带纽扣电池时断电时间仍会走，不需要每次重新设置；
 *   - 组号显示方式是固定两位：组号 <10 时高位补 0（第 5 组显示 05:MM）。
 * ============================================================================
 */
#include "TM1637.h"
#include "RTClib.h"

/* ------------------------- 需要改的地方 ------------------------- */
#define GROUP_NO   11       // 第 11 组（按实际组号设置）
/* --------------------------------------------------------------- */

#define CLK_PIN    10       // TM1637 CLK -> D10
#define DIO_PIN    9        // TM1637 DIO -> D9

TM1637   tm1637(CLK_PIN, DIO_PIN);
RTC_DS1307 rtc;

int lastSecond = -1;

void printGroupTag() {
  Serial.print(F("[组号 "));
  Serial.print(GROUP_NO);
  Serial.print(F("] "));
}

/* 两位数字拆分显示用的辅助函数 */
void showGroupAndMinute(uint8_t minute) {
  tm1637.point(POINT_ON);                 // 点亮中间冒号，读作“组号：分钟”
  tm1637.display(0, (GROUP_NO / 10) % 10);
  tm1637.display(1,  GROUP_NO % 10);
  tm1637.display(2,  minute / 10);
  tm1637.display(3,  minute % 10);
}

void setup() {
  Serial.begin(9600);

  tm1637.init();
  tm1637.set(BRIGHT_TYPICAL);             // 亮度：BRIGHT_DARKEST ~ BRIGHTEST
  tm1637.clearDisplay();

  if (!rtc.begin()) {
    Serial.println(F("Couldn't find RTC —— 检查 DS1307 的 SDA(A4)/SCL(A5)/VCC/GND"));
    while (1) { delay(10); }
  }

  if (!rtc.isrunning()) {
    Serial.println(F("RTC is NOT running —— 用编译时刻对时"));
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  printGroupTag();
  Serial.println(F("实验二 DS1307 + TM1637 启动"));
}

void loop() {
  DateTime now = rtc.now();

  /* 数码管：组号：分钟 */
  showGroupAndMinute(now.minute());

  /* 串口：每秒打印一次本组组号 + 完整时间 */
  if (now.second() != lastSecond) {
    lastSecond = now.second();

    printGroupTag();
    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(' ');
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.print(F("    分钟 = "));
    Serial.println(now.minute(), DEC);
  }

  delay(200);   // 降低刷新频率，避免数码管闪烁
}
