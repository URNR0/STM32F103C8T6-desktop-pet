/* oled.h —— SSD1306 OLED 驱动库的头文件(标准库版)
 *
 * 使用方法:
 *   1. 把 oled.c / oled.h / font8x8.h 放进你的 Keil 工程
 *   2. 上电后调用一次 OLED_Init()
 *   3. 先往"缓冲区"里画东西, 最后调用 OLED_Refresh() 整屏显示
 *
 * 所有画图都是先画进内存里的缓冲区(1KB), 不会直接操作屏幕,
 * 所以画面不会闪烁; 画完记得 Refresh, 否则屏幕上没变化。
 */
#ifndef __OLED_H
#define __OLED_H

#include "stm32f10x.h"
#include <stdint.h>

/* ===================== 引脚配置(改这里即可换引脚) =====================
 * 你的接线: SCL -> PB6, SDA -> PB7
 * 软件 I2C = 用两个普通 GPIO 模拟 I2C 时序, 不占用片上 I2C 外设
 */
#define OLED_SCL_PORT   GPIOB
#define OLED_SCL_PIN    GPIO_Pin_6
#define OLED_SDA_PORT   GPIOB
#define OLED_SDA_PIN    GPIO_Pin_7

/* SSD1306 的 I2C 地址: 绝大多数 0.96 寸模块是 0x3C
 * 屏幕一直不亮时, 试试改成 0x3D(部分模块背面有电阻可换地址) */
#define OLED_I2C_ADDR   0x3C

/* ===================== 对外接口 ===================== */
void  OLED_Init(void);                          /* 上电初始化(只调用一次) */
void  OLED_Clear(void);                         /* 清空缓冲区(黑屏) */
void  OLED_Refresh(void);                       /* 把缓冲区整屏推到屏幕 */
void  OLED_Fill(void);                          /* 整屏点亮(测试屏幕用) */

void  OLED_DrawPixel(uint8_t x, uint8_t y, uint8_t on);   /* on=1 亮, 0 灭 */
void  OLED_DrawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t on);
void  OLED_DrawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t on);
void  OLED_FillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t on);

/* 显示一整屏图(用于动画, 图片格式见 pet_frames.h 说明); 128x64, 共 1024 字节 */
void  OLED_ShowFrame(const uint8_t *frame);

/* 在 (x,y) 位置显示图的一角; 图片按"列优先, 每字节=纵向8个点"编码 */
void  OLED_ShowBMP(uint8_t x, uint8_t y, uint8_t w, uint8_t h, const uint8_t *data);

/* 文字: 内置 8x8 小字体(见 font8x8.h)。y 建议取 8 的倍数(0/8/16/...56) */
void  OLED_ShowChar(uint8_t x, uint8_t y, uint8_t ch);
void  OLED_ShowString(uint8_t x, uint8_t y, const char *s);
void  OLED_ShowNum(uint8_t x, uint8_t y, long num, uint8_t len); /* 整数, 右对齐 len 位 */

#endif /* __OLED_H */