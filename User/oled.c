/* oled.c —— SSD1306 OLED 驱动(软件 I2C 版, 标准库)
 *
 *   OLED原理
 *   屏幕里有一块 1KB 的"显存"; 我们程序里也留了一块一模一样的 1KB 缓冲区。
 *   你画画、写字都先在程序里的缓冲区改, 改完一次性把 1KB 拷贝/发送到屏幕,
 *   画面就整屏更新了
 *   软件 I2C 的写法: 两个 GPIO 按 I2C 时序手动拉高拉低,
 *   SDA 线在发送完 8 位后会释放(开漏输出), 看设备有没有拉低应答。
 *   注意: 开漏输出需要上拉电阻, 屏幕模块通常自带(板上 4.7k);
 *         如果没有, 在 SDA、SCL 上各接一个 4.7k 电阻到 3.3V。
 */
#include "oled.h"
#include "font8x8.h"
#include "string.h"

/* ---------- 显存缓冲区: 8 页 x 128 列 ----------
 * OLED_GRAM[页][列], 每个字节里 bit0 是这一页最上面那行像素
 */
static uint8_t OLED_GRAM[8][128];

#define PAGE_OF(y)  ((y) >> 3)    /* 像素 y 在哪一页(0~7) */
#define BIT_OF(y)   ((y) & 0x07)  /* 像素 y 在页内第几行(0=最上) */

/* SSD1306 初始化命令序列(顺序基本固定, 别乱改)
 * 常见问题: 画面上下颠倒 -> 0xC8 改成 0xC0
 *          左右镜像     -> 0xA1 改成 0xA0 */
static const uint8_t s_init_cmds[] = {
    0xAE,        /* 关显示 */
    0x20, 0x02,  /* 页寻址模式 */
    0xC8,        /* COM 扫描方向(反扫, 让画面上下正常) */
    0x40,        /* 显示起始行 = 0 */
    0x81, 0x7F,  /* 对比度 */
    0xA1,        /* 段重映射(修正左右) */
    0xA6,        /* 正常显示(白点亮) */
    0xA8, 0x3F,  /* 多路复用比: 1/64 (128x64) */
    0xA4,        /* 恢复显示 RAM 内容 */
    0xD3, 0x00,  /* 显示偏移 = 0 */
    0xD5, 0x80,  /* 时钟分频 */
    0xD9, 0xF1,  /* 预充电周期 */
    0xDA, 0x12,  /* COM 引脚配置(64 行用 0x12) */
    0xDB, 0x40,  /* VCOMH 电平 */
    0x8D, 0x14,  /* 开电荷泵 —— 不开它屏幕必黑, 排障第一检查项! */
    0xAF         /* 开显示 */
};

/* ============================================================
 * 软件 I2C 底层
 * ============================================================ */
static void I2C_SCL(uint8_t v) { v ? GPIO_SetBits(OLED_SCL_PORT, OLED_SCL_PIN) : GPIO_ResetBits(OLED_SCL_PORT, OLED_SCL_PIN); }
static void I2C_SDA(uint8_t v) { v ? GPIO_SetBits(OLED_SDA_PORT, OLED_SDA_PIN) : GPIO_ResetBits(OLED_SDA_PORT, OLED_SDA_PIN); }

/* 微秒延时: 简单的软件循环(NOP 忙等待), 不用 DWT/SysTick, 兼容所有标准库工程。
 * 注意: 这是"大约"延时, 受编译优化和主频影响;
 *       对 SSD1306 的 I2C 时序, 延时"偏长没关系、偏短才出错",
 *       所以这里系数偏保守, 保证各种主频下都够用。 */
static void I2C_DelayUs(uint32_t us)
{
    uint32_t i;
    uint32_t loops = us * (SystemCoreClock / 4000000UL);  /* 72MHz 下约 18 次循环 ≈ 1us */
    for (i = 0; i < loops; i++) {
        __NOP();
    }
}

/* I2C 起始信号: SCL 高电平时把 SDA 拉低 */
static void I2C_Start(void)
{
    I2C_SDA(1); I2C_SCL(1); I2C_DelayUs(2);
    I2C_SDA(0);             I2C_DelayUs(2);
    I2C_SCL(0);             I2C_DelayUs(2);
}

/* I2C 停止信号: SCL 高电平时把 SDA 拉高 */
static void I2C_Stop(void)
{
    I2C_SDA(0); I2C_SCL(1); I2C_DelayUs(2);
    I2C_SDA(1);             I2C_DelayUs(2);
}

