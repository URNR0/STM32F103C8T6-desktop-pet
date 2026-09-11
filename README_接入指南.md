# 桌面宠物 STM32 标准库接入指南

> 这份指南对应的是 `User/` 目录下的标准库(STM32F10x_StdPeriph_Lib)代码。
> 如果你用的是 HAL 库, 请用回旧版本或告诉我。

## 1. 工程里要加哪些文件

把你的工程里的 `main.c` 对应替换, 并加入以下文件:

| 文件 | 作用 |
| --- | --- |
| `main.c` | 程序入口, 初始化 + 主循环 |
| `delay.c/h` | 标准库下提供 1ms 系统滴答和延时函数 |
| `oled.c/h` | SSD1306 驱动(软件 I2C, PB6/PB7) |
| `font8x8.h` | 8x8 ASCII 字库 |
| `pet.c/h` | 宠物动画: 睁眼、眨眼、开心待机 |
| `pet_frames.h` | 宠物三帧像素图(128x64) |
| `uart_cmd.c/h` | 串口指令环形缓冲 + 命令表 |
| `usart1.c/h` | 标准库串口1初始化 + 中断接收(PA9/PA10) |

## 2. 接线

### OLED (I2C)
- VCC -> 3.3V
- GND -> GND
- SCL -> PB6
- SDA -> PB7

### 串口 / 蓝牙(JDY-31)
- 模块 TX -> 板子 RX (PA10)
- 模块 RX -> 板子 TX (PA9)
- 模块 GND -> 板子 GND
- 模块 VCC -> 3.3V 或 5V(看模块要求)

**蓝牙透传不需要你写蓝牙协议!** 手机蓝牙串口助手连接 JDY-31 后, 发送 `HAPPY` 就会自动转发到 STM32 的串口, 和电脑串口助手效果一样。

## 3. Keil 里要确认的事

1. **芯片型号**: STM32F103C8
2. **启动文件**: `startup_stm32f10x_md.s` (中等密度)
3. **宏定义**: `USE_STDPERIPH_DRIVER, STM32F10X_MD`
   - 路径: 工程 -> Options for Target -> C/C++ -> Define
4. **包含路径**: 把 `User` 文件夹和 CMSIS/标准库头文件路径加进去
5. **不要勾选 MicroLIB 也能用**, 但如果想用 `printf`, 勾选 MicroLIB 并在 `usart1.c` 里重定向 `fputc` 即可

## 4. 代码逻辑一句话

- 上电后屏幕显示"正常脸"宠物, 然后按待机脚本循环播放动画。
- 串口收到 `NORMAL` / `BLINK` / `HAPPY` 会立即切换到对应表情。
- 主循环只做两件事:

```c
while (1) {
    UartCmd_Poll();   // 处理串口指令
    Pet_Update();     // 更新宠物动画
}
```

## 5. 串口协议(回车结束)

| 发送命令 | 效果 |
| --- | --- |
| `HAPPY` | 宠物变开心 |
| `BLINK` | 宠物眨眼(单帧) |
| `NORMAL` | 切回待机动画 |
| `PING` | 板子回 `PONG` |
| `HELP` | 返回支持的命令 |

命令不区分大小写, 以 `\r` 或 `\n` 结束。

## 6. 常见坑与排查

### 6.1 屏幕不亮

1. 确认 VCC 是 3.3V, GND 共好。
2. 确认 SCL/SDA 没接反, 且是 PB6/PB7。
3. 用万用表量 SCL/SDA 对 3.3V 是否有 4.7k 左右上拉电阻。
4. 尝试改 I2C 地址: `oled.h` 里 `#define OLED_I2C_ADDR 0x3C` 改成 `0x3D`。
5. 检查初始化命令里的 `0x8D, 0x14` 电荷泵命令有没有被注释掉。

### 6.2 串口收不到/只收一次

1. 确认 TX/RX 交叉接线: 模块 TX -> 板子 PA10; 模块 RX -> 板子 PA9。
2. 确认波特率一致: 默认 115200。
3. 确认串口中断在 `NVIC` 里开启了(看 `usart1.c` 里的 `NVIC_Init`)。
4. 确认启动文件里有 `USART1_IRQHandler` 这个中断向量名(标准库启动文件一般都有)。
5. 检查 Keil "Use MicroLIB" 是否勾选(不影响接收, 但影响 `printf`)。

### 6.3 提示 `SystemCoreClock` 未定义

- 标准库需要在 `system_stm32f10x.c` 里定义, 标准库模板工程通常已包含。
- 如果报错, 在 `main.c` 顶部确认有 `#include "stm32f10x.h"`。

### 6.4 画面方向反了

打开 `oled.c`, 改两个命令:
- 上下颠倒: `0xC8` 改成 `0xC0`
- 左右镜像: `0xA1` 改成 `0xA0`

### 6.5 编译报错 "unknown type name 'uint32_t'"

- 在对应文件顶部加 `#include <stdint.h>`。
- 或者确认 `stm32f10x.h` 已包含。

## 7. 下一步扩展

### 7.1 加 JDY-31 蓝牙模块

**不需要改 STM32 程序!** 把 JDY-31 的 TX/RX 接到 PA10/PA9, 手机用蓝牙串口助手连接后, 直接发送 `HAPPY` 即可。

### 7.2 加 DHT11 温湿度传感器

建议新增 `dht11.c/h`:
- 用一个 GPIO 单总线读取 DHT11
- 读到温度后, 用 `OLED_ShowNum()` 显示到屏幕上
- 也可以定义一个串口命令 `TEMP?`, 板子把温度通过串口发回

### 7.3 做 Python 上位机

用 `pyserial` 打开串口, 发送命令即可:

```python
import serial
ser = serial.Serial('COM3', 115200)
ser.write(b'HAPPY\r\n')
```

## 8. 文件编码

本目录下的 `.c/.h` 文件均使用 UTF-8 BOM 编码, 以便在 Keil 中正常显示中文注释。