/* main_example.c —— 怎么跟你现有的 CubeMX 工程拼起来
 *
 * 你已经有能跑通的工程了, 所以这个文件不是让你整个替换,
 * 而是"对照着改 4 个地方", 每个地方下面都有注释说明。
 *
 * 假设你现在用 USART1(PA9=TX, PA10=RX), 波特率 115200。
 * 如果你用的是别的串口, 把 huart1 换成你自己的句柄即可。
 *
 * 改的地方:
 *   ① 全局区: 加一个"串口中断收字节的中转变量"
 *   ② main() 开头: 初始化 OLED、宠物, 并启动串口中断接收
 *   ③ while(1) 循环里: 加两条调用
 *   ④ 文件后面: 加一个"串口接收完成"的回调函数
 */
#include "main.h"        /* CubeMX 生成 */
#include "oled.h"
#include "pet.h"
#include "uart_cmd.h"

/* ---------- ① 全局区: 加这个变量 ----------
 * 串口中断一次只收 1 个字节, 先存在这里, 再转交给 UartCmd_OnByte() */
static volatile uint8_t rx_byte = 0;

/* ---------- ② main() 开头(在你要用的外设初始化完之后) ----------
 * 示意如下, 请对照你自己的 main() 修改:

int main(void)
{
    HAL_Init();
    SystemClock_Config();        // 这些是你 CubeMX 生成的, 别动
    MX_GPIO_Init();
    MX_USART1_UART_Init();

    OLED_Init();                 // 屏幕初始化(如果屏幕已经点亮, 可跳过)
    Pet_Init();                  // 宠物开机第一帧

    // 启动串口中断接收: 收满 1 个字节就进回调。
    // 注意: 这条一定要放在 while 之前, 只调用一次
    HAL_UART_Receive_IT(&huart1, (uint8_t *)&rx_byte, 1);

    while (1)
    {
        // ---------- ③ 主循环里加这两行 ----------
        UartCmd_Poll();          // 解析串口指令(攒够一行就执行)
        Pet_Update();            // 宠物自动换帧(待机动画)
    }
}
*/

/* ---------- ④ 串口"收满 1 个字节"回调 ----------
 * CubeMX 工程里没有这个函数的话, 直接加在 main.c 里即可;
 * 如果你的工程里已经有同名回调(比如之前就用了串口中断), 把里面的内容合进去。
 *
 * 最容易踩的坑: 回调里处理完, 必须重新调用一次 HAL_UART_Receive_IT 挂起
 * 下一次接收, 否则串口收 1 个字节之后就再也不进中断了! */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        UartCmd_OnByte(rx_byte);                    /* 字节交给指令框架排队 */
        HAL_UART_Receive_IT(&huart1, (uint8_t *)&rx_byte, 1);   /* 重新挂起接收! */
    }
}

/*
 * 补充: 如果你还没配置 USART1, CubeMX 里这样配(它帮你生成代码):
 *   - USART1 Mode 选 Asynchronous
 *   - 波特率 115200, 8 位, 无校验, 1 停止位(默认就行)
 *   - NVIC 设置里勾选 USART1 global interrupt(这一步别忘!)
 *   然后重新生成代码。
 *
 * 想用 printf 往串口打印调试信息的话, 在工程选项里勾选 MicroLIB,
 * 再在任意 .c 文件里加:
 *   int fputc(int ch, FILE *f){ HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 100); return ch; }
 * 之后就可以 printf("x=%d\r\n", x); 了。
 */