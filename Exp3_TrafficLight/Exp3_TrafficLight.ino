/*
 * ============================================================================
 * 实验三：流水灯 / 简易交通灯（数码管倒计时 + 外部中断紧急控制）
 * 北京交通大学 电子信息工程学院 大学生创新活动中心
 * ----------------------------------------------------------------------------
 * 接线：
 *   红灯 -> D8     黄灯 -> D7     绿灯 -> D6
 *   每个 LED 长脚串 220Ω 电阻接对应数字引脚，短脚接 GND
 *   TM1637  CLK -> D10    DIO -> D9    VCC -> 5V    GND -> GND
 *   紧急控制跳线：D2 与 GND 之间插一根杜邦线
 *                 插上 -> D2 被拉低 -> 进入紧急控制
 *                 拔出 -> D2 被内部上拉拉高 -> 恢复正常运行
 *
 * 功能：
 *   1. 简易交通灯循环：绿灯 15s -> 黄灯 3s -> 红灯 15s，循环往复
 *   2. 数码管显示当前相位剩余秒数（右侧两位，倒计时）
 *   3. D2 电平变化触发外部中断（attachInterrupt），立刻进入紧急控制：
 *      - 全部灯熄灭，仅红灯持续闪烁（0.5s 亮 / 0.5s 灭）
 *      - 数码管显示 00，倒计时暂停
 *   4. 紧急控制期间用串口持续输出本组组号与报警信息，解除后串口给出恢复提示
 *
 * 说明：
 *   - 中断服务程序里不能调用 delay()，所以 ISR 只置标志位，真正的动作放在 loop() 里做；
 *   - 全部逻辑用 millis() 计时，不使用长 delay()，保证串口与中断响应及时。
 * ============================================================================
 */
#include "TM1637.h"

/* ------------------------- 需要改的地方 ------------------------- */
#define GROUP_NO      0        // TODO: 改成你们组的组号（例如 5 就写 5）
/* --------------------------------------------------------------- */

#define PIN_RED       8
#define PIN_YELLOW    7
#define PIN_GREEN     6
#define PIN_EMERGENCY 2        // UNO 外部中断引脚 INT0

#define CLK_PIN       10
#define DIO_PIN       9

#define GREEN_SECONDS   15
#define YELLOW_SECONDS   3
#define RED_SECONDS     15

#define EMERG_BLINK_MS  500UL  // 紧急状态红灯闪烁半周期

TM1637 tm1637(CLK_PIN, DIO_PIN);

enum Phase { PHASE_GREEN = 0, PHASE_YELLOW = 1, PHASE_RED = 2 };

const uint8_t PHASE_SECONDS[3] = { GREEN_SECONDS, YELLOW_SECONDS, RED_SECONDS };

Phase         phase          = PHASE_GREEN;
unsigned long phaseStartMs   = 0;
int           lastShownSec   = -1;

volatile bool interruptFlag  = false;   // ISR 只负责置位
bool          emergencyActive = false;
unsigned long lastEmergPrint = 0;
unsigned long lastRedBlink   = 0;
bool          redBlinkOn     = false;

/* --------------------------- 工具函数 --------------------------- */

void printGroupTag() {
  Serial.print(F("[组号 "));
  Serial.print(GROUP_NO);
  Serial.print(F("] "));
}

/* 中断服务程序：D2 电平发生变化（插上/拔出导线）时被调用 */
void onEmergencyChange() {
  interruptFlag = true;
}

void applyLights() {
  digitalWrite(PIN_GREEN,  phase == PHASE_GREEN  ? HIGH : LOW);
  digitalWrite(PIN_YELLOW, phase == PHASE_YELLOW ? HIGH : LOW);
  digitalWrite(PIN_RED,    phase == PHASE_RED    ? HIGH : LOW);
}

void setPhase(Phase p) {
  phase        = p;
  phaseStartMs = millis();
  lastShownSec = -1;
  applyLights();

  const char* name = (p == PHASE_GREEN) ? "绿灯" : (p == PHASE_YELLOW) ? "黄灯" : "红灯";
  printGroupTag();
  Serial.print(F("相位切换 -> "));
  Serial.print(name);
  Serial.print(F("，倒计时 "));
  Serial.print(PHASE_SECONDS[p]);
  Serial.println(F(" s"));
}

