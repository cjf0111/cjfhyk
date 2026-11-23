# Arduino UNO 秋季实验（DHT11 / DS1307 + TM1637 / 简易交通灯）

北京交通大学 电子信息工程学院 大学生创新活动中心 —— 25 秋季实验课的三个 Arduino 实验代码。

开发板：Arduino UNO ｜ 开发环境：Arduino IDE 2.2.1 ｜ 编译目标：`arduino:avr:uno`

> **动手前先改组号**：三个代码文件顶部的 `#define GROUP_NO 0` 改成你们组的组号，串口输出和数码管都会用到它。

## 目录结构

| 目录 | 对应实验 | 说明 |
| --- | --- | --- |
| `Exp1_DHT11/` | 实验一 温湿度传感器 | DHT11 + LED，串口输出组号与温湿度 |
| `Exp2_RTC_TM1637/` | 实验二 时钟与数码管 | DS1307 + TM1637，数码管显示“组号：分钟” |
| `Exp3_TrafficLight/` | 实验三 流水灯 / 交通灯 | LED + TM1637，倒计时 + 外部中断紧急控制 |
| `RTCtester/` | 指导书原例程 | 未修改，留作对照 |

## 库依赖

Arduino IDE 中「工具 → 管理库」安装，或「项目 → 加载库 → 添加 .ZIP 库」导入 `实验准备.zip` 里的压缩包：

- DHT sensor library（含依赖 Adafruit Unified Sensor）
- RTClib
- TM1637
- TimerOne（本仓库代码未用到，按指导书一并安装）

## 接线一览

### 实验一 DHT11

| 元件 | 引脚 | 接到 UNO |
| --- | --- | --- |
| DHT11 | VCC | 5V |
| DHT11 | GND | GND |
| DHT11 | S（数字输出） | D2 |
| LED | 长脚（正极）经 220Ω | D8 |
| LED | 短脚（负极） | GND |

功能：串口每 2s 打印一次 `[组号 X] 湿度 xx %RH 温度 xx C 闪烁间隔 xxx ms`；温度 18℃ 及以下 LED 最慢闪（2s 一次），38℃ 及以上最快闪（0.1s 一次），中间线性过渡。想换灯脚或温度区间，改文件顶部宏即可。

### 实验二 DS1307 + TM1637

| 元件 | 引脚 | 接到 UNO |
| --- | --- | --- |
| DS1307 | VCC / GND | 5V / GND |
| DS1307 | SDA | A4 |
| DS1307 | SCL | A5 |
| TM1637 | VCC / GND | 5V / GND |
| TM1637 | CLK | D10 |
| TM1637 | DIO | D9 |

功能：数码管固定两位显示组号 + 两位显示当前分钟，中间冒号点亮，如组号 05、当前 23 分 → `05:23`；串口每秒输出 `[组号 05] 2026/11/23 19:23:45 分钟 = 23`。首次运行若 DS1307 未启动，程序会用编译时刻自动对时。

### 实验三 简易交通灯 + 紧急控制

| 元件 | 引脚 | 接到 UNO |
| --- | --- | --- |
| 红灯 | 长脚经 220Ω | D8 |
| 黄灯 | 长脚经 220Ω | D7 |
| 绿灯 | 长脚经 220Ω | D6 |
| 三个 LED | 短脚 | GND |
| TM1637 | CLK / DIO / VCC / GND | D10 / D9 / 5V / GND |
| 紧急跳线 | 一端 D2，另一端 GND | 插上 = 紧急，拔出 = 恢复 |

功能：
- 绿灯 15s → 黄灯 3s → 红灯 15s 循环，数码管右侧两位显示当前相位剩余秒数；
- D2 电平变化触发外部中断（`attachInterrupt` + `CHANGE`），立刻进入紧急控制：三个灯全灭、红灯 0.5s 闪烁、数码管显示 `00`、倒计时暂停；
- 紧急期间串口每秒打印 `[组号 X] 紧急状态：交通灯已锁定...`，拔出跳线后串口提示恢复并从绿灯相位重新开始；
- 中断服务程序里只置标志位，所有动作放在 `loop()` 中用 `millis()` 完成（中断里 `delay()` 不工作）。

## 编译与下载

1. 打开 `ExpX_.../ExpX_....ino`，IDE 中选择开发板 `Arduino UNO` 与对应端口；
2. 若端口不出现，先运行 `CH341SER-Driver/SETUP.EXE` 安装 USB 转串口驱动；
3. 点「上传」，用「工具 → 串口监视器」（9600 波特率）查看输出。

命令行编译（可选）：

```powershell
& "C:\Users\chenj\AppData\Local\Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe" `
  compile --fqbn arduino:avr:uno .\Exp1_DHT11
```

## 提交与远端

远端仓库：`git@github.com:CRgeF/DHT11_TEST_1_V1.git`（分支 `main`）。SSH 走 `ssh.github.com:443`，已在本机 `~/.ssh/config` 配置好。
