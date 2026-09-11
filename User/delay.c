/* delay.c —— 标准库下的简单毫秒延时与系统滴答
 *
 * 实现原理:
 *   启动 SysTick 每 1ms 中断一次, 中断里 tick 加 1。
 *   这样就有了一个全局的毫秒计数, 和 HAL 库的 HAL_GetTick() 一样好用。
 *
 * 注意:
 *   不要和 stm32f10x_it.c 里自带的 SysTick_Handler 同时写两份;
 *   如果工程里已有 SysTick 中断, 可以把 tick++ 那行合进去。
 */
#include "delay.h"

static volatile uint32_t g_tick = 0;

uint32_t SysTick_GetTick(void)
{
    return g_tick;
}

void delay_ms(uint32_t ms)
{
    uint32_t start = g_tick;
    while ((uint32_t)(g_tick - start) < ms) { }
}

/* SysTick 中断: 每毫秒进来一次 */
void SysTick_Handler(void)
{
    g_tick++;
}

void delay_init(void)
{
    /* 系统时钟 72MHz 时, 1ms = 72000 个 tick */
    if (SysTick_Config(SystemCoreClock / 1000)) {
        while (1);  /* 配置失败, 死在这 */
    }
}
