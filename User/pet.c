/* pet.c  宠物动画: 表情 + 待机动画脚本
 *
 * 动画节奏改动:
 *   改下面 anim_idle[] 表里的时长(单位毫秒)和帧顺序,
 *   例: 想让宠物更活泼, 把开心帧的时长调大/调频繁。
 *
 * 形象改变:
 *   用 PCtoLCD2002 / Image2LCD 等工具把你画的 128x64 图转成数组
 *   (列行式、纵向8点一个字节、bit0在最上), 替换 pet_frames.h 里的数组。
 *   工程里附带 gen_assets.py 可以重新生成这套帧, 也可以直接改它画新形象。
 */
#include "pet.h"
#include "oled.h"
#include "pet_frames.h"
#include "delay.h"          /* 标准库下自己提供 SysTick_GetTick */

/* ---------- 动画脚本(帧列表) ---------- */
typedef struct {
    const uint8_t *frame;   /* 指向一帧图片(1024 字节) */
    uint16_t hold_ms;       /* 这一帧显示多长时间 */
} PetAnimStep_t;

/* 待机动画: 睁眼 2 秒 -> 眨一下眼 -> 睁眼 1.2 秒 -> 开心 0.8 秒 -> 循环 */
static const PetAnimStep_t anim_idle[] = {
    { pet_frame_open,  2000 },
    { pet_frame_blink,  120 },
    { pet_frame_open,  1200 },
    { pet_frame_happy,  800 },
};
#define ANIM_IDLE_LEN  (sizeof(anim_idle) / sizeof(anim_idle[0]))

/* 单帧表情(指令强制切换时用), 顺序对应 PetFace_t 枚举 */
static const uint8_t *face_frame[PET_FACE_COUNT] = {
    pet_frame_open,      /* NORMAL */
    pet_frame_blink,     /* BLINK  */
    pet_frame_happy,     /* HAPPY  */
};

/* ---------- 宠物状态 ---------- */
static struct {
    PetFace_t face;        /* 当前被指令切换的表情(只有 NORMAL 才跑待机脚本) */
    uint8_t   idle_step;   /* 待机动画走到第几步 */
    uint32_t  last_ms;     /* 上次换帧时刻(SysTick_GetTick) */
} pet;

/* 画一帧: 拷进缓冲区 + 整屏刷新 */
static void Pet_Draw(const uint8_t *frame)
{
    OLED_ShowFrame(frame);
    OLED_Refresh();
}

void Pet_Init(void)
{
    pet.face      = PET_FACE_NORMAL;
    pet.idle_step = 0;
    pet.last_ms   = SysTick_GetTick();
    Pet_Draw(pet_frame_open);    /* 开机先显示正常脸 */
}

/* 指令切换表情: 立即显示, 不用等动画计时 */
void Pet_SetFace(PetFace_t face)
{
    if (face >= PET_FACE_COUNT) face = PET_FACE_NORMAL;
    pet.face      = face;
    pet.idle_step = 0;
    pet.last_ms   = SysTick_GetTick();
    Pet_Draw(face_frame[face]);
}

/* 主循环里反复调用:
 *   - 时间没到就返回(不刷屏, 省电省时间)
 *   - 时间到了: 画当前步骤的画面, 然后走到下一步
 *   - 如果被指令切到了别的表情, 就一直显示那个表情(等下次指令再切回) */
void Pet_Update(void)
{
    uint32_t now = SysTick_GetTick();
    const PetAnimStep_t *step = &anim_idle[pet.idle_step];

    if ((uint32_t)(now - pet.last_ms) < step->hold_ms) {
        return;
    }
    pet.last_ms = now;

    if (pet.face == PET_FACE_NORMAL) {
        Pet_Draw(step->frame);
    } else {
        Pet_Draw(face_frame[pet.face]);
    }

    pet.idle_step = (uint8_t)((pet.idle_step + 1) % ANIM_IDLE_LEN);
}