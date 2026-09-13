/* uart_cmd.c —— 串口指令接收与解析(环形缓冲区 + 行解析 + 命令表, 标准库版)
 *
 *
 *   1. 环形缓冲区: 中断往里塞, 主循环往外取 —— 两者互不干扰
 *   2. 行解析器:   从缓冲区取出字节, 看见回车就算一条完整命令
 *   3. 命令表:     查表执行, 以后加命令只需要在表里加一行
 *
 *   回复通过 usart1.h 里的 USART1_SendString() 发送。
 */
#include "stm32f10x.h"
#include "string.h"
#include "uart_cmd.h"
#include "pet.h"
#include "usart1.h"          /* 提供 USART1_SendString */

/* ============ 1. 环形缓冲区 ============ */
#define RX_RING_SIZE  128          /* 缓冲大小(字节), 够用就行 */
static volatile uint8_t  rx_ring[RX_RING_SIZE];
static volatile uint16_t rx_head = 0;   /* 写指针: 中断里写 */
static volatile uint16_t rx_tail = 0;   /* 读指针: 主循环里读 */

/* 中断里调用: 塞一个字节。缓冲区满了就直接丢(最简单的处理) */
void UartCmd_OnByte(uint8_t b)
{
    uint16_t next = (uint16_t)((rx_head + 1) % RX_RING_SIZE);
    if (next == rx_tail) return;        /* 满了, 丢掉这个字节 */
    rx_ring[rx_head] = b;
    rx_head = next;
}

/* 主循环里调用: 取出一个字节, 没有就返回 0 */
static uint8_t Ring_Pop(uint8_t *out)
{
    if (rx_tail == rx_head) return 0;
    *out = rx_ring[rx_tail];
    rx_tail = (uint16_t)((rx_tail + 1) % RX_RING_SIZE);
    return 1;
}

/* ============ 2. 行解析 ============ */
#define CMD_BUF_SIZE 32                /* 一条命令最长 31 个字符(含终结符) */
static uint8_t  cmd_buf[CMD_BUF_SIZE];
static uint8_t  cmd_len = 0;

/* ============ 3. 命令表(加新命令在这加一行) ============ */

/* 回一句给上位机/手机, 方便调试 */
static void Reply(const char *s)
{
    USART1_SendString(s);
}

static void Cmd_Normal(void) { Pet_SetFace(PET_FACE_NORMAL); Reply("OK:NORMAL\r\n"); }
static void Cmd_Blink(void)  { Pet_SetFace(PET_FACE_BLINK);  Reply("OK:BLINK\r\n"); }
static void Cmd_Happy(void)  { Pet_SetFace(PET_FACE_HAPPY);  Reply("OK:HAPPY\r\n"); }
static void Cmd_Sleep(void)  { Pet_SetFace(PET_FACE_SLEEP);  Reply("OK:SLEEP\r\n"); }
static void Cmd_Sad(void)    { Pet_SetFace(PET_FACE_SAD);    Reply("OK:SAD\r\n"); }
static void Cmd_Ping(void)   { Reply("PONG\r\n"); }

static void Cmd_Help(void)
{
    Reply("CMDS: NORMAL BLINK HAPPY SLEEP SAD PING HELP\r\n");
}

/* 命令表: 字符串匹配(不区分大小写)。以后要带参数, 在这里面用 sscanf 或
 * strchr(cmd, ':') 解析即可, 例如 "TEMP" 命令可以再读后面跟的数字 */
typedef struct {
    const char *name;
    void (*handler)(void);
} CmdEntry_t;

static const CmdEntry_t cmd_table[] = {
    { "NORMAL", Cmd_Normal },
    { "BLINK",  Cmd_Blink },
    { "HAPPY",  Cmd_Happy },
    { "SLEEP",  Cmd_Sleep },
    { "SAD",    Cmd_Sad },
    { "PING",   Cmd_Ping },
    { "HELP",   Cmd_Help },
};
#define CMD_TABLE_LEN  (sizeof(cmd_table) / sizeof(cmd_table[0]))

/* 不区分大小写地比较两个字符串, 完全相等返回 1 */
static uint8_t CmdCmp(const char *a, const char *b)
{
    while (*a && *b) {
        char ca = *a;
        char cb = *b;
        if (ca >= 'a' && ca <= 'z') ca -= 32;
        if (cb >= 'a' && cb <= 'z') cb -= 32;
        if (ca != cb) return 0;
        a++;
        b++;
    }
    return (*a == *b);
}

/* 查命令表并执行 */
static void UartCmd_Dispatch(const char *cmd)
{
    uint16_t i;
    for (i = 0; i < CMD_TABLE_LEN; i++) {
        if (CmdCmp(cmd, cmd_table[i].name)) {
            cmd_table[i].handler();
            return;
        }
    }
    Reply("ERR:UNKNOWN\r\n");          /* 不认识的命令, 回一句方便排查 */
}

/* 主循环反复调用: 攒行 -> 执行 */
void UartCmd_Poll(void)
{
    uint8_t b;
    while (Ring_Pop(&b)) {
        if (b == '\n' || b == '\r') {      /* 一行结束 */
            if (cmd_len > 0) {
                cmd_buf[cmd_len] = '\0';
                UartCmd_Dispatch((const char *)cmd_buf);
                cmd_len = 0;
            }
            /* 收到的是空行(例如 \r\n 里的 \n): 直接忽略 */
        } else if (cmd_len < CMD_BUF_SIZE - 1) {
            cmd_buf[cmd_len++] = b;
        } else {
            cmd_len = 0;                   /* 超长: 整条作废重来, 防垃圾数据 */
        }
    }
}

/* 测试用: 手动喂一条命令, 等价于串口发来 "xxx\n" */
void UartCmd_Send(const char *line)
{
    while (*line) {
        UartCmd_OnByte((uint8_t)*line);
        line++;
    }
    UartCmd_OnByte('\n');
}