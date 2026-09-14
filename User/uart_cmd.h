/* uart_cmd.h —— 串口指令接收与解析框架
 *
 * 整条链路(用大白话说):
 *
 *   [手机/电脑/蓝牙] 发来一行文字, 例如:  HAPPY 回车
 *                              |
 *                              v
 *   [串口中断] 一个字节一个字节地收, 每收一个就丢给 UartCmd_OnByte()
 *                              |
 *                              v
 *   [环形缓冲区] 暂时存起来(几毫秒内可能连来好多个字节, 先排队)
 *                              |
 *                              v
 *   [主循环 UartCmd_Poll()] 攒够一整行(见到回车)就解析、执行
 *
 * 协议约定(你自己定的, 够简单够好调):
 *   命令是"一行文字", 以回车(\r 或 \n)结尾。
 *   例: NORMAL / BLINK / HAPPY / PING / HELP
 *   以后加"带参数"的命令也没问题, 例如  TEMP?  /  FACE:2, 下面代码里有示例位置。
 */
#ifndef __UART_CMD_H
#define __UART_CMD_H

#include <stdint.h>

/* 固件版本号(VER 命令和开机握手都会报它) */
#define PET_FW_VERSION "1.0"

/* 串口中断回调里调用: 收到一个字节就传进来(内部会排队) */
void UartCmd_OnByte(uint8_t b);

/* 主循环里反复调用: 攒够一整行就执行对应的命令 */
void UartCmd_Poll(void);

/* 初始化(注册宠物状态上报回调), 在 main 里串口初始化后调用一次 */
void UartCmd_Init(void);

/* 给测试用: 直接模拟收到一条命令(等价于串口发来 "xxx\r\n") */
void UartCmd_Send(const char *line);

#endif /* __UART_CMD_H */