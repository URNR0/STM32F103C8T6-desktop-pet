/*
包含oled.h/pet.h/uart_cmd.h/usart1.h/delay.h/button.h
系统初始化后调用: delay_init()、OLED_Init()、Pet_Init()、USART1_Init(115200)、Button_Init()
while(1)循环里调用UartCmd_Poll()、Button_Scan()、Pet_Update()
*/
#include "stm32f10x.h"
#include "delay.h"
#include "oled.h"
#include "pet.h"
#include "uart_cmd.h"
#include "usart1.h"
#include "button.h"

int main(void)
{
    /*启动SysTick 1ms滴答*/
    delay_init();

    /*初始化OLED串口、按键 */
    OLED_Init();
    Pet_Init();
    USART1_Init(115200);
    Button_Init();

    while (1)
    {
        /*
        UartCmd_Poll();
        按键扫描(用杜邦线碰GND触发)
            PA0: 开心
            PA1: 睡觉/叫醒
            PA2: 委屈
			  */
        switch (Button_Scan()) {
        case BTN_1:
            Pet_SetFace(PET_FACE_HAPPY);
            break;
        case BTN_2:
            if (Pet_GetFace() == PET_FACE_SLEEP) {
                Pet_SetFace(PET_FACE_NORMAL);   
            } else {
                Pet_SetFace(PET_FACE_SLEEP);    
            }
            break;
        case BTN_3:
            Pet_SetFace(PET_FACE_SAD);
            break;
        default:
            break;
        }

        /*状态机换帧*/
        Pet_Update();
    }
}