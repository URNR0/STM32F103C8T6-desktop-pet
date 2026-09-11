/* main.c —— 桌面宠物标准库启动文件示例
 *
 * 如果你已经有标准库工程, 只需要在自己的 main.c 里做三件事:
 *   1. 包含 oled.h / pet.h / uart_cmd.h / usart1.h / delay.h
 *   2. 系统初始化后调用: delay_init()、OLED_Init()、Pet_Init()、USART1_Init(115200)
 *   3. while(1) 循环里调用 UartCmd_Poll() 和 Pet_Update()
 */
#include "stm32f10x.h"
#include "delay.h"
#include "oled.h"
#include "pet.h"
#include "uart_cmd.h"
#include "usart1.h"

int main(void)
{
    /* 1) 启动 SysTick 1ms 滴答(标准库没有 HAL_GetTick, 自己做一个) */
    delay_init();

    /* 2) 初始化 OLED、宠物、串口 */
    OLED_Init();
    Pet_Init();
    USART1_Init(115200);

    while (1)
    {
        /* 3) 反复处理串口指令 + 更新宠物动画 */
        UartCmd_Poll();
        Pet_Update();
    }
}