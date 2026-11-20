[English](README_US.md) | [简体中文](README.md)
# ESP32-DIV（PlatformIO 移植版）

把 [ESP32-DIV](https://github.com/cifertech/ESP32-DIV) 移植到 PlatformIO + Arduino 框架，并按本板硬件重写了引脚、显示、触摸和两条射频总线。

原工程是 Arduino IDE 的 `.ino` 草图，依赖 PCF8574 物理按键、蜂鸣器、WS2812 状态灯和 XPT2046 电阻触摸。本板这些都没有，所以 UI 交互改成了**纯触摸导航条**，外接模块改成**按需接、缺了就在界面上直接报不可用**，不再因为少一个模块就启动不了。

---

## 一、硬件

| 部件 | 型号 / 规格 | 说明 |
|---|---|---|
| 主控 | ESP32-S3 N16R8 | 16MB QIO flash + 8MB OPI PSRAM，双核 240MHz |
| 屏幕 | 240×320 ILI9341 SPI | 竖屏原生方向，**不做横屏旋转** |
| 触摸 | FT6336 电容触摸（I2C） | 与屏幕一体的贴合屏 |
| 存储 | microSD | 与屏幕共用 SPI 总线，只换片选 |
| 2.4GHz | nRF24L01+ | 独立片选，扫描 / 定频干扰 |
| Sub-GHz | CC1101 | 与 nRF24 共用 SPI 的 SCK/MISO/MOSI，独立片选 |

### 接线

**屏幕（SPI2）**

| 信号 | GPIO |
|---|---|
| SCLK | 12 |
| MOSI | 11 |
| MISO | 13 |
| CS | 10 |
| DC | 9 |
| RST | 8 |
| 背光 BL | 21（PWM 调光） |

**SD 卡**：SCLK 12 / MOSI 11 / MISO 13（与屏幕完全共用），**CS = 4**。

**FT6336 触摸（I2C）**：SCL 1 / SDA 2 / RST 42，INT 未接（走轮询）。

**nRF24（SPI3）**：SCK 16 / MISO 17 / MOSI 18 / **CSN 15** / CE 14，IRQ 不接。

**CC1101（与 nRF24 同一条 SPI3）**：SCK 16 / MISO 17 / MOSI 18 / **CSN 5**，GDO0 6，GDO2 不接。

```
        ┌──────────── SPI2（屏幕 + SD）────────────┐
        │  SCLK 12   MOSI 11   MISO 13             │
        │  TFT_CS 10   SD_CS 4                     │
        └──────────────────────────────────────────┘

        ┌──────────── SPI3（nRF24 + CC1101）───────┐
        │  SCLK 16   MOSI 18   MISO 17             │
        │  NRF_CS 15   CC1101_CS 5                 │
        └──────────────────────────────────────────┘

        I2C（触摸）  SCL 1  SDA 2  RST 42
```

> **CC1101 的 VCC 只能接 3V3**。模块工作范围 1.8–3.6V，接 5V 直接报废。
>
> GDO0 接不接的区别：**扫描 / 干扰**是纯 SPI 操作，不接也能跑；**录制与回放遥控码**用的是 RCSwitch 的软件 OOK 编解码，走的就是 GDO0，不接就录不到码。

### 本板没有的东西

| 缺失 | 代码里的表现 |
|---|---|
| PCF8574 按键板 | `HAS_PCF8574_BUTTONS 0`，导航全靠屏幕下方 5 格触摸条 |
| 蜂鸣器 | `BUZZER_PIN -1` |
| WS2812 状态灯 | `STATUS_LED_COUNT 0` |
| 电池分压电阻 | `BATTERY_ADC_PIN -1`，`readBatteryVoltage()` 直接返回 `NAN` |
| PN532 / 红外 / GPS 模块 | 引脚全部为 `-1`，进入对应功能会提示不可用，不会去碰任何 GPIO |

---

## 二、功能

主菜单 2 列 × 4 行，共 8 项：

```
┌──────────┬──────────┐
│  WiFi    │  Tools   │
├──────────┼──────────┤
│  More    │  About   │
├──────────┼──────────┤
│ Settings │  2.4GHz  │
├──────────┼──────────┤
│Bluetooth │  SubGHz  │
└──────────┴──────────┘
```

| 主菜单 | 子功能 |
|---|---|
| WiFi | Packet Monitor / Beacon Spammer / WiFi Deauther / Probe Request Flood / Deauth Detector / WiFi Scanner / Captive Portal |
| 2.4GHz | Scanner（126 信道 RPD 频谱）/ Proto Kill（8 类目标定频跳频干扰） |
| SubGHz | Replay Attack / SubGHz Jammer / Saved Profile |
| Bluetooth | BLE Spoofer / Sour Apple / Sniffer / BLE Scanner / BLE Rubber Ducky |
| Tools | Serial Monitor / Update Firmware / SD File Manager |
| More | IR Remote（Record / Saved Profile / Universal Controller）、RFID/NFC（9 项）、GPS（Wardriver / Satellite Scanner） |
| Settings | Brightness / Theme / Accent / Auto Scan |

**交互方式**：屏幕底部有一条 5 格触摸导航条，从左到右依次对应 `左 / 下 / 中 / 上 / 右`。功能界面里中间格固定是 **Exit**，其余格子按当前界面定义（`Next`、`Run`、`Save` 之类）。主菜单是**点一下直接进**，不需要长按。

设置项会写到 SD 卡的 `/config/settings.json`，没有 SD 卡时用默认值。

---

## 三、编译与烧写

需要 [PlatformIO](https://platformio.org/)（VS Code 插件或命令行都行）。

```bash
# 编译
pio run

# 烧写（把 COM5 换成实际串口）
pio run -t upload --upload-port COM5

# 看启动日志（交互式，Ctrl+C 退出）
pio device monitor --port COM5 --baud 115200
```

非交互环境下 `pio device monitor` 会拒绝运行，用仓库自带的小脚本代替：

```bash
python tools/serial_probe.py COM5 115200 12 noreset
# 参数：串口 / 波特率 / 读取秒数 / noreset 表示不脉冲 DTR
```

> **为什么要 `noreset`**：脚本脉冲 DTR 强制复位时，UART 线上会有毛刺，复位后头几十毫秒的输出会被吞掉，看起来像「卡死在 `tft.init()`」。排查启动问题时一定要用 `noreset`，或者先用 esptool 硬复位再立刻读。

当前构建占用：**Flash 54.7%（约 1.68MB / 3.08MB）**，**RAM 34.8%（约 114KB / 320KB）**。

### 分区表

`partitions_16MB.csv`，双 OTA 槽 + 64KB coredump：

| 分区 | 偏移 | 大小 |
|---|---|---|
| nvs | 0x9000 | 20KB |
| otadata | 0xe000 | 8KB |
| app0 (ota_0) | 0x10000 | 3MB |
| app1 (ota_1) | 0x300000 | 3MB |
| spiffs | 0x5F0000 | 2MB |
| coredump | 0x7F0000 | 64KB |

> 这个 CSV **只能用 ASCII**。写成 GBK 中文注释会让 PlatformIO 的分区解析器直接抛 `UnicodeDecodeError`，构建中断。

---

## 四、目录结构

```
├── platformio.ini           构建配置（env: esp32-s3-devkitc-1）
├── partitions_16MB.csv      16MB flash 分区表
├── src/
│   ├── main.cpp             菜单、触摸导航条、setup/loop 主流程
│   ├── shared.h             全部引脚定义、UI 调色板、面板尺寸、功能开关
│   ├── config.h             公共 include 与各命名空间声明
│   ├── radio_bus.cpp/.h     SPI3 公共总线（nRF24 + CC1101 共用）
│   ├── nrf24.cpp/.h         2.4GHz Scanner + Proto Kill
│   ├── subghz.cpp/.h        CC1101 Sub-GHz 三功能
│   ├── wifi.cpp             WiFi 七件套
│   ├── bluetooth.cpp        BLE 五件套（NimBLE）
│   ├── ir.cpp               红外收发
│   ├── rfid.cpp             PN532 RFID/NFC
│   ├── gps.cpp              GPS / Wardriver
│   ├── ducky.cpp            BLE Rubber Ducky
│   ├── Touchscreen.cpp/.h   FT6336 驱动与坐标映射
│   ├── KeyboardUI.cpp/.h    屏幕软键盘
│   ├── SettingsStore.cpp/.h 设置持久化
│   ├── utils.cpp/.h         状态栏、通知、触摸导航条、SD 初始化等公共件
│   └── icon.h               位图图标
├── lib/                     依赖全部本地固定，构建时不联网
│   ├── TFT_eSPI/            2.5.43（含本板 User_Setup.h，已打 S3 补丁）
│   ├── FT6336-arduino/      电容触摸驱动
│   ├── NimBLE-Arduino/      BLE 协议栈
│   ├── IRremoteESP8266/     红外
│   ├── Adafruit_PN532/      RFID/NFC
│   ├── Adafruit_BusIO/      PN532 的 I2C/SPI 底层
│   ├── SmartRC-CC1101/      CC1101 驱动（已改为共享总线）
│   ├── rc-switch/           OOK 编解码（Replay / Profile）
│   ├── ArduinoJson/         配置读写
│   └── arduinoFFT/          WiFi 频谱
├── tools/serial_probe.py    串口日志抓取小工具
└── ESP32-DIV/               原 Arduino 工程，仅作参考，不参与构建
```

---

## 五、实现要点（踩过的坑）

### 1. 两条射频总线不能混

屏幕和 SD 走 **SPI2**，nRF24 和 CC1101 走 **SPI3**。`SPIClass` 的引脚映射是**一次性绑定**的——`begin()` 发现 `_spi` 非空就直接返回，之后再调也不会改引脚。所以：

- `tft.init()` 必须**先于** `SD.begin()`，否则屏幕引脚永远配不上；
- nRF24 与 CC1101 必须拿到**同一个** `SPIClass` 实例（`src/radio_bus.cpp`），否则第二次 `begin()` 会 `periph_module_reset(SPI3)`，把正在通信的那一路打断；
- `lib/TFT_eSPI/User_Setup.h` 里**绝对不能定义 `USE_HSPI_PORT`**。定义了 TFT_eSPI 会自建 `SPIClass spi = SPIClass(HSPI)`，和 SD 各持一个实例、各有一把 `_inTransaction`，事务互斥彻底失效。

### 2. TFT_eSPI 的 S3 端口号必须打补丁

上游 `Processors/TFT_eSPI_ESP32_S3.h` 默认分支写的是 `#define SPI_PORT FSPI`。而 Arduino core 在 S3 上把 `FSPI` 定义为 **0**，IDF 的 `REG_SPI_BASE(i)` 是 `(i >= 2) ? ... : 0`，于是 `SPI_USER_REG` 这类宏被常量折叠成 `0x10`、`SPI_W0_REG` 折叠成 `0x98`——`tft.init()` 发出的第一条命令就写到地址 `0x10`，直接 `StoreProhibited`，开机死循环。

已改为 `#define SPI_PORT 2`（对应 SPI2 窗口 `0x60024000`）。**不要用官方源码覆盖这个目录。**

### 3. CC1101 驱动原本会把整条总线拆掉

上游驱动每次寄存器访问都 `SPI.begin(...)` + `SPI.end()`，而且用的是全局 `SPI`（那就是 SPI2，屏幕那条）。已改成：`SpiStart()` 只准备自己的片选并 `beginTransaction()`，`SpiEnd()` 只 `endTransaction()`，所有传输走 `radioBus()`。

另外驱动里有个全局 `bool spi`，和 TFT_eSPI 导出的 `SPIClass& spi` 同名，链接期会 `multiple definition`。已改名 `static bool cc1101_customSpi`。

### 4. 触摸坐标只映射一次

FT6336 固定输出面板原生坐标（`ROTATION_NORMAL`），屏幕方向的变化统一在 `Touchscreen.cpp` 的 `mapTouchToScreen()` 里按 `TFT_ROTATION` 处理。不要在驱动层再转一次，否则双重旋转。

### 5. 菜单索引是列优先

主菜单布局是 2 列 × 4 行，索引算法 `col = i / 4`、`row = i % 4`。加菜单项时下面这几处必须同步改，漏一个就会出现「点左边跳右边」：

- `NUM_MENU_ITEMS` 与 `menu_items[]` / `bitmap_icons[]`
- `MAIN_MENU_*_IDX` 这批索引常量
- `updateActiveSubmenu()` 里的 `case`
- `handleButtons()` 里 `in_sub_menu` 的分发 `case`
- `displayMenu()` 里 `icon_colors[]` 的长度

### 6. 功能退出要靠 `feature_exit_requested`

每个 feature 的 `loop()` 开头都有：

```c
if (feature_active && (feature_exit_requested || featureExitButtonPressed())) {
    feature_exit_requested = true;
    return;
}
```

外层 `run*Feature()` 的 `while` 靠这个标志退出，退出后再统一把 `in_sub_menu` / `feature_active` / 触摸导航条复位。新增功能照这个模式写，不要自己 `return` 到主菜单。

---

## 六、已知限制

- **PN532 / 红外 / GPS 未接**：引脚为 `-1`，功能入口提示不可用。要恢复就把 `src/shared.h` 里对应宏改成实际引脚。
- **Replay Attack / Saved Profile 依赖 GDO0**：不接 GDO0 时这两个功能的录制与回放不工作，SubGHz Jammer 不受影响。
- **干扰类功能（Proto Kill / SubGHz Jammer）效果取决于环境和目标设备**，不要指望「一键全灭」。
- **状态栏电量恒为无效**：本板没有电池分压电路，`BATTERY_ADC_PIN -1` 时 `readBatteryVoltage()` 返回 `NAN`。
- **Settings 里没有触摸校准**：FT6336 出厂坐标已够准，映射参数写死在 `Touchscreen.cpp` 顶部，需要微调就改 `TOUCH_MAP_X1/X2/Y1/Y2`。
- `src/` 里还留着一些历史编译警告（`ICON_NUM` / `MAX_LINES` / `STATUS_BAR_Y_OFFSET` 重复定义等），不影响运行。

---

## 七、来源

- 原始项目：[ESP32-DIV](https://github.com/DeaDHackS/ESP32-DIV)——菜单结构、WiFi/BLE/IR/RFID/GPS/Ducky 各功能的界面与逻辑都来自这里
- [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI) by Bodmer
- [NimBLE-Arduino](https://github.com/h2zero/NimBLE-Arduino)
- [IRremoteESP8266](https://github.com/crankyoldgit/IRremoteESP8266)
- [SmartRC-CC1101-Driver-Lib](https://github.com/LSatan/SmartRC-CC1101-Driver-Lib) by LSatan
- [rc-switch](https://github.com/sui77/rc-switch) by sui77
- [Adafruit_PN532](https://github.com/adafruit/Adafruit-PN532) / [Adafruit_BusIO](https://github.com/adafruit/Adafruit_BusIO)
- [ArduinoJson](https://github.com/bblanchon/ArduinoJson) by Benoit Blanchon
- [arduinoFFT](https://github.com/kosme/arduinoFFT) by Enrique Condes

`lib/` 下的库都是本地固定的副本，其中 TFT_eSPI 与 SmartRC-CC1101 打了本板专用补丁（见上文第五节），**升级依赖前请先确认补丁是否还在**。

---

## 八、免责声明

这套工具会主动发射 2.4GHz / Sub-GHz 信号、发起 WiFi 与 BLE 相关操作。请只在**你自己拥有的设备**或**获得明确书面授权**的环境里使用。在多数国家和地区，对公共频段实施干扰、对他人网络发起认证攻击都是违法的。作者不对任何滥用行为负责。
