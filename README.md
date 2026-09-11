# STM32F103C8T6-desktop-pet

An electronic desktop pet running on STM32F103C8T6.

一个基于 STM32F103C8T6 + 0.96 寸 SSD1306 OLED 的桌面电子宠物项目。

## 目录

- `reference/` —— 参考代码（OLED 驱动、宠物动画、串口指令框架、接入指南）
- 详细说明见：**[reference/README_接入指南.md](reference/README_接入指南.md)**

## 快速上手

1. 把 `reference/` 里的 `.c` / `.h` 加入你的 Keil 工程（步骤见接入指南第 1 步）
2. 烧录后宠物会播放待机动画（睁眼 → 眨眼 → 开心 循环）
3. 串口发送 `HAPPY` / `BLINK` / `NORMAL` 可切换表情

## 硬件

- MCU: STM32F103C8T6（蓝莓派最小系统板）
- 屏幕: 0.96" OLED, SSD1306, I2C（SCL→PB6, SDA→PB7, VCC→3.3V, GND 共地）
- 烧录: ST-Link V2（只接 SWCLK/SWDIO/GND，板子 USB 独立供电）