/* 发送一个字节(高位先发), 返回 1 = 设备没应答(说明地址不对或没接好) */
static uint8_t I2C_SendByte(uint8_t dat)
{
    uint8_t i;
    uint8_t nack;
    for (i = 0; i < 8; i++) {
        I2C_SDA((dat >> (7 - i)) & 1); I2C_DelayUs(1);
        I2C_SCL(1);                     I2C_DelayUs(2);
        I2C_SCL(0);                     I2C_DelayUs(1);
    }
    /* 读应答: 释放 SDA(开漏输出写1=让线上拉), 拉高 SCL 后读引脚 */
    I2C_SDA(1); I2C_DelayUs(1);
    I2C_SCL(1); I2C_DelayUs(2);
    nack = (GPIO_ReadInputDataBit(OLED_SDA_PORT, OLED_SDA_PIN) == (uint8_t)Bit_SET);
    I2C_SCL(0); I2C_DelayUs(1);
    return nack;
}

/* 给屏幕写一个"命令"字节(控制字节 0x00) */
static void OLED_WriteCmd(uint8_t cmd)
{
    I2C_Start();
    I2C_SendByte(OLED_I2C_ADDR << 1);   /* 器件地址 + 写方向 */
    I2C_SendByte(0x00);                 /* Co=0, D/C#=0 -> 命令 */
    I2C_SendByte(cmd);
    I2C_Stop();
}

/* 给屏幕写一个"数据"字节(控制字节 0x40) */
static void OLED_WriteData(uint8_t dat)
{
    I2C_Start();
    I2C_SendByte(OLED_I2C_ADDR << 1);
    I2C_SendByte(0x40);                 /* D/C#=1 -> 数据 */
    I2C_SendByte(dat);
    I2C_Stop();
}

/* ============================================================
 * 初始化
 * ============================================================ */
void OLED_Init(void)
{
    GPIO_InitTypeDef gpio;
    uint16_t i;

    /* 1) 配置 I2C 引脚: 开漏输出 + 内部上拉。
     *    开漏输出在输出 1 时不主动拉高, 靠上拉电阻把线拉高,
     *    这样才能"读"到设备拉低应答的信号 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    gpio.GPIO_Pin   = OLED_SCL_PIN | OLED_SDA_PIN;
    gpio.GPIO_Mode  = GPIO_Mode_Out_OD;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(OLED_SCL_PORT, &gpio);

    /* 2) 等芯片上电稳定再初始化 */
    I2C_DelayUs(50000);

    /* 3) 按序列发送初始化命令 */
    for (i = 0; i < sizeof(s_init_cmds); i++) {
        OLED_WriteCmd(s_init_cmds[i]);
    }

    OLED_Clear();
    OLED_Refresh();
}

/* ============================================================
 * 缓冲区和基本图形
 * ============================================================ */
void OLED_Clear(void)
{
    memset(OLED_GRAM, 0, sizeof(OLED_GRAM));
}

void OLED_Fill(void)
{
    memset(OLED_GRAM, 0xFF, sizeof(OLED_GRAM));
}

void OLED_DrawPixel(uint8_t x, uint8_t y, uint8_t on)
{
    if (x >= 128 || y >= 64) return;
    if (on) OLED_GRAM[PAGE_OF(y)][x] |=  (1u << BIT_OF(y));
    else    OLED_GRAM[PAGE_OF(y)][x] &= ~(1u << BIT_OF(y));
}

/* 画线(Bresenham 算法, 整数运算, 不占多少 Flash) */
void OLED_DrawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t on)
{
    int16_t dx, dy, sx, sy, err, e2;
    dx = (int16_t)x1 - x0;
    dy = (int16_t)y1 - y0;
    sx = (dx > 0) ? 1 : -1;
    sy = (dy > 0) ? 1 : -1;
    dx = (dx > 0) ? dx : -dx;
    dy = (dy > 0) ? dy : -dy;
    err = dx - dy;
    for (;;) {
        OLED_DrawPixel(x0, y0, on);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 <  dx) { err += dx; y0 += sy; }
    }
}

void OLED_DrawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t on)
{
    OLED_DrawLine(x,     y,     x + w - 1, y,         on);
    OLED_DrawLine(x,     y + h - 1, x + w - 1, y + h - 1, on);
    OLED_DrawLine(x,     y,     x,         y + h - 1, on);
    OLED_DrawLine(x + w - 1, y, x + w - 1, y + h - 1, on);
}

void OLED_FillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t on)
{
    uint8_t i, j;
    for (j = 0; j < h; j++) {
        for (i = 0; i < w; i++) {
            OLED_DrawPixel(x + i, y + j, on);
        }
    }
}

