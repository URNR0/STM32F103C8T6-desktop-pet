/* usart1.h —— 标准库串口1初始化 + 中断接收
 *
 * 默认使用 PA9(TX)、PA10(RX), 波特率 115200。
 * 收到的每个字节都会通过 UartCmd_OnByte() 交给指令框架处理。
 */
#ifndef __USART1_H
#define __USART1_H

#include "stm32f10x.h"

void USART1_Init(uint32_t baudrate);

/* 阻塞式发送一个字节/一串字符, 主要用于调试回复 */
void USART1_SendByte(uint8_t b);
void USART1_SendString(const char *s);

#endif /* __USART1_H */