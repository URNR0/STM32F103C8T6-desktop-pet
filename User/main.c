/*
包含 oled.h/pet.h/uart_cmd.h/usart1.h/delay.h/button.h
系统初始化后调用: delay_init()、OLED_Init()、Pet_Init()、USART1_Init(9600)、Button_Init()
while(1) 循环里调用 UartCmd_Poll()、Button_Scan()、Pet_Update()

蓝牙模块说明:
    TXD接PA10(USART1_RX),RXD接PA9(USART1_TX),GND接GND
    波特率9600
    蓝牙只是"无线透传",手机发来的文字原样进USART1,和之前USB转串口走同一条路
*/
#include "stm32f10x.h"
#include "delay.h"
#include "oled.h"
#include "pet.h"
#include "uart_cmd.h"
#include "usart1.h"
#include "button.h"
#include "mood.h"   /* 情绪模型 */

int main(void)
{
    /* 启动 SysTick 1ms 滴答 */
    delay_init();

    /* 初始化 OLED、串口(蓝牙)、按键 */
    OLED_Init();
    Pet_Init();
    USART1_Init(9600);   /* 蓝牙透传模块默认 9600, 手机端也要设成 9600 */
    Button_Init();
    UartCmd_Init();      /* 注册宠物状态上报回调 */
    Mood_Init();         /* 初始化心情/饥饿/亲密度 */

    /* 开机握手: 上电发一条上线消息(蓝牙未连接时会丢失, 不影响后续) */
    USART1_SendString("READY: desktop-pet v" PET_FW_VERSION "\r\n");

    while (1)
    {
        /* 1) 处理串口/蓝牙发来的命令(手机 App 发 PING 应回 PONG) */
        UartCmd_Poll();

        /* 2) 按键扫描(用杜邦线碰 GND 触发)
         *    PA0: 摸头/互动(提心情)  PA1: 睡觉/叫醒  PA2: 委屈
         */
        switch (Button_Scan()) {
        case BTN_1:
            Mood_Play();              /* 互动: 提心情 + 开心 */
            break;
        case BTN_2:
            if (Pet_GetFace() == PET_FACE_SLEEP) {
                Pet_SetFace(PET_FACE_NORMAL);   /* 叫醒 */
            } else {
                Pet_SetFace(PET_FACE_SLEEP);    /* 睡觉 */
            }
            break;
        case BTN_3:
            Pet_SetFace(PET_FACE_SAD);
            break;
        default:
            break;
        }

        /* 3) 更新宠物动画(状态机换帧) */
        Pet_Update();

        /* 4) 情绪模型: 每秒更新情绪值, 心情过低会自动委屈 */
        Mood_Tick();
    }
}
