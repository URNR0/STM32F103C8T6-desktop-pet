/* pet.c —— 宠物动画(状态机版)
 *
 * 状态机包含 4 个情绪状态 + 1 个瞬时动作:
 *   NORMAL   醒着(默认), 会定时眨眼
 *   SLEEP    打盹(空闲太久自动进入, 闭眼 + Zzz)
 *   HAPPY    开心(指令触发, 持续几秒后回正常)
 *   SAD      委屈(指令触发, 持续几秒后回正常)
 *   BLINK    眨眼(瞬时动作, 不占状态)
 *
 * 用大白话总结行为:
 *   - 平时睁着眼, 每 2 秒眨一下。
 *   - 一直没人理它(无指令)超过 30 秒, 就自己睡着了。
 *   - 发 HAPPY/SAD 让它开心/委屈, 3 秒后自己缓过来回正常。
 *   - 睡着的宠物, 任何指令都能把它叫醒。
 *
 * 调参数只改下面几个常量; 换形象只改 gen_assets.py 重新生成帧。
 */
#include "pet.h"
#include "oled.h"
#include "pet_frames.h"
#include "delay.h"          /* 标准库下自己提供 SysTick_GetTick */

/* ---------- 可调参数(时间单位: 毫秒) ---------- */
#define BLINK_EVERY_MS    2000     /* 醒着时每隔多久眨一次眼 */
#define BLINK_HOLD_MS      120     /* 眨眼(闭眼)持续多久 */
#define IDLE_TO_SLEEP_MS 30000     /* 空闲多久自动打盹 */
#define MOOD_HOLD_MS      3000     /* 开心/委屈持续多久后回正常 */

/* ---------- 宠物运行状态 ---------- */
static struct {
    PetFace_t  state;              /* 当前情绪状态 */
    uint8_t    blink_phase;        /* NORMAL 下的眨眼: 0=睁眼 1=闭眼 */
    uint32_t   last_ms;            /* 上次换帧时刻 */
    uint32_t   last_interact_ms;   /* 最后一次收到指令的时刻(判空闲用) */
} pet;

/* 画一帧: 拷进缓冲区 + 整屏刷新 */
static void Pet_Draw(const uint8_t *frame)
{
    OLED_ShowFrame(frame);
    OLED_Refresh();
}

/* 切换到某个状态, 立即画出对应表情 */
static void Pet_EnterState(PetFace_t face)
{
    pet.state       = face;
    pet.blink_phase = 0;
    pet.last_ms     = SysTick_GetTick();

    switch (face) {
    case PET_FACE_NORMAL: Pet_Draw(pet_frame_open);  break;
    case PET_FACE_BLINK:  Pet_Draw(pet_frame_blink); break;
    case PET_FACE_HAPPY:  Pet_Draw(pet_frame_happy); break;
    case PET_FACE_SLEEP:  Pet_Draw(pet_frame_sleep); break;
    case PET_FACE_SAD:    Pet_Draw(pet_frame_sad);   break;
    default: break;
    }
}

void Pet_Init(void)
{
    pet.state            = PET_FACE_NORMAL;
    pet.blink_phase      = 0;
    pet.last_ms          = SysTick_GetTick();
    pet.last_interact_ms = SysTick_GetTick();
    Pet_Draw(pet_frame_open);       /* 开机先显示正常脸 */
}

/* 查询当前状态(按键切换睡/醒时用) */
PetFace_t Pet_GetFace(void)
{
    return pet.state;
}

/* 指令(串口/按键)切换表情: 立即生效, 并重置"空闲计时" */
void Pet_SetFace(PetFace_t face)
{
    if (face >= PET_FACE_COUNT) face = PET_FACE_NORMAL;

    pet.last_interact_ms = SysTick_GetTick();   /* 有互动了, 重新计时 */

    if (face == PET_FACE_BLINK) {
        /* 眨眼是瞬时动作: 先保证醒着, 再立刻闭一下眼 */
        if (pet.state == PET_FACE_SLEEP) {
            pet.state = PET_FACE_NORMAL;
        }
        pet.blink_phase = 1;
        pet.last_ms     = SysTick_GetTick();
        Pet_Draw(pet_frame_blink);
        return;
    }

    Pet_EnterState(face);
}

/* 主循环反复调用: 驱动状态机换帧。时间没到就直接返回(不刷屏) */
void Pet_Update(void)
{
    uint32_t now = SysTick_GetTick();

    switch (pet.state) {

    case PET_FACE_NORMAL:
        /* 空闲太久没人理 -> 打盹 */
        if ((uint32_t)(now - pet.last_interact_ms) >= IDLE_TO_SLEEP_MS) {
            Pet_EnterState(PET_FACE_SLEEP);
            break;
        }
        /* 眨眼子状态: 1=在闭眼, 0=在睁眼 */
        if (pet.blink_phase) {
            if ((uint32_t)(now - pet.last_ms) >= BLINK_HOLD_MS) {
                pet.blink_phase = 0;            /* 闭够了, 睁开 */
                pet.last_ms = now;
                Pet_Draw(pet_frame_open);
            }
        } else {
            if ((uint32_t)(now - pet.last_ms) >= BLINK_EVERY_MS) {
                pet.blink_phase = 1;            /* 到点了, 眨一下 */
                pet.last_ms = now;
                Pet_Draw(pet_frame_blink);
            }
        }
        break;

    case PET_FACE_SLEEP:
        /* 打盹: 静态显示, 等指令叫醒(Pet_SetFace 会切状态) */
        break;

    case PET_FACE_HAPPY:
    case PET_FACE_SAD:
        /* 临时情绪, 持续一段时间后自己缓回正常 */
        if ((uint32_t)(now - pet.last_ms) >= MOOD_HOLD_MS) {
            Pet_EnterState(PET_FACE_NORMAL);
        }
        break;

    default:
        break;
    }
}