# STM32F103C8T6-desktop-pet

An electronic desktop pet running on STM32F103C8T6.

一个基于 STM32F103C8T6 + 0.96 寸 SSD1306 OLED 的桌面电子宠物项目。

## 目录

- `User/` —— 标准库工程源码(OLED 驱动、宠物动画、串口指令框架、接入指南)
- `README_接入指南.md` —— 详细的接法、协议、排查说明
- `pet_frames.h` / `gen_assets.py` —— 像素帧与重新生成工具

## 快速上手

1. 把 `User/` 里的 `.c` / `.h` 文件加入你的标准库 Keil 工程
2. 接线: OLED 接 PB6/PB7, 串口/蓝牙接 PA10/PA9
3. 烧录后宠物会播放待机动画(睁眼 → 眨眼 → 开心循环)
4. 串口发送 `HAPPY` / `BLINK` / `NORMAL` 可切换表情

## 硬件

- MCU: STM32F103C8T6(蓝莓派最小系统板)
- 屏幕: 0.96" OLED, SSD1306, I2C(SCL→PB6, SDA→PB7, VCC→3.3V, GND 共地)
- 烧录: ST-Link V2(只接 SWCLK/SWDIO/GND, 板子 USB 独立供电)
- 串口: PA9(TX), PA10(RX), 115200 baud

## 详细说明

见 **[README_接入指南.md](README_接入指南.md)**。