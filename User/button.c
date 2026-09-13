/* button.c —— 按键交互模块(3 按键, 去抖, 边沿触发)
 *
 * 去抖原理(通俗版):
 *   按键(或杜邦线)碰上去的瞬间, 触点会"弹跳"几下,
 *   CPU 跑得太快, 会以为你按了好几下。所以每 20ms 看一眼,
 *   连续两次看到一样的值才算数, 弹跳就被忽略了。
 *
 * 边沿触发:
 *   只有"从松开变成按下"那一刻才产生事件,
 *   按住不动不会反复触发; 必须松开后才能触发下一次。
 */
#include "button.h"
#include "delay.h"       /* SysTick_GetTick */

/* 三个按键分别用 PA0 / PA1 / PA2 */
#define BTN_COUNT 3
static const uint16_t btn_pin[BTN_COUNT] = { GPIO_Pin_0, GPIO_Pin_1, GPIO_Pin_2 };

/* 每个按键的去抖状态 */
static struct {
    uint8_t stable;   /* 去抖后的稳定电平: 1=松开, 0=按下 */
    uint8_t last;     /* 上一次采样的原始电平 */
    uint8_t fired;    /* 本次按下是否已经触发过(防按住不放反复触发) */
} btn[BTN_COUNT];

static uint32_t last_scan_ms = 0;
#define DEBOUNCE_MS  20   /* 去抖采样间隔: 20ms 一次 */

void Button_Init(void)
{
    GPIO_InitTypeDef gpio;
    uint8_t i;

    /* 开 GPIOA 时钟(USART1_Init 也开过, 重复开没副作用) */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    /* PA0 / PA1 / PA2 配成内部上拉输入: 不碰=高(1), 碰GND=低(0) */
    gpio.GPIO_Pin   = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2;
    gpio.GPIO_Mode  = GPIO_Mode_IPU;
    gpio.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOA, &gpio);

    /* 初始状态: 全部松开 */
    for (i = 0; i < BTN_COUNT; i++) {
        btn[i].stable = 1;
        btn[i].last   = 1;
        btn[i].fired  = 0;
    }
}

ButtonEvent_t Button_Scan(void)
{
    uint32_t now = SysTick_GetTick();
    uint8_t  i;

    /* 每 20ms 才采样一次, 中间的调用直接返回"无事件" */
    if ((uint32_t)(now - last_scan_ms) < DEBOUNCE_MS) {
        return BTN_NONE;
    }
    last_scan_ms = now;

    for (i = 0; i < BTN_COUNT; i++) {
        uint8_t raw = GPIO_ReadInputDataBit(GPIOA, btn_pin[i]);

        if (raw == btn[i].last) {
            /* 连续两次采样相同 → 值稳定了, 可以更新 */
            if (btn[i].stable != raw) {
                btn[i].stable = raw;

                if (raw == 0 && !btn[i].fired) {
                    /* 刚从松开变成按下 → 触发事件! */
                    btn[i].fired = 1;
                    return (ButtonEvent_t)(BTN_1 + i);
                }
                if (raw == 1) {
                    /* 刚从按下变成松开 → 允许下一次触发 */
                    btn[i].fired = 0;
                }
            }
        }
        btn[i].last = raw;   /* 记下本次采样, 供下次比较 */
    }

    return BTN_NONE;
}