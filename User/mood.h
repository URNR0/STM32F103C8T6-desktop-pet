/* mood.h —— 情绪模型: 让宠物有"心情/饥饿/亲密度"三个数值
 *
 * 原理一句话: 宠物不再只会被动响应你的指令, 而是有一组会自己变化的"情绪值",
 * 情绪值反过来决定它摆什么表情、什么时候委屈、什么时候开心。
 *
 *   - mood   (心情 0~100): 越久没人理越低落, 互动会回升
 *   - hunger (饥饿 0~100): 随时间慢慢上涨, 喂食会下降
 *   - bond   (亲密度 0~100): 每次互动累积, 代表你们有多熟
 *
 * 表情联动:
 *   - 心情过低(<30) 或 太饿(>80) -> 宠物自动委屈(SAD)
 *   - 喂食/互动 -> 宠物开心(HAPPY)
 *
 * Mood_Tick() 放在主循环里反复调用, 内部自己计时, 每秒更新一次。
 */
#ifndef __MOOD_H
#define __MOOD_H

#include <stdint.h>

void    Mood_Init(void);        /* 开机调用一次: 初始化情绪值 */
void    Mood_Tick(void);        /* 主循环反复调用: 每秒更新情绪 + 触发表情 */
void    Mood_Feed(void);        /* 喂食: 降饥饿、提心情、加亲密度, 并开心 */
void    Mood_Play(void);        /* 摸头/互动: 提心情、加亲密度, 并开心 */

uint8_t Mood_GetMood(void);     /* 心情 0~100 */
uint8_t Mood_GetHunger(void);   /* 饥饿 0~100 */
uint8_t Mood_GetBond(void);     /* 亲密度 0~100 */

#endif /* __MOOD_H */
