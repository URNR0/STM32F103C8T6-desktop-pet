/* nvstore.c —— 掉电记忆: 用内部 Flash 保存数据
 *
 * 原理: STM32 的 Flash 掉电后内容不丢(和 RAM 不同)。我们把宠物情绪
 * 存档写到 Flash 的一个固定页里, 下次开机读回来, 宠物就"记得"你了。
 *
 * Flash 写操作注意事项:
 *   1. 写之前必须先擦除(Flash 只能整页擦, 一页 1KB)
 *   2. 擦/写都要先 FLASH_Unlock() 解锁, 用完 FLASH_Lock() 锁上
 *   3. 只能按 16 位(半字)写, 所以数据拆成几个半字写
 *   4. 擦写次数有限(约 1 万次), 所以别每秒钟都写
 */
#include "nvstore.h"
#include "stm32f10x.h"

/* 用 64KB Flash 的最后一页(Page 63)存存档。
 * 页起始地址 0x0800FC00, 大小 1KB。程序代码很小, 用不到这一页。 */
#define SAVE_ADDR     0x0800FC00UL

/* 页头 magic: 用来判断这页是不是我们写过的有效数据 */
#define SAVE_MAGIC    0xA5A5u

/* 存档格式版本, 以后改数据结构就 +1, 老数据会自动被判为无效 */
#define SAVE_VERSION  1u

/* 从 Flash 读存档。返回 1=读到有效数据, 0=没有(首次开机/数据损坏) */
uint8_t NvStore_Load(PetSave_t *out)
{
    uint16_t magic;
    uint16_t w1, w2;

    magic = *(volatile uint16_t *)SAVE_ADDR;
    if (magic != SAVE_MAGIC) {
        return 0;                       /* 首次开机, 或这页没写过 */
    }

    w1 = *(volatile uint16_t *)(SAVE_ADDR + 2);
    w2 = *(volatile uint16_t *)(SAVE_ADDR + 4);

    /* 版本不符也判无效(以后改格式时用) */
    if ((uint8_t)(w2 >> 8) != SAVE_VERSION) {
        return 0;
    }

    out->mood   = (uint8_t)(w1 & 0xFF);
    out->hunger = (uint8_t)(w1 >> 8);
    out->bond   = (uint8_t)(w2 & 0xFF);
    return 1;
}

/* 把存档写进 Flash(会擦写一整页)。调用方自己控制频率。 */
void NvStore_Save(const PetSave_t *in)
{
    uint16_t w1 = (uint16_t)(in->mood) | ((uint16_t)(in->hunger) << 8);
    uint16_t w2 = (uint16_t)(in->bond) | ((uint16_t)SAVE_VERSION << 8);

    FLASH_Unlock();
    FLASH_ClearFlag(FLASH_FLAG_BSY | FLASH_FLAG_EOP |
                    FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
    FLASH_ErasePage(SAVE_ADDR);
    FLASH_ProgramHalfWord(SAVE_ADDR,     SAVE_MAGIC);
    FLASH_ProgramHalfWord(SAVE_ADDR + 2, w1);
    FLASH_ProgramHalfWord(SAVE_ADDR + 4, w2);
    FLASH_Lock();
}
