/* delay.h —— 标准库下的简单毫秒延时与系统滴答
 *
 * 标准库不像 HAL 自带 HAL_GetTick(), 这里自己做一个:
 *   delay_init()      放在 main 开头调用一次, 启动 SysTick 1ms 中断
 *   SysTick_GetTick() 返回从开机到现在经过的毫秒数
 *   delay_ms(x)       阻塞延时 x 毫秒
 */
#ifndef __DELAY_H
#define __DELAY_H

#include "stm32f10x.h"

void delay_init(void);
void delay_ms(uint32_t ms);
uint32_t SysTick_GetTick(void);

#endif /* __DELAY_H */