/* pet.h —— 宠物形象 + 待机动画的头文件
 *
 * 设计思路:
 *   1. 动画 = 一帧一帧换图片(就像翻页动画)。
 *      每一帧是一整屏 128x64 的图片, 存成一个 1024 字节的数组。
 *   2. 一个"动画脚本"就是按顺序排列的帧列表, 每帧还配上显示时长。
 *   3. Pet_Update() 放在主循环里反复调用, 内部用 HAL_GetTick() 计时,
 *      时间到了就自动切到下一帧。
 *   4. 串口/按键想强制换表情, 调 Pet_SetFace() 即可。
 */
#ifndef __PET_H
#define __PET_H

#include <stdint.h>

/* 宠物表情(状态机里的情绪状态; BLINK 是 NORMAL 下的瞬时动作) */
typedef enum {
    PET_FACE_NORMAL = 0,   /* 醒着(睁眼, 会眨眼) */
    PET_FACE_BLINK,        /* 眨眼(瞬时动作, 不是持久状态) */
    PET_FACE_HAPPY,        /* 开心 */
    PET_FACE_SLEEP,        /* 打盹(闭眼 + Zzz) */
    PET_FACE_SAD,          /* 委屈(垂眼角 + 倒U嘴) */
    PET_FACE_COUNT         /* 自动计数, 别删 */
} PetFace_t;

void Pet_Init(void);                    /* 开机调用一次: 显示第一帧 */
void Pet_Update(void);                  /* 主循环反复调用: 按时间换帧 */
void Pet_SetFace(PetFace_t face);       /* 外部指令切换表情(串口/按键调用) */
PetFace_t Pet_GetFace(void);            /* 查询当前状态(按键切换睡/醒用) */

/* 状态变化通知回调(蓝牙上报用): 宠物状态真正切换时被调用, 参数是新状态 */
typedef void (*PetNotify_t)(PetFace_t face);
void Pet_SetNotify(PetNotify_t fn);     /* 注册回调, 传 NULL 取消 */

#endif /* __PET_H */