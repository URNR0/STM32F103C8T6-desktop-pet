/* usart1.c —— 标准库串口1初始化 + 中断接收
 *
 * 使用 PA9(TX)、PA10(RX)。
 * 中断里收到一个字节后, 调用 UartCmd_OnByte() 交给指令框架排队。
 */
#include "usart1.h"
#include "uart_cmd.h"

/* 串口中断服务函数: 名字必须和启动文件里的向量表一致 */
void USART1_IRQHandler(void)
{
    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
        uint8_t b = (uint8_t)USART_ReceiveData(USART1);
        UartCmd_OnByte(b);
        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    }
}

void USART1_SendByte(uint8_t b)
{
    USART_SendData(USART1, b);
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET) { }
}

void USART1_SendString(const char *s)
{
    while (*s) {
        USART1_SendByte((uint8_t)*s);
        s++;
    }
}

void USART1_Init(uint32_t baudrate)
{
    GPIO_InitTypeDef gpio;
    USART_InitTypeDef usart;

    /* 开时钟: GPIOA、USART1 都在 APB2 上 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);

    /* PA9 TX - 复用推挽输出 */
    gpio.GPIO_Pin = GPIO_Pin_9;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);

    /* PA10 RX - 浮空输入 */
    gpio.GPIO_Pin = GPIO_Pin_10;
    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &gpio);

    /* 串口参数: 115200 / 8 / 无校验 / 1停止位 / 收发使能 */
    usart.USART_BaudRate = baudrate;
    usart.USART_WordLength = USART_WordLength_8b;
    usart.USART_StopBits = USART_StopBits_1;
    usart.USART_Parity = USART_Parity_No;
    usart.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_Init(USART1, &usart);

    /* 开启接收中断 */
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

    /* 在 NVIC 里设置 USART1 中断优先级并启用 */
    NVIC_InitTypeDef nvic;
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    nvic.NVIC_IRQChannel = USART1_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 1;
    nvic.NVIC_IRQChannelSubPriority = 0;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);

    /* 使能串口 */
    USART_Cmd(USART1, ENABLE);
}