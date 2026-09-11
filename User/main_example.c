/* main_example.c —— 标准库工程最小改动示例
 *
 * 如果你已经有自己的标准库 main.c, 只需要对照以下 3 步修改:
 *   1. 文件顶部包含头文件
 *   2. main() 里系统初始化后, 加入我们的初始化函数
 *   3. while(1) 主循环里加入 UartCmd_Poll() 和 Pet_Update()
 */
#include "stm32f10x.h"
#include "delay.h"
#include "oled.h"
#include "pet.h"
#include "uart_cmd.h"
#include "usart1.h"

int main(void)
{
    /* ---------- 1) 你的标准库初始化(如果已有, 保留) ----------
     * 例如: RCC_Configuration(); GPIO_Configuration(); ...
     */

    /* ---------- 2) 加上宠物相关初始化 ---------- */
    delay_init();              /* 启动 1ms SysTick 滴答(必须先调) */
    OLED_Init();               /* 屏幕初始化 */
    Pet_Init();                /* 宠物开机第一帧 */
    USART1_Init(115200);       /* 串口 115200, 开中断接收 */

    while (1)
    {
        /* ---------- 3) 主循环里加入这两行 ---------- */
        UartCmd_Poll();        /* 解析串口指令(攒够一行就执行) */
        Pet_Update();          /* 宠物自动换帧(待机动画) */
    }
}

/*
 * 提示:
 *   - 串口中断服务函数 USART1_IRQHandler 已经写在 usart1.c 里,
 *     不需要也不应该在 stm32f10x_it.c 里重复写, 否则会报重复定义。
 *   - 如果你的启动文件不是 startup_stm32f10x_md.s, 请确认里面
 *     有 USART1_IRQHandler 这个中断向量名。
 *   - 如果想用 printf, 勾选 Keil MicroLIB 后, 在 usart1.c 里加:
 *       int fputc(int ch, FILE *f) {
 *           if (ch == '\n') USART1_SendByte('\r');
 *           USART1_SendByte((uint8_t)ch);
 *           return ch;
 *       }
 *     并包含 <stdio.h> 即可。
 */