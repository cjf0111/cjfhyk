/*
 * ============================================================================
 * 实验一：DHT11 温湿度传感器的使用
 * 北京交通大学 电子信息工程学院 大学生创新活动中心
 * ----------------------------------------------------------------------------
 * 接线（DHT11 模块只有数字输出，必须接数字引脚并加上拉电阻，模块板自带 10k）：
 *   DHT11  VCC  -> UNO 5V
 *   DHT11  GND  -> UNO GND
 *   DHT11  S    -> UNO D2
 *   LED    长脚（正极）-> UNO D8（串 220Ω 限流电阻）  短脚（负极）-> GND
 *
 * 功能：
 *   1. 串口输出传感器数据前，先输出本组组号（见下方 GROUP_NO）
 *   2. 把温度变化转换成 LED 的闪烁：温度上升闪得快，温度下降闪得慢
 *
 * 说明：
 *   - 闪烁用 millis() 做非阻塞计时，读传感器时灯照样在闪；
 *   - DHT11 采样间隔不能小于 1s，这里每 2s 读一次；
 *   - 温度到闪烁间隔的换算：18℃ 及以下 -> 2000ms（最慢），38℃ 及以上 -> 100ms（最快），
 *     中间线性过渡，可自行调整下面的 TEMP_LOW / TEMP_HIGH。
 * ============================================================================
 */
#include "DHT.h"

/* ------------------------- 需要改的地方 ------------------------- */
#define GROUP_NO   11       // 第 11 组（按实际组号设置）
/* --------------------------------------------------------------- */

#define DHTPIN     2        // DHT11 数据脚
#define DHTTYPE    DHT11    // 模块是 DHT11（若是 DHT22/AM2302 改成 DHT22）
#define LED_PIN    8        // 用于指示温度的 LED

#define READ_INTERVAL_MS   2000UL   // 采样间隔
#define TEMP_LOW           18.0     // 低于该温度，灯闪得最慢
#define TEMP_HIGH          38.0     // 高于该温度，灯闪得最快
#define BLINK_MAX_MS       2000UL   // 最慢闪烁间隔
#define BLINK_MIN_MS       100UL    // 最快闪烁间隔

DHT dht(DHTPIN, DHTTYPE);

unsigned long blinkInterval = BLINK_MAX_MS;  // 当前闪烁间隔
unsigned long lastToggle    = 0;
unsigned long lastRead      = 0;
bool          ledOn         = false;

/* 温度 -> 闪烁间隔（温度越高，间隔越小，闪得越快） */
unsigned long intervalByTemperature(float t) {
  if (t <= TEMP_LOW)  return BLINK_MAX_MS;
  if (t >= TEMP_HIGH) return BLINK_MIN_MS;

  float k = (t - TEMP_LOW) / (TEMP_HIGH - TEMP_LOW);          // 0 ~ 1
  return (unsigned long)(BLINK_MAX_MS - k * (BLINK_MAX_MS - BLINK_MIN_MS));
}

/* 串口输出，前面统一带上组号 */
void printGroupTag() {
  Serial.print(F("[组号 "));
  Serial.print(GROUP_NO);
  Serial.print(F("] "));
}

void setup() {
  Serial.begin(9600);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  dht.begin();

  printGroupTag();
  Serial.println(F("实验一 DHT11 温湿度传感器 启动"));
  delay(2000);   // DHT11 上电后需要一点稳定时间
}

void loop() {
  unsigned long now = millis();

  /* ---------- 1. 定时采样并打印 ---------- */
  if (now - lastRead >= READ_INTERVAL_MS) {
    lastRead = now;

    float h = dht.readHumidity();
    float t = dht.readTemperature();

    printGroupTag();

    if (isnan(h) || isnan(t)) {
      Serial.println(F("读取失败：请检查接线（VCC/GND/S -> 5V/GND/D2）"));
    } else {
      /* 温度变化 -> 闪烁快慢 */
      blinkInterval = intervalByTemperature(t);

      Serial.print(F("湿度 "));
      Serial.print(h, 1);
      Serial.print(F(" %RH    温度 "));
      Serial.print(t, 1);
      Serial.print(F(" C    闪烁间隔 "));
      Serial.print(blinkInterval);
      Serial.println(F(" ms"));
    }
  }

  /* ---------- 2. 非阻塞闪烁 ---------- */
  if (now - lastToggle >= blinkInterval) {
    lastToggle = now;
    ledOn = !ledOn;
    digitalWrite(LED_PIN, ledOn ? HIGH : LOW);
  }
}