/* ============================================================
 * 整屏刷新(把 1KB 缓冲区推到屏幕)
 * 页寻址模式下, 一条数据事务可以连续发 128 个字节, 不用反复 Start
 * ============================================================ */
void OLED_Refresh(void)
{
    uint8_t page, x;
    for (page = 0; page < 8; page++) {
        OLED_WriteCmd(0xB0 + page);   /* 选页 */
        OLED_WriteCmd(0x00);          /* 列地址低 4 位 = 0 */
        OLED_WriteCmd(0x10);          /* 列地址高 4 位 = 0 */
        I2C_Start();
        I2C_SendByte(OLED_I2C_ADDR << 1);
        I2C_SendByte(0x40);
        for (x = 0; x < 128; x++) {
            I2C_SendByte(OLED_GRAM[page][x]);   /* 连续发数据, 屏幕自动加列地址 */
        }
        I2C_Stop();
    }
}

/* ============================================================
 * 图片
 * ============================================================ */
/* 显示一整屏动画帧: 帧数据是"列优先"(frame[列*8+页]),
 * 而显存是"页优先"(OLED_GRAM[页][列]), 两者排列不同, 必须转置拷贝, 不能 memcpy */
void OLED_ShowFrame(const uint8_t *frame)
{
    uint8_t col, page;
    for (col = 0; col < 128; col++) {
        for (page = 0; page < 8; page++) {
            OLED_GRAM[page][col] = frame[col * 8 + page];
        }
    }
}

/* 显示图片的一角。
 * 图片编码约定(和 pet_frames.h 一致):
 *   按列存放; 每字节 = 这一列里纵向连续的 8 个点; 字节内 bit0 = 最上面那个点
 *   数据排列: 先第 0 列的所有页, 再第 1 列的所有页...
 *   建议把 y 放在 8 的倍数上(0/8/16/...), 这样最简单不串行 */
void OLED_ShowBMP(uint8_t x, uint8_t y, uint8_t w, uint8_t h, const uint8_t *data)
{
    uint8_t pages = (h + 7) / 8;
    uint8_t col, p;
    for (col = 0; col < w; col++) {
        for (p = 0; p < pages; p++) {
            uint8_t dat = data[col * pages + p];
            uint8_t pageY = PAGE_OF(y) + p;
            uint8_t shift = BIT_OF(y);
            if (pageY < 8 && x + col < 128) {
                if (shift == 0) {
                    OLED_GRAM[pageY][x + col] = dat;          /* y 在 8 的倍数上: 直接覆盖 */
                } else {
                    OLED_GRAM[pageY][x + col] |= (uint8_t)(dat << shift);
                    if (pageY + 1 < 8) {
                        OLED_GRAM[pageY + 1][x + col] |= (uint8_t)(dat >> (8 - shift));
                    }
                }
            }
        }
    }
}

/* ============================================================
 * 文字(内置 8x8 ASCII 字库)
 * ============================================================ */
/* 显示一个 8x8 字符。y 建议为 8 的倍数。
 * 原理: 字库里每个字符保存 8 个字节(每字节是一行, bit7=最左),
 *       这里把它"竖过来"转成显存需要的格式再写入 */
void OLED_ShowChar(uint8_t x, uint8_t y, uint8_t ch)
{
    const uint8_t *rows = font8x8[ch & 0x7F];
    uint8_t page = PAGE_OF(y);
    uint8_t c, r;
    if (x > 120 || page > 7) return;
    for (c = 0; c < 8; c++) {
        uint8_t colData = 0;
        for (r = 0; r < 8; r++) {
            if (rows[r] & (0x80 >> c)) {
                colData |= (uint8_t)(1u << r);   /* 第 r 行 -> 字节里第 r 位(0=最上) */
            }
        }
        if (x + c < 128) {
            OLED_GRAM[page][x + c] = colData;
        }
    }
}

void OLED_ShowString(uint8_t x, uint8_t y, const char *s)
{
    while (*s) {
        OLED_ShowChar(x, y, (uint8_t)*s);
        x += 8;                 /* 8x8 字体, 每个字占 8 列 */
        if (x > 120) break;     /* 超出屏幕就停 */
        s++;
    }
}

/* 显示一个整数, 靠右对齐 len 位(不足前面补 0, 像数码管) */
void OLED_ShowNum(uint8_t x, uint8_t y, long num, uint8_t len)
{
    uint8_t i;
    if (len > 8) len = 8;
    for (i = 0; i < len; i++) {
        uint8_t ch = (uint8_t)('0' + (num % 10));
        OLED_ShowChar(x + 8 * (len - 1 - i), y, ch);   /* 从右往左填 */
        num /= 10;
    }
}