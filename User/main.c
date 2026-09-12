/*
包含oled.h/pet.h/uart_cmd.h/usart1.h/delay.h
系统初始化后调用: delay_init()、OLED_Init()、Pet_Init()、USART1_Init(115200)
while(1)循环里调用UartCmd_Poll()和Pet_Update()
*/
#include "stm32f10x.h"
#include "delay.h"
#include "oled.h"
#include "pet.h"
#include "uart_cmd.h"
#include "usart1.h"

int main(void)
{
    //启动SysTick 1ms滴答
    delay_init();

    //初始化OLED、串口
    OLED_Init();
    Pet_Init();
    USART1_Init(115200);

    while (1)
    {
        //反复处理串口指令,更新动画
        UartCmd_Poll();
        Pet_Update();
    }
}