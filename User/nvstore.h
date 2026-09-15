/* nvstore.h —— 掉电记忆: 用内部 Flash 保存数据, 关机不丢
 *
 * 原理: STM32 的 Flash 掉电后内容不丢(和 RAM 不同)。我们把宠物情绪
 * 存档写到 Flash 的一个固定页里, 下次开机读回来, 宠物就"记得"你了。
 *
 * 用法:
 *   开机: NvStore_Load() 读回存档, 失败(首次开机)就用默认值
 *   互动后: NvStore_Save() 保存存档
 *
 * 注意: Flash 擦写有寿命(约 1 万次), 所以 Save 别太频繁。
 */
#ifndef __NVSTORE_H
#define __NVSTORE_H

#include <stdint.h>

/* 宠物存档数据结构 */
typedef struct {
    uint8_t mood;      /* 心情 0~100 */
    uint8_t hunger;    /* 饥饿 0~100 */
    uint8_t bond;      /* 亲密度 0~100 */
} PetSave_t;

/* 开机调用: 从 Flash 读回存档。返回 1=读到有效数据, 0=没有(首次开机)。 */
uint8_t NvStore_Load(PetSave_t *out);

/* 保存存档到 Flash(会擦写一整页)。调用方自己控制频率, 别太频繁。 */
void NvStore_Save(const PetSave_t *in);

#endif /* __NVSTORE_H */
