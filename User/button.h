/* button.h —— 按键交互模块(3 个按键, 用杜邦线碰 GND 模拟按下)
 *
 * 接线(全部共享一个 GND):
 *   按键1: PA0 ──碰──> GND   作用: 逗它开心(也会叫醒睡着的宠物)
 *   按键2: PA1 ──碰──> GND   作用: 睡觉/叫醒(来回切换)
 *   按键3: PA2 ──碰──> GND   作用: 让它委屈(也会叫醒睡着的宠物)
 *
 * 原理:
 *   GPIO 配成"内部上拉输入", 不碰线时读到 1(高), 碰 GND 时读到 0(低)。
 *   每 20ms 采样一次, 连续两次相同才确认(去抖), 防止抖动误触发。
 *
 * 没有真按键? 用杜邦线就行:
 *   一根线插 PA0, 另一端去碰 GND 排针 = 按下按键1, 松开 = 松手。
 */
#ifndef __BUTTON_H
#define __BUTTON_H

#include "stm32f10x.h"

/* 按键事件 */
typedef enum {
    BTN_NONE = 0,   /* 无事件(最常见, 主循环里大部分时候都是这个) */
    BTN_1,          /* PA0 被按下: 开心 */
    BTN_2,          /* PA1 被按下: 睡觉/叫醒 */
    BTN_3,          /* PA2 被按下: 委屈 */
} ButtonEvent_t;

void Button_Init(void);           /* 初始化 3 个按键的 GPIO */
ButtonEvent_t Button_Scan(void);  /* 主循环调用, 返回事件(没按下就返回 BTN_NONE ) */

#endif /* __BUTTON_H */