/* 数码管显示倒计时（右侧两位，左侧两位留空） */
void showCountdown(uint8_t seconds) {
  tm1637.point(POINT_OFF);
  tm1637.display(0, 0x7f);              // 0x7f = 该位熄灭
  tm1637.display(1, 0x7f);
  tm1637.display(2, seconds / 10);
  tm1637.display(3, seconds % 10);
}

/* --------------------------- 紧急控制 --------------------------- */

void enterEmergency() {
  emergencyActive = true;
  redBlinkOn      = false;
  lastRedBlink    = millis();
  lastEmergPrint  = millis();

  digitalWrite(PIN_GREEN,  LOW);
  digitalWrite(PIN_YELLOW, LOW);
  digitalWrite(PIN_RED,    LOW);

  tm1637.point(POINT_OFF);
  tm1637.display(0, 0);
  tm1637.display(1, 0);
  tm1637.display(2, 0);
  tm1637.display(3, 0);

  printGroupTag();
  Serial.println(F("!!! 紧急控制触发：D2 被拉低，红灯闪烁，倒计时暂停 !!!"));
}

void emergencyLoop() {
  unsigned long now = millis();

  /* 红灯闪烁，不阻塞 */
  if (now - lastRedBlink >= EMERG_BLINK_MS) {
    lastRedBlink = now;
    redBlinkOn = !redBlinkOn;
    digitalWrite(PIN_RED, redBlinkOn ? HIGH : LOW);
  }

  /* 串口每秒提示一次，带上组号 */
  if (now - lastEmergPrint >= 1000UL) {
    lastEmergPrint = now;
    printGroupTag();
    Serial.println(F("紧急状态：交通灯已锁定，等待拔出 D2 上的导线恢复"));
  }
}

void exitEmergency() {
  emergencyActive = false;
  digitalWrite(PIN_RED, LOW);

  printGroupTag();
  Serial.println(F("紧急控制解除，交通灯从绿灯相位重新开始"));

  setPhase(PHASE_GREEN);
}

/* --------------------------- 正常交通灯 --------------------------- */

void normalLoop() {
  unsigned long elapsedMs = millis() - phaseStartMs;
  uint8_t duration = PHASE_SECONDS[phase];
  int remaining = (int)duration - (int)(elapsedMs / 1000UL);

  if (remaining < 0) {                 // 当前相位结束，切下一个
    Phase next;
    if      (phase == PHASE_GREEN)  next = PHASE_YELLOW;
    else if (phase == PHASE_YELLOW) next = PHASE_RED;
    else                            next = PHASE_GREEN;

    setPhase(next);
    remaining = PHASE_SECONDS[next];
  }

  if (remaining != lastShownSec) {     // 秒数变化才刷数码管，避免频繁刷新
    lastShownSec = remaining;
    showCountdown((uint8_t)remaining);
  }
}

/* ------------------------------ 主流程 ------------------------------ */

void setup() {
  Serial.begin(9600);

  pinMode(PIN_RED,    OUTPUT);
  pinMode(PIN_YELLOW, OUTPUT);
  pinMode(PIN_GREEN,  OUTPUT);
  digitalWrite(PIN_RED,    LOW);
  digitalWrite(PIN_YELLOW, LOW);
  digitalWrite(PIN_GREEN,  LOW);

  pinMode(PIN_EMERGENCY, INPUT_PULLUP);   // 平时被上拉为高，插线到 GND 即变低
  attachInterrupt(digitalPinToInterrupt(PIN_EMERGENCY), onEmergencyChange, CHANGE);

  tm1637.init();
  tm1637.set(BRIGHT_TYPICAL);

  printGroupTag();
  Serial.println(F("实验三 简易交通灯 启动（绿 15s -> 黄 3s -> 红 15s）"));

  setPhase(PHASE_GREEN);
}

void loop() {
  if (interruptFlag) {
    interruptFlag = false;               // 中断已响应，具体动作在下面按电平判断
  }

  bool emergencyNow = (digitalRead(PIN_EMERGENCY) == LOW);

  if (emergencyNow) {
    if (!emergencyActive) enterEmergency();
    emergencyLoop();
    return;
  }

  if (emergencyActive) {
    exitEmergency();
  }

  normalLoop();
}
